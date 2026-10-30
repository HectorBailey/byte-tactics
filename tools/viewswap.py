"""Measure a file's views moving onto a header type, without touching src/.

    uv run tools/viewswap.py src/frontend/options.cpp --pairs pairs.csv
    uv run tools/viewswap.py src/frontend/options.cpp --all
    uv run tools/viewswap.py src/gui/gui_file.cpp --pairs pairs.csv --whole

A `pairs.csv` row is `view,header[,member-map]`, for example
`Source_004adc70,TdfFile,tdf=current` or `Obj_004ae440,Gadget,text=u.text`;
several members are separated by `;`. `--all` derives the pairs from the
file's own /Z7 type data instead.

For each pair the tool:

1. Refuses it when the layouts disagree, before compiling anything. Both the
   view and the header type come out of one `/Z7` compile of a probe that
   includes the header and the file (the same compile tools/gametypes.py
   does). Every member the view names must sit at the header member's offset
   with the same width, and a bitfield must match by position and width.
2. Applies the swap in a scratch copy, never in `src/`: the view's definition
   goes, its name becomes the header's, the member map is applied to every
   use of a view-typed name, and the header is included. The copy sits at a
   path of the original's length (tools/idpad.py's trick), so the compiler's
   symbol counter sees the same path and the comparison is fair.
3. Compiles it and compares every `// FUNCTION:` address of the file, not
   only the ones near the change, printing whether it compiled, how many kept
   and each function that moved with its percentage and byte counts. When a
   function moved it runs tools/idpad.py's search for real declarations and
   prints the filler it found or "no candidate".
4. Reports the decorated names that change, because data/progress.csv has to
   follow them.
5. Tries the whole file at once (`--whole`): one include of the header with
   every pair's view typed to it.

`--all` takes every view the file defines and looks, in its own type data,
for a type of the same size whose named members agree at the same offsets,
then proposes the best match per view and lists the views it could not match.

Nothing under `src/` or `data/` is written: the scratch copies and objects
live under `build/`, and the tree is clean at the end.
"""

import argparse
import os
import re
import shutil
import struct
import sys
from pathlib import Path

from check import DEFAULT_FLAGS, ROOT, Original, annotations, compare, compile_source
from coff import parse_object
from ghidratypes import (FWDREF, LF_BITFIELD, LF_CLASS, LF_MEMBER, LF_STRUCTURE, LF_UNION,
                         TypeTable, numeric, pstring)
import idpad
from trycasts import code_mask

FLAGS = DEFAULT_FLAGS + " /Z7"
SCRATCH = ROOT / "build" / "scratch" / "viewswap"
COMPOSITES = (LF_CLASS, LF_STRUCTURE, LF_UNION)
DEFINITION = re.compile(r"^\s*(?:typedef\s+)?(?:struct|class|union)\s+(\w+)\s*(?::[^{;]*)?\{", re.M)
FILLER = re.compile(r"^(?:unknown|unused|pad|padding|reserved|field|gap)_?\w*$", re.I)
ANONYMOUS = {"", "__unnamed", "<unnamed-tag>", "unnamed"}
ADDRESSED = re.compile(r"^(\w+)_?0x?[0-9a-fA-F]{6}$")


# --- the type data ------------------------------------------------------------

def header_for(source: Path, name: str) -> Path | None:
    """The header (or file) that defines a type, preferring one the source includes."""
    roots = [source.parent, ROOT / "src", ROOT / "include"]
    candidates = []
    for root in roots:
        for path in sorted(root.rglob("*")):
            if not path.is_file() or path.suffix not in (".h", ".hpp", ".inl", ".cpp"):
                continue
            text = path.read_text(errors="replace")
            if DEFINITION.search(text) and re.search(
                    rf"\b(?:struct|class|union)\s+{re.escape(name)}\s*(?::[^{{;]*)?\{{", text):
                candidates.append(path)
    if not candidates:
        return None
    text = source.read_text(errors="replace")
    for path in candidates:
        if path.name in text:
            return path
    return candidates[0]


