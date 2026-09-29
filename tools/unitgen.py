"""Emit one merged unit source from data/units.json and the files in src/.

    uv run tools/unitgen.py Class_004b0610
    uv run tools/unitgen.py Class_004b0610 -o build/units/Class_004b0610.cpp

The unit's members live in several files, and each file declares its own view of
the class. This emits one candidate file: the reference file's declaration with
the other views merged in (any member they declare that the reference does not,
and the most specific type where two views disagree about a field), the union of
the other files' includes and declarations, and every member's definition copied
from the file that holds it, in address order.

Every line comes from an existing file; nothing is invented. Where two views
disagree about a field's type the choice is a heuristic, so the unit's conflicts
from the map are printed and the candidate is not a commit. Merging changes the
compiler state and the inline budget, so every function has to be re-checked
with tools/check.py once the file compiles.

Writes under build/ by default, which git ignores.
"""

import argparse
import json
import re
from pathlib import Path

from check import ROOT

UNITS = ROOT / "data/units.json"
ANNOTATION = re.compile(r"^\s*//\s*FUNCTION:\s*(0x[0-9a-fA-F]+)")
BLOCK = re.compile(r"^\s*(struct|class|union|enum)\s+([A-Za-z_]\w*)\s*(:[^{;]*)?\s*\{")
FIELD = re.compile(r"^\s*((?:const\s+|unsigned\s+|signed\s+|static\s+)*)"
                   r"([A-Za-z_][\w:]*[\w:<>]*\s*[*&]?)\s+([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*;")
SCALARS = ("int", "unsigned", "char", "short", "long", "bool", "float", "double", "void", "__int64")


def read(rel: str) -> str:
    return (ROOT / rel).read_text(errors="replace")


def prelude_of(text: str) -> list[str]:
    lines = text.splitlines()
    for i, line in enumerate(lines):
        if ANNOTATION.match(line):
            return lines[:i]
    return lines


def find_block(lines: list[str], name: str) -> tuple[int, int] | None:
    """(start, end) inclusive of the `class/struct <name> { ... };` block."""
    for i, line in enumerate(lines):
        m = BLOCK.match(line)
        if not (m and m.group(2) == name and m.group(1) in ("class", "struct")):
            continue
        depth = 0
        for j in range(i, len(lines)):
            depth += lines[j].count("{") - lines[j].count("}")
            if depth <= 0:
                return i, j
    return None


def split_prelude(lines: list[str]) -> list[tuple[str, str | None, list[str]]]:
    """('block', name, lines) for definitions, ('text', None, line) for the rest."""
    out, i = [], 0
    while i < len(lines):
        m = BLOCK.match(lines[i])
        if not m:
            out.append(("text", None, [lines[i]]))
            i += 1
            continue
        depth, j = 0, i
        while j < len(lines):
            depth += lines[j].count("{") - lines[j].count("}")
            if depth <= 0:
                break
            j += 1
        out.append(("block", m.group(2), lines[i:j + 1]))
        i = j + 1
    return out


def fields_of(body: list[str]) -> dict[object, dict]:
    """key (offset, or name when no offset) -> the field and the line it is on."""
    out = {}
    for line in body:
        code = line.split("//")[0]
        if "(" in code:
            continue
        m = FIELD.match(code)
        if not m:
            continue
        type_ = (m.group(1) + m.group(2)).strip()
        if type_ in ("public", "private", "protected"):
            continue
        off = re.search(r"^\s*\+?\s*0x([0-9a-fA-F]+)", line.partition("//")[2].strip())
        offset = int(off.group(1), 16) if off else None
        key = offset if offset is not None else m.group(3)
        out[key] = {"offset": offset, "type": type_, "name": m.group(3),
                    "array": m.group(4) or "", "line": line}
    return out


def methods_of(body: list[str]) -> list[tuple[str, str]]:
    """(name, declaration line) for each method declaration in a class body."""
    out = []
    for line in body:
        code = line.split("//")[0].strip()
        if "(" not in code or not code.endswith(";"):
            continue
        head = code.split("(")[0].split()
        if not head:
            continue
        out.append((head[-1].lstrip("*&"), line))
    return out


