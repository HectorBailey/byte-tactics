"""Search for the declarations that give a file back its symbol ids.

    uv run tools/idpad.py src/units/unit.cpp 0x48b090
    uv run tools/idpad.py src/units/unit.cpp 0x48b090 0x489a90 --max 30
    uv run tools/idpad.py src/units/unit.cpp 0x48b090 --base upstream/main

Moving a file onto a shared header often breaks a match only because the
header's declarations move MSVC 5's symbol counter (docs/c2-regalloc.md). The
usual repair is a few real but unused declarations ("Unused here: ..."), and
working out how many and which is the slow part. This tries candidate paddings
and prints the ones that bring the listed functions back.

It reports the file's state now and on --base first, and stops if the listed
functions already match. A candidate is otherwise a set of real
zero-parameter prototypes from `include/ta_protos.h` (1 id each, and no type
of its own to pay for), placed just after the file's includes or just before
the first listed function, or the removal of one existing "Unused here"
block. Every variant is compiled in a scratch copy, in parallel, and the file
itself is never written.

List every // FUNCTION: address of the file, not only the ones that moved: a
padding after the includes moves the functions before the first listed one as
well.

The copy has to sit at a path of exactly the same length as the original: the
front end's counter moves 4 ids per character of the source path, so a copy
under build/scratch would measure a different file. Each variant therefore
goes in a short scratch directory at the root of the checkout, named to the
length of the original's directory, and the original's directory is passed on
the include path for its quoted includes. A copy made that way compiles to the
original's bytes: src/units/unit.cpp's 0x48b090 still matched from
Y:\\.idpad000\\unit.cpp.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from check import ROOT, Original, annotations, compare, compile_source
from coff import parse_object

PROTO = re.compile(r"^([A-Za-z_][\w \t\*]*?)\s+([A-Za-z_]\w*)\(([^();]*)\);\s*$", re.M)
PRIMITIVE = re.compile(r"^(?:void|int|char|short|long|unsigned|signed|float|double|bool"
                       r"|unsigned (?:int|char|short|long)|signed (?:int|char|short|long))\s*\*?$")
FUNCTION_LINE = re.compile(r"^// FUNCTION: 0x([0-9a-f]+)")
DECLARATION = re.compile(r"^[A-Za-z_][\w \t\*]*\b[A-Za-z_]\w*\([^;{}]*\);\s*$")
NAME_IN_DECL = re.compile(r"\b([A-Za-z_]\w*)\(")

CREATED: list[Path] = []


def prototypes() -> list[tuple[str, int]]:
    """`include/ta_protos.h` as (line, id cost): a free function is 1, plus one per parameter."""
    text = (ROOT / "include" / "ta_protos.h").read_text(errors="replace")
    out = []
    for ret, name, params in PROTO.findall(text):
        if not PRIMITIVE.fullmatch(ret.strip()):
            continue  # a return type of its own would cost its declaration too
        params = params.strip()
        count = 0 if params in ("", "void") else len([p for p in params.split(",") if p.strip()])
        out.append((f"{ret.strip()} {name}({params or 'void'});", 1 + count))
    return out


def padding_declarations(source: str, wanted: int) -> list[str]:
    """`wanted` ids' worth of real zero-parameter prototypes the file does not already name."""
    lines, total = [], 0
    for line, cost in prototypes():
        if cost != 1:
            continue
        name = NAME_IN_DECL.search(line).group(1)
        if re.search(rf"\b{re.escape(name)}\b", source):
            continue
        lines.append(line)
        total += 1
        if total >= wanted:
            return lines
    return []


def unused_blocks(lines: list[str]) -> list[tuple[int, int, int]]:
    """Each "Unused here" comment and the declarations under it: (start, end, id cost)."""
    blocks = []
    for i, line in enumerate(lines):
        if "Unused here" not in line:
            continue
        start = i
        while start > 0 and lines[start - 1].lstrip().startswith("//"):
            start -= 1
        end = i
        while end < len(lines) and (lines[end].lstrip().startswith("//") or not lines[end].strip()):
            end += 1
        cost = 0
        while end < len(lines) and DECLARATION.match(lines[end]):
            params = re.search(r"\(([^;]*)\)", lines[end]).group(1).strip()
            cost += 1 if params in ("", "void") else 1 + len([p for p in params.split(",") if p.strip()])
            end += 1
        if cost and (start, end) not in [(b[0], b[1]) for b in blocks]:
            blocks.append((start, end, cost))
    return blocks


def scratch_dir(index: int, length: int) -> Path:
    """A directory at the root of the checkout whose name is `length` characters long."""
    name = ("." + format(index, "x")).ljust(max(2, length), "q")
    out = ROOT / name
    if not out.exists():
        out.mkdir()
        CREATED.append(out)
    return out


def variant_path(source: Path, index: int) -> Path:
    rel = source.resolve().relative_to(ROOT)
    return scratch_dir(index, len(rel.parent.as_posix())) / rel.name


def attempt(variant: Path, text: str, include_dir: Path, targets: list[int], qual: dict):
    """Compile one variant and compare every target: (matched count, best ratio, note)."""
    variant.write_text(text)
    obj, log = compile_source(variant, out_dir=f"scratch/idpad/{variant.parent.name}",
                              includes=(include_dir,))
    if obj is None:
        return 0, 0.0, "did not compile"
    ours = parse_object(obj.read_bytes(), obj.name)
    orig = Original()
    results = [compare(orig, ours, a, None, qual.get(a)) for a in targets]
    return sum(1 for r in results if r.matched), max((r.ratio for r in results), default=0.0), ""