def read_fields(table: TypeTable, ti: int, base: int = 0) -> list[dict]:
    """One composite's named members: an anonymous union's flattened at their own
    offsets, so `Gadget.text` reads as the enclosing type's field."""
    full = table.resolve(ti)
    leaf, d = table.records.get(full, (0, b""))
    if leaf not in COMPOSITES or struct.unpack_from("<H", d, 2)[0] & FWDREF:
        return []
    fields = []
    for sub, s, pos in table.fieldlist(struct.unpack_from("<I", d, 4)[0]):
        if sub != LF_MEMBER:
            continue
        mt = struct.unpack_from("<I", s, pos + 2)[0]
        off, p = numeric(s, pos + 6)
        field = {"off": base + off, "name": pstring(s, p)[0], "width": table.size_of(mt)}
        kind = table.expr(mt)
        if isinstance(kind, dict) and isinstance(kind.get("ref"), str):
            field["ref"] = kind["ref"]
        bl, bd = table.records.get(mt, (0, b""))
        if bl == LF_BITFIELD:
            field["bits"] = (bd[5], bd[4])  # position, length
            fields.append(field)
            continue
        nested = table.resolve(mt) if mt >= 0x1000 and mt in table.records else 0
        nleaf = table.records.get(nested, (0, b""))[0]
        if nleaf in COMPOSITES and table.composite_name(nested) in ANONYMOUS:
            fields.extend(read_fields(table, mt, base + off))
            continue
        fields.append(field)
    return fields


def layout(table: TypeTable, name: str) -> dict | None:
    """A named composite's size and members, from one object's CodeView data."""
    for ti, (leaf, _) in table.records.items():
        if leaf not in COMPOSITES or table.composite_name(ti) != name:
            continue
        full = table.resolve(ti)
        fields = read_fields(table, full)
        if not fields and struct.unpack_from("<H", table.records[full][1], 2)[0] & FWDREF:
            return None
        return {"name": name, "size": table.size_of(full), "fields": fields}
    return None


def member(layout_: dict, path: str, table: TypeTable) -> dict | None:
    """The member a dotted path names, its offset summed along the way."""
    parts = path.split(".")
    current, offset = layout_, 0
    for i, part in enumerate(parts):
        hit = next((f for f in current["fields"] if f["name"] == part), None)
        if hit is None:
            return None
        offset += hit["off"]
        if i == len(parts) - 1:
            return {**hit, "off": offset}
        expr = hit.get("ref")
        current = layout(table, expr) if expr else None
        if current is None:
            return {**hit, "off": offset}
    return None


def probe_layouts(source: Path, view: str, header_type: str) -> tuple[dict, dict, TypeTable, str]:
    """One /Z7 compile that holds both types: (view layout, header layout, table, note)."""
    header = header_for(source, header_type)
    if header is None:
        return {}, {}, TypeTable(b""), f"no file declares {header_type}"
    SCRATCH.mkdir(parents=True, exist_ok=True)
    rel_source = os.path.relpath(source, source.parent).replace(os.sep, "/")
    rel_header = os.path.relpath(header, source.parent).replace(os.sep, "/")
    text = (f'#include "{rel_header}"\n'
            f'#include "{rel_source}"\n'
            f"static {view} viewswap_probe_view;\n"
            f"static {header_type} viewswap_probe_header;\n")
    stem = re.sub(r"\W+", "_", f"{view}_{header_type}")
    probe = SCRATCH / f"layout_{stem}.cpp"
    probe.parent.mkdir(parents=True, exist_ok=True)
    probe.write_text(text)
    obj, log = compile_source(probe, FLAGS, out_dir="scratch/viewswap/layout",
                              includes=(source.parent, header.parent))
    if obj is None:
        return {}, {}, TypeTable(b""), f"the layout probe did not compile ({first_error(log)})"
    o = parse_object(obj.read_bytes(), obj.name)
    debug = next((s for s in o.sections if s.name == ".debug$T"), None)
    if debug is None:
        return {}, {}, TypeTable(b""), "the probe has no .debug$T section"
    table = TypeTable(debug.data)
    view_layout = layout(table, view)
    header_layout = layout(table, header_type)
    if view_layout is None:
        return {}, {}, table, f"no full definition of {view} in the probe"
    if header_layout is None:
        return {}, {}, table, f"no full definition of {header_type} in the probe"
    return view_layout, header_layout, table, ""


def first_error(log: str) -> str:
    for line in log.splitlines():
        if "error" in line.lower():
            return line.strip()
    return log.strip().splitlines()[-1] if log.strip() else "no output"


# --- the refusal --------------------------------------------------------------