def specificity(type_: str, array: str) -> int:
    """A crude ranking for choosing between two views of one field. void* loses
    to a named pointer; a scalar loses to a named type. Deliberately simple: the
    choice is reported as a conflict and the reviewer decides."""
    t = type_.replace(" ", "")
    base = t.rstrip("*&")
    named = bool(re.match(r"^[A-Za-z_]\w*$", base)) and (base[0].isupper() or "_" in base)
    if t == "void*":
        return 0
    if t.endswith(("*", "&")):
        return 5 if named else 1
    if array:
        return 4 if named else 2
    return 3 if named else 1 if base in SCALARS else 2


def annotations(lines: list[str]) -> list[tuple[int, str]]:
    return [(i, m.group(1)) for i, line in enumerate(lines) if (m := ANNOTATION.match(line))]


def brace_end(lines: list[str], start: int) -> int:
    depth, seen = 0, False
    for i in range(start, len(lines)):
        for ch in lines[i].split("//")[0]:
            if ch == "{":
                depth, seen = depth + 1, True
            elif ch == "}":
                depth -= 1
        if seen and depth <= 0:
            return i
    return len(lines) - 1


def member_span(lines: list[str], anns: list[tuple[int, str]], address: str) -> list[str]:
    """The definition at `address`: its annotation, any body-less annotation
    before it, and the definition itself."""
    for i, addr in anns:
        if addr != address:
            continue
        end, j = i, i
        while True:
            rest = [l for l in lines[j + 1:] if l.strip() and not l.strip().startswith("//")]
            if rest and ("(" in rest[0] or "=" in rest[0]):
                return lines[i:brace_end(lines, j + 1) + 1]
            later = [a for a in anns if a[0] > j]
            if not later:
                break
            j = later[0][0]
            end = j
        return lines[i:end + 1]
    return []


def merge_declaration(unit: str, lines: list[str], views: dict[str, dict]) -> tuple[list[str], list[str], list[str]]:
    """Rewrite the reference file's class declaration in place: pick the most
    specific view of each field, and add the members the other views declare.
    Returns (lines, added members, fields that came only from other views)."""
    found = find_block(lines, unit)
    if found is None:
        return lines, [], []
    start, end = found
    body = lines[start + 1:end]
    chosen = {}
    for f, view in views.items():
        for key, field in view["fields"].items():
            best = chosen.get(key)
            if best is None or specificity(field["type"], field["array"]) > specificity(best["type"], best["array"]):
                chosen[key] = field

    out, added = list(lines), []
    for i in range(start + 1, end):
        out_i = lines[i]
        code_i = out_i.split("//")[0]
        if "(" in code_i:
            continue
        m = FIELD.match(code_i)
        if not m:
            continue
        off = re.search(r"^\s*\+?\s*0x([0-9a-fA-F]+)", out_i.partition("//")[2].strip())
        key = int(off.group(1), 16) if off else m.group(3)
        pick = chosen.get(key)
        if pick is not None and (pick["type"], pick["name"], pick["array"]) != \
                ((m.group(1) + m.group(2)).strip(), m.group(3), m.group(4) or ""):
            comment = out_i.partition("//")[2]
            spacing = " " * max(1, 28 - len(pick["type"]) - len(pick["name"]) - len(pick["array"]))
            out_i = f"    {pick['type']} {pick['name']}{pick['array']};{spacing}// {comment.strip()}"
        out[i] = out_i

    present = {name for name, _ in methods_of(body)}
    insert = []
    for f, view in views.items():
        for name, decl in view["methods"]:
            if name in present:
                continue
            present.add(name)
            insert.append(decl)
            added.append(name)
    if insert:
        out[end:end] = [""] + insert
    return out, added, []


