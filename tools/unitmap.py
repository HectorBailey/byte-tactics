"""Group the matched members under src/ into the units they belong to.

    uv run tools/unitmap.py            # rewrite data/units.json
    uv run tools/unitmap.py --check    # exit 1 if data/units.json is stale
    uv run tools/unitmap.py --list     # print the units, largest first

A unit is a class whose matched members are spread over more than one file in
src/unsorted/. Consolidating a unit means giving the class one home, so the map
records for each one:

  members      the matched members, in address order, and the file defining each
  files        every file the unit's members live in
  declared_in  the files that declare the class
  fields       the union of the field ledgers those declarations give, one entry
               per offset, with the other views of the same offset where they
               disagree (the reconciliation report)
  vtable       the vtable address, when data/symbols.csv knows one

Nothing here is a decision: a disagreement is reported, not resolved.

Writes data/units.json, which tools/unitgen.py reads.
"""

import argparse
import csv
import json
import re
import sys
from pathlib import Path

from check import ROOT, base_name

PROGRESS = ROOT / "data/progress.csv"
SYMBOLS = ROOT / "data/symbols.csv"
UNITS = ROOT / "data/units.json"

ANNOTATION = re.compile(r"^\s*//\s*FUNCTION:")
BLOCK = re.compile(r"^\s*(struct|class|union|enum)\s+([A-Za-z_]\w*)\s*(:[^{;]*)?\s*\{")
FIELD = re.compile(r"^\s*((?:const\s+|unsigned\s+|signed\s+|static\s+)*)"
                   r"([A-Za-z_][\w:]*[\w:<>]*\s*[*&]?)\s+([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*;")
OFFSET = re.compile(r"^\s*\+?\s*0x([0-9a-fA-F]+)")
STL_MEMBERS = ("_Ucopy", "_Ufill", "_Destroy", "_Construct")


def source_of(rel: str) -> str:
    return (ROOT / rel).read_text(errors="replace")


def prelude(text: str) -> str:
    """Everything before the first // FUNCTION: annotation in a file."""
    lines = text.splitlines()
    for i, line in enumerate(lines):
        if ANNOTATION.match(line):
            return "\n".join(lines[:i])
    return text


def class_body(text: str, name: str) -> list[str] | None:
    """The lines inside `class/struct <name> { ... }`, or None."""
    lines = text.splitlines()
    for i, line in enumerate(lines):
        m = BLOCK.match(line)
        if not (m and m.group(2) == name and m.group(1) in ("class", "struct")):
            continue
        depth, body = 0, []
        for j in range(i, len(lines)):
            if j > i:
                body.append(lines[j])
            depth += lines[j].count("{") - lines[j].count("}")
            if j > i and depth <= 0:
                return body[:-1]
        return body
    return None


def class_bases(text: str, name: str) -> list[str]:
    """The base classes the header declares, without access or virtual."""
    for line in text.splitlines():
        m = BLOCK.match(line)
        if m and m.group(2) == name and m.group(1) in ("class", "struct") and m.group(3):
            out = []
            for part in m.group(3).split(","):
                words = [w for w in part.split() if w not in ("public", "private", "protected", "virtual")]
                if words:
                    out.append(words[-1])
            return out
    return []


def fields_of(body: list[str]) -> list[dict]:
    """The data members of a class body, with the offset its comment gives."""
    out = []
    for line in body:
        code, _, comment = line.partition("//")
        if "(" in code:                     # methods, and function pointers
            continue
        m = FIELD.match(code)
        if not m:
            continue
        type_ = (m.group(1) + m.group(2)).strip()
        if type_ in ("public", "private", "protected"):
            continue
        off = OFFSET.search(comment.strip())
        out.append({"offset": int(off.group(1), 16) if off else None,
                    "type": type_, "name": m.group(3), "array": m.group(4) or ""})
    return out


def vtable_address(symbols: dict[str, int], unit: str) -> str | None:
    address = symbols.get(f"??_7{unit}@@6B@")
    return f"{address:#x}" if address else None