def refuse(view: dict, header: dict, mapping: dict[str, str], table: TypeTable) -> str | None:
    """The reason the pair cannot move, or None when the layouts agree.

    Only the members the view names are compared: a view is a partial window
    onto the header, so it may be smaller than the type it becomes.
    """
    for field in view["fields"]:
        if FILLER.match(field["name"]) or field["name"] in ("base_", "vftable"):
            continue
        target = mapping.get(field["name"], field["name"])
        hit = member(header, target, table)
        if hit is None:
            return f"{view['name']}.{field['name']} has no {header['name']}.{target}"
        if hit["off"] != field["off"]:
            return (f"{view['name']}.{field['name']} is at +{field['off']:#x}, "
                    f"{header['name']}.{target} at +{hit['off']:#x}")
        if field.get("bits") != hit.get("bits"):
            if field.get("bits"):
                return (f"{view['name']}.{field['name']} is a {field['bits'][1]}-bit field at bit "
                        f"{field['bits'][0]}, {header['name']}.{target} "
                        + (f"is a {hit['bits'][1]}-bit field at bit {hit['bits'][0]}"
                           if hit.get("bits") else f"is {hit['width']} bytes wide"))
            return (f"{view['name']}.{field['name']} is {field['width']} bytes wide, "
                    f"{header['name']}.{target} is a bitfield")
        if hit["width"] != field["width"]:
            return (f"{view['name']}.{field['name']} is {field['width']} bytes wide, "
                    f"{header['name']}.{target} {hit['width']}")
    return None


# --- the swap -----------------------------------------------------------------

def remove_definition(text: str, name: str) -> tuple[str, bool]:
    """Cut `struct name { ... };` out of a file, braces matched."""
    occurrence = (r"^\s*(?:typedef\s+)?(?:struct|class|union)\s+" + re.escape(name)
                  + r"\s*(?::[^{;]*)?\{")
    for m in re.finditer(occurrence, text, re.M):
        depth, i = 0, m.end() - 1
        while i < len(text):
            if text[i] == "{":
                depth += 1
            elif text[i] == "}":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        end = text.find(";", i)
        if end < 0:
            continue
        start = m.start()
        while start > 0 and text[start - 1] in " \t":
            start -= 1
        return text[:start] + text[end + 1:].lstrip("\n"), True
    return text, False


def view_names(text: str, view: str) -> set[str]:
    """Names declared or cast as the view: the uses the member map applies to."""
    names = set(re.findall(rf"\b{re.escape(view)}\b\s*\**\s*(\w+)\s*(?=[;=,\[\)])", text))
    names |= set(re.findall(rf"\(\s*{re.escape(view)}\s*\*\s*\)\s*(\w+)", text))
    return {n for n in names if n not in ("struct", "class", "union")}


def apply_pair(text: str, view: str, header: str, mapping: dict[str, str]) -> tuple[str, int, int]:
    """Swap one view for the header in a file's text: (text, uses renamed, definition cut)."""
    names = view_names(text, view)
    renamed = 0
    for old, new in mapping.items():
        pattern = re.compile(rf"\b({'|'.join(map(re.escape, names)) or '\\0'})\s*(->|\.)\s*{re.escape(old)}\b")
        text, n = pattern.subn(lambda m: f"{m.group(1)}{m.group(2)}{new}", text)
        renamed += n
    text, cut = remove_definition(text, view)
    text = re.sub(rf"\b{re.escape(view)}\b", header, text)
    return text, renamed, 1 if cut else 0


def add_include(text: str, source: Path, header: Path) -> str:
    """Put one `#include` in, after the file's last include."""
    rel = os.path.relpath(header, source.parent).replace(os.sep, "/")
    line = f'#include "{rel}"\n'
    if f'"{rel}"' in text:
        return text
    lines = text.splitlines(keepends=True)
    last = max((i for i, l in enumerate(lines) if l.lstrip().startswith("#include")), default=-1)
    if last < 0:
        return line + text
    lines.insert(last + 1, line)
    return "".join(lines)


# --- measuring ----------------------------------------------------------------

def names_in(obj) -> set[str]:
    """The decorated names the object defines."""
    out = set()
    for s in obj.symbols:
        if s.name.startswith("?") and s.storage_class == 2 and s.section > 0:
            out.add(s.name)
    return out