def read_base(source: Path, base: str) -> str | None:
    """The file as `base` has it, or None."""
    rel = source.resolve().relative_to(ROOT).as_posix()
    proc = subprocess.run(["git", "show", f"{base}:{rel}"], cwd=ROOT, capture_output=True, text=True)
    return proc.stdout if proc.returncode == 0 else None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("file", type=Path)
    ap.add_argument("addresses", nargs="+", type=lambda s: int(s, 16), metavar="ADDR")
    ap.add_argument("--max", type=int, default=40, help="largest padding, in symbol ids, to try")
    ap.add_argument("--jobs", type=int, default=min(8, os.cpu_count() or 4))
    ap.add_argument("--base", default="origin/main", help="revision to report the file's own state from")
    ap.add_argument("--keep", action="store_true", help="keep the scratch copies")
    args = ap.parse_args()

    source = args.file if args.file.is_absolute() else (Path.cwd() / args.file)
    source = source.resolve()
    if not source.is_file():
        sys.exit(f"{args.file} is not a file")
    rel = source.relative_to(ROOT)
    text = source.read_text()
    lines = text.splitlines(keepends=True)
    qual = dict(annotations(source))
    targets = args.addresses
    for a in targets:
        if a not in qual:
            sys.exit(f"{a:#x} is not a // FUNCTION: address in {rel}")
    include_dir = source.parent

    try:
        print(f"{rel}: {len(targets)} function(s), {len(prototypes())} prototypes in ta_protos.h")
        base = read_base(source, args.base)
        if base is None:
            print(f"  {args.base} does not have {rel}: no comparison")
        else:
            matched, ratio, note = attempt(variant_path(source, 0), base, include_dir, targets, qual)
            print(f"  {args.base}: {matched}/{len(targets)} MATCH"
                  + (f", closest {ratio * 100:.1f}%" if matched < len(targets) else "")
                  + (f" ({note})" if note else ""))

        matched, ratio, note = attempt(variant_path(source, 0x900), text, include_dir, targets, qual)
        print(f"  now: {matched}/{len(targets)} MATCH"
              + (f", closest {ratio * 100:.1f}%" if matched < len(targets) else "")
              + (f" ({note})" if note else ""))
        if matched == len(targets):
            print("  the file already matches every listed function: no padding needed")
            return 0

        first = next((i for i, l in enumerate(lines) if any(
            FUNCTION_LINE.match(l) and int(FUNCTION_LINE.match(l).group(1), 16) == a for a in targets)), None)
        if first is None:
            sys.exit("no // FUNCTION: line for the listed addresses")
        last_include = max((i for i, l in enumerate(lines[:first]) if l.lstrip().startswith("#include")),
                           default=-1)
        places = [("after the includes", last_include + 1)]
        if first != last_include + 1:
            places.append((f"before the first function (line {first + 1})", first))

        def with_block(block: list[str], at: int) -> str:
            note = ["// Unused here: the symbol ids these declarations take keep "
                    + ", ".join(f"0x{a:x}" for a in targets) + " matching (docs/c2-regalloc.md).\n"]
            body = list(lines)
            body[at:at] = note + [line + "\n" for line in block]
            return "".join(body)

        jobs = []  # (label, variant text, what it adds)
        for where, at in places:
            for k in range(1, args.max + 1):
                block = padding_declarations(text, k)
                if not block:
                    break
                jobs.append((f"add {k} zero-parameter declaration(s) {where}",
                             with_block(block, at), block, k))
        for start, end, cost in unused_blocks(lines):
            body = "".join(lines[:start] + lines[end:])
            jobs.append((f"remove the 'Unused here' block at line {start + 1} ({cost} ids)", body,
                         [f"remove lines {start + 1}-{end}"], -cost))

        def go(job):
            i, (label, variant, block, delta) = job
            n, ratio, note = attempt(variant_path(source, i + 1), variant, include_dir, targets, qual)
            return label, n, ratio, note, block, delta

        with ThreadPoolExecutor(max_workers=args.jobs) as pool:
            results = list(pool.map(go, enumerate(jobs)))

        good = [r for r in results if r[1] == len(targets)]
        if good:
            good.sort(key=lambda r: len(r[4]))
            label, n, ratio, note, block, delta = good[0]
            print(f"\nFOUND: {len(good)} candidate(s) make all {len(targets)} function(s) MATCH; smallest:")
            print(f"  {label}")
            print(f"  (the file is {abs(delta)} symbol id{'s' if abs(delta) != 1 else ''} "
                  f"{'short' if delta > 0 else 'over'} before these functions)")
            print("\n  apply it as:")
            print("    // Unused here: the symbol ids these declarations take keep "
                  + ", ".join(f"0x{a:x}" for a in targets) + " matching (docs/c2-regalloc.md).")
            for line in block:
                print("    " + line)
        else:
            results.sort(key=lambda r: (-r[1], -r[2], len(r[0])))
            print("\nno candidate matches; closest:")
            for label, n, ratio, note, _, _ in results[:5]:
                print(f"  {n}/{len(targets)} MATCH  {ratio * 100:5.1f}%  {label}"
                      + (f" ({note})" if note else ""))
        print(f"({len(jobs)} candidates compiled)")
        return 0 if good else 1
    finally:
        if not args.keep:
            for d in CREATED:
                shutil.rmtree(d, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