def build() -> dict:
    with PROGRESS.open() as fh:
        rows = [r for r in csv.DictReader(fh) if r["status"] == "matched"]
    symbols = {}
    with SYMBOLS.open() as fh:
        for r in csv.DictReader(fh):
            symbols.setdefault(r["name"], int(r["address"], 16))

    members: dict[str, list[dict]] = {}
    for r in rows:
        name = base_name(r["symbol"])
        if "::" not in name:
            continue
        unit = name.split("::")[0]
        if unit in ("std", "type_info") or unit.startswith("std::"):
            continue
        members.setdefault(unit, []).append(
            {"address": r["address"], "name": name, "file": r["file"], "size": int(r["size"] or 0)})

    units = {}
    for unit, mem in sorted(members.items()):
        if len(mem) < 2:                    # one member cannot be consolidated
            continue
        mem.sort(key=lambda m: int(m["address"], 16))
        files = sorted({m["file"] for m in mem})

        ledgers = {}
        for f in files:
            body = class_body(source_of(f), unit)
            if body is not None:
                ledgers[f] = fields_of(body)
        # The richest declaration leads the union; other views become conflicts.
        reference = max(files, key=lambda f: (len(ledgers.get(f, [])), -files.index(f)))
        union: dict[object, dict] = {}
        for f in [reference] + [f for f in files if f != reference]:
            for field in ledgers.get(f, []):
                key = field["offset"] if field["offset"] is not None else field["name"]
                view = (field["type"], field["name"], field["array"])
                entry = union.setdefault(key, {"offset": field["offset"], "type": field["type"],
                                               "name": field["name"], "array": field["array"],
                                               "file": f, "others": []})
                if entry["file"] != f and view != (entry["type"], entry["name"], entry["array"]) \
                        and view not in [(o["type"], o["name"], o["array"]) for o in entry["others"]]:
                    entry["others"].append({"type": field["type"], "name": field["name"],
                                            "array": field["array"], "file": f})
        fields = sorted(union.values(), key=lambda e: (e["offset"] is None, e["offset"] or 0))
        conflicts = [{"offset": e["offset"], "name": e["name"], "file": e["file"],
                      "views": [{"type": e["type"], "name": e["name"], "array": e["array"],
                                 "file": e["file"]}] + e["others"]}
                     for e in fields if e["others"]]
        for c in conflicts:
            offset = f"{c['offset']:#x}" if c["offset"] is not None else c["name"]
            for v in c["views"]:
                v["offset"] = offset

        units[unit] = {
            "kind": "stl" if any(m["name"].split("::")[-1].startswith(STL_MEMBERS)
                                 for m in mem) or unit.startswith("UElem_") else "class",
            "bases": sorted({b for f in files for b in class_bases(source_of(f), unit)}),
            "files": files,
            "declared_in": sorted(ledgers),
            "members": mem,
            "fields": fields,
            "conflicts": conflicts,
            "vtable": vtable_address(symbols, unit),
            "safe": not conflicts,
        }
    return {"units": units}


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--check", action="store_true", help="exit 1 if data/units.json is not current")
    ap.add_argument("--list", action="store_true", help="print the units, largest first")
    args = ap.parse_args()

    text = json.dumps(build(), indent=1, sort_keys=True) + "\n"
    if args.list:
        units = json.loads(text)["units"]
        for u in sorted(units.values(), key=lambda u: -len(u["members"])):
            size = sum(m["size"] for m in u["members"])
            state = "clean" if u["safe"] else f"{len(u['conflicts'])} in dispute"
            unit = u["members"][0]["name"].split("::")[0]
            print(f"{len(u['members']):3d} members {size:7d}B  {len(u['files']):2d} files  "
                  f"{state:14s} {unit}")
        return
    if args.check:
        sys.exit(0 if UNITS.exists() and UNITS.read_text() == text
                 else "data/units.json is not current, run: uv run tools/unitmap.py")
    UNITS.write_text(text)
    units = json.loads(text)["units"]
    clean = sum(1 for u in units.values() if u["safe"])
    print(f"{len(units)} units, {sum(len(u['members']) for u in units.values())} functions, "
          f"{len({f for u in units.values() for f in u['files']})} files")
    print(f"  {clean} with agreeing declarations, {len(units) - clean} with an offset in dispute")
    print(f"wrote {UNITS.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