def measure(source: Path, text: str, addresses: list[int], qual: dict, out: str):
    """Compile one variant of the file and compare all its functions."""
    variant = idpad.variant_path(source, len(addresses) * 7 + len(text) % 97)
    variant.write_text(text)
    obj, log = compile_source(variant, out_dir=out, includes=(source.parent,))
    if obj is None:
        return None, None, log
    ours = parse_object(obj.read_bytes(), obj.name)
    orig = Original()
    results = {a: compare(orig, ours, a, None, qual.get(a)) for a in addresses}
    return obj, results, log


def moved(results: dict) -> dict[int, object]:
    return {a: r for a, r in results.items() if not r.matched}


def idpad_hint(source: Path, text: str, results: dict[int, object], target: int,
               qual: dict, depth: int) -> str:
    """The filler that brings one moved function back, or "no candidate"."""
    lines = text.splitlines(keepends=True)
    first = next((i for i, l in enumerate(lines)
                  if l.startswith("// FUNCTION:") and int(l.split()[2], 16) == target), None)
    if first is None:
        return "no // FUNCTION: line"
    last_include = max((i for i, l in enumerate(lines[:first]) if l.lstrip().startswith("#include")),
                       default=-1)
    at = last_include + 1
    for k in range(1, depth + 1):
        block = idpad.padding_declarations(text, k)
        if not block:
            return "no candidate"
        body = list(lines)
        body[at:at] = (["// Unused here: the symbol ids these declarations take keep "
                       f"0x{target:x} matching (docs/c2-regalloc.md).\n"]
                      + [line + "\n" for line in block])
        _, hit, _ = measure(source, "".join(body), [target], qual, "scratch/viewswap/idpad")
        if hit and hit[target].matched:
            return "; ".join(block)
    return "no candidate"


# --- reporting ----------------------------------------------------------------

def line(pair: str, results: dict[int, object] | None, error: str, total: int) -> str:
    if results is None:
        return f"{pair}: did not compile ({first_error(error)})"
    kept = sum(1 for r in results.values() if r.matched)
    text = f"{pair}: compiles, {kept} of {total} MATCH"
    if kept < total:
        parts = []
        for a, r in sorted(moved(results).items()):
            parts.append(f"0x{a:x} {r.ratio * 100:5.1f}% ({r.ours_size} vs {r.size} bytes)")
        text += "; moved: " + ", ".join(parts)
    return text


def read_pairs(path: Path) -> list[tuple[str, str, dict[str, str]]]:
    out = []
    for row in path.read_text().splitlines():
        row = row.strip()
        if not row or row.startswith("#"):
            continue
        parts = [p.strip() for p in row.split(",")]
        if len(parts) < 2:
            sys.exit(f"{path}: '{row}' is not view,header[,member-map]")
        mapping = {}
        if len(parts) > 2 and parts[2]:
            for item in re.split(r"[;|]", parts[2]):
                if not item.strip():
                    continue
                old, _, new = item.partition("=")
                mapping[old.strip()] = new.strip()
        out.append((parts[0], parts[1], mapping))
    return out