def generate(unit: str, entry: dict) -> tuple[str, str, list[str], list[str], list[str]]:
    files, members = entry["files"], entry["members"]

    def weight(f: str) -> tuple[int, int]:
        return (sum(1 for e in entry["fields"] if e["file"] == f),
                sum(1 for m in members if m["file"] == f))
    reference = max(files, key=weight)
    ref_lines = read(reference).splitlines()

    views = {}
    for f in [reference] + [f for f in files if f != reference]:
        text = read(f)
        block = find_block(text.splitlines(), unit)
        if block:
            body = text.splitlines()[block[0] + 1:block[1]]
            views[f] = {"fields": fields_of(body), "methods": methods_of(body)}

    ref_lines, added, _ = merge_declaration(unit, ref_lines, views)

    # Union the other preludes in: include lines, externs, forward and function
    # declarations, and every type block the reference does not declare. Source
    # order per file is preserved, so dependencies still hold.
    declared = {name for kind, name, _ in split_prelude(prelude_of(read(reference))) if kind == "block"}
    seen = {l.strip() for l in prelude_of(read(reference)) if l.strip()}
    extra, pulled = [], []
    for f in files:
        if f == reference:
            continue
        for kind, name, ls in split_prelude(prelude_of(read(f))):
            if kind == "block":
                if name in declared:
                    continue
                extra += [""] + ls
                declared.add(name)
                pulled.append(name)
            elif ls[0].strip() and ls[0].strip() not in seen:
                seen.add(ls[0].strip())
                extra.append(ls[0])
        extra.append("")

    # The merged class declaration now uses types declared only in the other
    # files, so those blocks have to sit above it, not below.
    block = find_block(ref_lines, unit)
    insert_at = block[0] if block else next(i for i, l in enumerate(ref_lines) if ANNOTATION.match(l))
    out = ref_lines[:insert_at] + extra + ref_lines[insert_at:]

    emitted = {m["address"] for m in members if m["file"] == reference}
    emitted |= {a for _, a in annotations(ref_lines)}
    for f in files:
        if f == reference:
            continue
        fl = read(f).splitlines()
        anns = annotations(fl)
        for m in sorted((m for m in members if m["file"] == f), key=lambda m: int(m["address"], 16)):
            if m["address"] in emitted:
                continue
            span = member_span(fl, anns, m["address"])
            if span:
                out += [""] + span
                emitted.add(m["address"])

    missing = [m["address"] for m in members if m["address"] not in emitted]
    unresolved = [b for b in entry.get("bases", []) if b not in declared]
    return "\n".join(out) + "\n", reference, pulled, added, missing, unresolved


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("unit")
    ap.add_argument("-o", "--output", type=Path)
    args = ap.parse_args()

    if not UNITS.exists():
        raise SystemExit("data/units.json missing, run: uv run tools/unitmap.py")
    units = json.loads(UNITS.read_text())["units"]
    if args.unit not in units:
        near = [u for u in units if args.unit.lower() in u.lower()][:5]
        raise SystemExit(f"no unit {args.unit!r} in data/units.json"
                         + (f", did you mean: {', '.join(near)}" if near else ""))
    entry = units[args.unit]

    text, reference, pulled, added, missing, unresolved = generate(args.unit, entry)
    dest = args.output or ROOT / "build/units" / f"{args.unit}.cpp"
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_text(text)

    print(f"wrote {dest.relative_to(ROOT)} ({text.count(chr(10))} lines), "
          f"{len(entry['members'])} members from {len(entry['files'])} files")
    print(f"  reference declaration: {reference}")
    if added:
        print(f"  members added to the declaration: {', '.join(added)}")
    if pulled:
        print(f"  types pulled in: {', '.join(pulled)}")
    if missing:
        print(f"  NOT emitted: {', '.join(missing)}")
    if unresolved:
        print(f"  base class(es) not declared in the unit's files: {', '.join(unresolved)}")
    if entry["conflicts"]:
        print(f"  {len(entry['conflicts'])} offset(s) in dispute, resolved by the specificity rule:")
        for c in entry["conflicts"]:
            views = "  vs  ".join(f"{v['type']} {v['name']}{v['array']}" for v in c["views"])
            print(f"    {c['offset']}  {views}")


if __name__ == "__main__":
    main()