def derive_pairs(source: Path, text: str) -> tuple[list[tuple[str, str, dict[str, str]]], list[str]]:
    """Views in the file matched, by layout, against the types its TU knows."""
    views = {m.group(1) for m in DEFINITION.finditer(text)}
    if not views:
        return [], ["the file defines no struct, class or union"]
    header = header_for(source, next(iter(views)))
    if header is None:
        return [], ["no header declares any of the file's types"]
    probe = SCRATCH / "all.cpp"
    SCRATCH.mkdir(parents=True, exist_ok=True)
    rel = os.path.relpath(header, source.parent).replace(os.sep, "/")
    probe.write_text(f'#include "{rel}"\n#include "{source.name}"\n')
    obj, log = compile_source(probe, FLAGS, out_dir="scratch/viewswap/all",
                              includes=(source.parent, header.parent))
    if obj is None:
        return [], [f"the probe did not compile ({first_error(log)})"]
    o = parse_object(obj.read_bytes(), obj.name)
    debug = next((s for s in o.sections if s.name == ".debug$T"), None)
    if debug is None:
        return [], ["the probe has no .debug$T section"]
    table = TypeTable(debug.data)
    known: dict[str, dict] = {}
    for ti, (leaf, _) in table.records.items():
        if leaf in COMPOSITES:
            found = table.composite_name(ti)
            got = layout(table, found)
            if got and not found.startswith("__"):
                known.setdefault(found, got)
    pairs, unmatched = [], []
    for view in sorted(views):
        mine = layout(table, view)
        if not mine:
            unmatched.append(f"{view}: no layout in the type data")
            continue
        best, best_hits = None, -1
        for name, other in known.items():
            if name == view or name in views or other["size"] != mine["size"]:
                continue
            hits = 0
            for field in mine["fields"]:
                if FILLER.match(field["name"]):
                    continue
                hit = member(other, field["name"], table)
                if hit and hit["off"] == field["off"] and hit["width"] == field["width"]:
                    hits += 1
            if hits > best_hits:
                best, best_hits = name, hits
        if best and best_hits:
            pairs.append((view, best, {}))
        else:
            unmatched.append(f"{view}: no type of the same size agrees at its offsets")
    return pairs, unmatched


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("file", type=Path)
    ap.add_argument("--pairs", type=Path, help="view,header[,member-map] rows")
    ap.add_argument("--all", action="store_true", help="derive the pairs from the file's type data")
    ap.add_argument("--whole", action="store_true", help="also try every pair at once")
    ap.add_argument("--idpad-depth", type=int, default=8, help="paddings to try per moved function")
    ap.add_argument("--keep", action="store_true", help="keep the scratch copies")
    args = ap.parse_args()

    source = args.file if args.file.is_absolute() else (Path.cwd() / args.file)
    source = source.resolve()
    if not source.is_file():
        sys.exit(f"{args.file} is not a file")
    rel = source.relative_to(ROOT)
    text = source.read_text(encoding="latin-1")
    qual = dict(annotations(source))
    addresses = sorted(qual)
    if not addresses:
        sys.exit(f"{rel} has no // FUNCTION: address")
    total = len(addresses)
    print(f"{rel}: {total} function(s)")

    base_obj, base_results, base_log = measure(source, text, addresses, qual, "scratch/viewswap/base")
    if base_results is None:
        sys.exit(f"the file does not compile as it is: {first_error(base_log)}")
    kept = sum(1 for r in base_results.values() if r.matched)
    print(f"  now: {kept} of {total} MATCH")
    base_names = names_in(parse_object(base_obj.read_bytes(), base_obj.name))

    pairs = read_pairs(args.pairs) if args.pairs else []
    if args.all or not pairs:
        derived, unmatched = derive_pairs(source, text)
        for note in unmatched:
            print(f"  --all: {note}")
        pairs = pairs or derived
    if not pairs:
        sys.exit("no pair to try: give --pairs or --all")

    try:
        moved_any = False
        for view, header_type, mapping in pairs:
            view_layout, header_layout, table, note = probe_layouts(source, view, header_type)
            label = f"{view} -> {header_type}"
            if note:
                print(f"  {label}: refused, {note}")
                continue
            reason = refuse(view_layout, header_layout, mapping, table)
            if reason:
                print(f"  {label}: refused, {reason}")
                continue
            swapped, uses, cut = apply_pair(text, view, header_type, mapping)
            if not cut:
                print(f"  {label}: refused, no definition of {view} in the file")
                continue
            swapped = add_include(swapped, source, header_for(source, header_type))
            obj, results, log = measure(source, swapped, addresses, qual, "scratch/viewswap/swap")
            print("  " + line(label + f" [{uses} member use(s)]", results, log, total))
            if results is None:
                continue
            for a in sorted(moved(results)):
                moved_any = True
                print(f"      0x{a:x}: idpad: {idpad_hint(source, swapped, results, a, qual, args.idpad_depth)}")
            if obj:
                swapped_names = names_in(parse_object(obj.read_bytes(), obj.name))
                gone = sorted(base_names - swapped_names)
                new = sorted(swapped_names - base_names)
                for name in gone:
                    print(f"      name gone: {name}")
                for name in new:
                    print(f"      name new:  {name}")
        if args.whole and len(pairs) > 1:
            header_type = pairs[0][1]
            whole = text
            for view, _, mapping in pairs:
                whole, _, _ = apply_pair(whole, view, header_type, mapping)
            whole = add_include(whole, source, header_for(source, header_type))
            obj, results, log = measure(source, whole, addresses, qual, "scratch/viewswap/whole")
            print("  " + line(f"whole file -> {header_type}", results, log, total))
        return 0
    finally:
        for d in idpad.CREATED:
            if not args.keep:
                shutil.rmtree(d, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
