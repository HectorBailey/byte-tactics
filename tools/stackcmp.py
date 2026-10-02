"""Match a near-miss function's stack slots to the original's.

    uv run tools/stackcmp.py 0x405980
    uv run tools/stackcmp.py 0x405980 src/unsorted/0x405980.cpp
    uv run tools/stackcmp.py --verify-all          # is /Z7 code-neutral?

When a function differs only in its stack frame, check.py's diff is a wall of
`[esp + 0x1c]` versus `[esp + 0x24]` and there is no way to tell which local is
which. This compiles the candidate once more with /Z7 to read the name and frame
offset of every local from CodeView, aligns our instructions with the original's
(ignoring stack operands and internal jump targets), and prints which of our
slots the original puts somewhere else. The code being compared is still the
normal build: --verify-all checks that /Z7 leaves the code bytes alone.

The last line is a `--stack` list for tools/permute.py, the locals whose slot is
wrong, so the permuter can aim its declaration moves at them.
"""

import argparse
import difflib
import re
import struct
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from check import (DEFAULT_FLAGS, PADDING, ROOT, Original, annotations, compare,  # noqa: E402
                   compile_source, disasm, find_source, link_placeholders, load_symbols,
                   normalise, select_function)
from coff import parse_object  # noqa: E402

DEBUG_FLAGS = DEFAULT_FLAGS + " /Z7"

# CodeView symbol records (VC5), little-endian.
S_END = 0x0006
S_REGISTER = 0x1001
S_BPREL32 = 0x1006
S_LPROC32 = 0x100a
S_GPROC32 = 0x100b
PROC_TYPES = (S_LPROC32, S_GPROC32)

# `[esp + 0x1c]`, `[esp - 4]`, `[esp]`: the frame slots MSVC 5 /O2 uses (it does
# not keep an EBP frame, so ebp operands are struct and global accesses).
STACK_OP = re.compile(r"\[esp(?:\s*([+-])\s*(0x[0-9a-f]+))?\]")
STACK_ANY = re.compile(r"\[esp(?:\s*[+-]\s*0x[0-9a-f]+)?\]")


# --- the original's FPO frame -------------------------------------------------

def fpo_records(orig: Original) -> dict[int, tuple[int, int, int, int]]:
    """address -> (locals bytes, parameter bytes, saved-register bytes, prologue bytes).

    IMAGE_DEBUG_TYPE_FPO holds one 16-byte FPO_DATA per function; cdwLocals is
    the frame the compiler reserved for locals, which is the number to compare
    our own frame against."""
    out: dict[int, tuple[int, int, int, int]] = {}
    for d in getattr(orig.pe, "DIRECTORY_ENTRY_DEBUG", []):
        if d.struct.Type != 3:
            continue
        raw = orig.pe.__data__[d.struct.PointerToRawData:d.struct.PointerToRawData + d.struct.SizeOfData]
        for i in range(0, len(raw), 16):
            start, _, locals_dw, params, packed = struct.unpack_from("<IIIHH", raw, i)
            saved = (packed >> 8) & 0x7
            prologue = packed & 0xFF
            out[orig.base + start] = (locals_dw * 4, params * 4, saved * 4, prologue)
    return out


def object_fpo(obj, symname: str) -> tuple[int, int, int, int] | None:
    """Our own FPO record from the /Z7 object's .debug$F section."""
    for sec in obj.sections:
        if sec.name == ".debug$F" and any(symname in r.symbol for r in sec.relocs):
            d = sec.data
            if len(d) < 16:
                continue
            locals_dw, params, packed = struct.unpack_from("<IHH", d, 8)
            return (locals_dw * 4, params * 4, ((packed >> 8) & 0x7) * 4, packed & 0xFF)
    return None


# --- CodeView locals ----------------------------------------------------------

def find_debug_section(obj, symname: str, short: str | None):
    for sec in obj.sections:
        if sec.name == ".debug$S" and any(symname in r.symbol for r in sec.relocs):
            return sec
    for sec in obj.sections:
        if sec.name != ".debug$S":
            continue
        for _, rectyp, name in cv_records(sec.data):
            if rectyp in PROC_TYPES and short and name.split("::")[-1] == short:
                return sec
    return None


def cv_records(data: bytes):
    """Yield (record offset, record type, proc name) for every CodeView record."""
    off = 0
    if len(data) >= 4 and struct.unpack_from("<I", data, 0)[0] == 4:
        off = 4
    while off + 4 <= len(data):
        reclen, rectyp = struct.unpack_from("<HH", data, off)
        if reclen == 0:
            break
        end = off + 2 + reclen
        raw = data[off:end]
        name = ""
        if rectyp in PROC_TYPES and len(raw) >= 40:
            n = raw[39]
            name = raw[40:40 + n].decode("latin-1")
        yield off, rectyp, name
        off = end


def parse_locals(sec) -> tuple[list[tuple[str, int]], list[tuple[str, int]]]:
    """(stack locals, register locals): (name, frame offset) and (name, register)."""
    stack: list[tuple[str, int]] = []
    regs: list[tuple[str, int]] = []
    for off, rectyp, _ in cv_records(sec.data):
        raw = sec.data[off:off + 2 + struct.unpack_from("<H", sec.data, off)[0]]
        if rectyp == S_BPREL32 and len(raw) >= 13:
            (value,) = struct.unpack_from("<i", raw, 4)
            n = raw[12]
            stack.append((raw[13:13 + n].decode("latin-1"), value))
        elif rectyp == S_REGISTER and len(raw) >= 11:
            (reg,) = struct.unpack_from("<H", raw, 8)
            n = raw[10]
            regs.append((raw[11:11 + n].decode("latin-1"), reg))
    return stack, regs


# --- instruction slots --------------------------------------------------------

def esp_disps(op_str: str) -> list[int]:
    out = []
    for m in STACK_OP.finditer(op_str):
        if m.group(2) is None:
            out.append(0)
        else:
            v = int(m.group(2), 16)
            out.append(-v if m.group(1) == "-" else v)
    return out


def esp_effect(ins) -> int:
    """Bytes by which this instruction lowers the stack pointer (below entry ESP)."""
    m, ops = ins.mnemonic, ins.op_str
    if m == "push":
        return 4
    if m == "pop":
        return -4
    if m == "lea" and ops.startswith("esp"):
        v = re.search(r"([+-])\s*0x([0-9a-f]+)", ops)
        if v:
            n = int(v.group(2), 16)
            return -n if v.group(1) == "+" else n
    return 0


def _reg_imm(ins_list, i: int, reg: str) -> int | None:
    """The constant `reg` most recently loaded with, if it is still live."""
    for k in range(i - 1, max(-1, i - 16), -1):
        ins = ins_list[k]
        if ins.op_str.startswith(reg + ","):
            rest = ins.op_str.split(",", 1)[1].strip()
            return int(rest, 16) if re.fullmatch(r"0x[0-9a-f]+", rest) else None
        if ins.mnemonic in ("xor", "add", "sub", "inc", "dec", "pop", "and", "or", "imul", "lea") \
                and ins.op_str.startswith(reg):
            return None
    return None


def effect(ins_list, i: int) -> int:
    """esp_effect, resolving `sub esp, eax` from the preceding `mov eax, N`
    (the `_chkstk` idiom for frames larger than one page)."""
    ins = ins_list[i]
    if ins.mnemonic in ("sub", "add") and ins.op_str.startswith("esp"):
        m = re.match(r"esp,\s*(.*)$", ins.op_str)
        arg = m.group(1).strip() if m else ""
        if re.fullmatch(r"0x[0-9a-f]+", arg):
            n = int(arg, 16)
        elif re.fullmatch(r"e?[a-z]{2}", arg):
            n = _reg_imm(ins_list, i, arg)
        else:
            n = None
        if n is not None:
            return n if ins.mnemonic == "sub" else -n
        return 0
    return esp_effect(ins)


def _branch_target(ins) -> int | None:
    if not (ins.mnemonic.startswith("j") or ins.mnemonic.startswith("loop")):
        return None
    m = re.search(r"0x([0-9a-f]+)", ins.op_str)
    return int(m.group(1), 16) if m else None


def _successors(ins_list, i: int, by_addr: dict[int, int]) -> list[int]:
    ins = ins_list[i]
    m = ins.mnemonic
    if m in ("ret", "retn", "retf"):
        return []
    if m == "jmp":
        target = _branch_target(ins)
        return [by_addr[target]] if target in by_addr else []
    if m.startswith("j") or m.startswith("loop"):
        out = []
        target = _branch_target(ins)
        if target in by_addr:
            out.append(by_addr[target])
        if i + 1 < len(ins_list):
            out.append(i + 1)
        return out
    return [i + 1] if i + 1 < len(ins_list) else []


def _delta_pass(ins_list, by_addr, resting: int | None):
    """Forward dataflow for bytes below entry ESP; None where a block was reached
    with two different stack depths (an unbalanced push on one path)."""
    n = len(ins_list)
    delta: list[int | None] = [None] * n
    conflict: set[int] = set()
    delta[0] = 0
    todo = [0]
    while todo:
        i = todo.pop()
        before = delta[i]
        ins = ins_list[i]
        if ins.mnemonic == "call" and resting is not None:
            nxt = ins_list[i + 1] if i + 1 < n else None
            cleans = nxt is not None and (
                (nxt.mnemonic in ("add", "lea") and nxt.op_str.startswith("esp")) or nxt.mnemonic == "pop")
            after = before if cleans else resting
        else:
            after = before + effect(ins_list, i)
        for s in _successors(ins_list, i, by_addr):
            if delta[s] is None:
                delta[s] = after
                todo.append(s)
            elif delta[s] != after:
                conflict.add(s)  # two paths, two depths: keep the first
    return delta, conflict


def stack_deltas(ins_list, resting_hint: int | None = None) -> list[int]:
    """Bytes below entry ESP at each instruction, for converting `[esp + N]`.

    `[esp + d]` with the stack pointer `delta` bytes below entry is the frame
    location `d - delta`, the same number CodeView reports for a local. delta
    follows the control flow; a call that cleans its own arguments (a stdcall
    callee with no following `add esp`) returns the stack pointer to the body's
    resting depth instead of leaving the pushed arguments counted. The resting
    depth is the frame's locals plus its saved registers (the FPO record); the
    mode of the raw depths is only a fallback, since uncorrected stdcall calls
    inflate it over a large function."""
    by_addr = {ins.address: i for i, ins in enumerate(ins_list)}
    if resting_hint is None:
        first, _ = _delta_pass(ins_list, by_addr, None)
        seen = [first[i] for i, ins in enumerate(ins_list) if first[i] is not None and esp_disps(ins.op_str)]
        resting_hint = Counter(seen).most_common(1)[0][0] if seen else 0
    second, _ = _delta_pass(ins_list, by_addr, resting_hint)
    return [resting_hint if d is None else d for d in second]


def slot_key(ins, lo: int, hi: int, in_image) -> str:
    """Instruction text with stack slots and code addresses masked, for alignment."""
    return STACK_ANY.sub("[esp+?]", normalise(ins, lo, hi, in_image, mask_targets=True))


def pair_slots(ours, theirs, ours_delta, theirs_delta, lo, hi, in_image):
    """(ours_offset -> Counter(theirs_offset), pairs) from aligned instructions.

    Instructions are aligned on their masked text, so a pair that differs only in
    a stack operand still lines up and its two frame offsets can be paired."""
    keys_o = [slot_key(i, lo, hi, in_image) for i in ours]
    keys_t = [slot_key(i, lo, hi, in_image) for i in theirs]
    mapping: dict[int, Counter] = {}
    pairs = 0

    def record(i: int, j: int):
        """Pair ours[i] with theirs[j]."""
        nonlocal pairs
        do, dt = esp_disps(ours[i].op_str), esp_disps(theirs[j].op_str)
        if len(do) != len(dt):
            return
        for a, b in zip(do, dt):
            oo, ot = a - ours_delta[i], b - theirs_delta[j]
            mapping.setdefault(oo, Counter())[ot] += 1
            pairs += 1

    for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(None, keys_t, keys_o, autojunk=False).get_opcodes():
        if tag == "equal":
            for k in range(i2 - i1):
                record(j1 + k, i1 + k)
            continue
        if tag == "replace" and (i2 - i1) == (j2 - j1):
            for k in range(i2 - i1):
                if keys_t[i1 + k] == keys_o[j1 + k]:
                    record(j1 + k, i1 + k)
            continue
        # Uneven block: pair greedily on the masked text, each instruction once.
        used: set[int] = set()
        for i in range(i1, i2):
            for j in range(j1, j2):
                if j not in used and keys_t[i] == keys_o[j]:
                    used.add(j)
                    record(j, i)
                    break
    return mapping, pairs


# --- report -------------------------------------------------------------------

REG_NAMES = {
    0: "ax", 1: "cx", 2: "dx", 3: "bx", 4: "sp", 5: "bp", 6: "si", 7: "di",
    8: "al", 9: "cl", 10: "dl", 11: "bl", 12: "ah", 13: "ch", 14: "dh", 15: "bh",
    17: "eax", 18: "ecx", 19: "edx", 20: "ebx", 21: "esp", 22: "ebp", 23: "esi", 24: "edi",
}


def signed(v: int) -> str:
    return f"+{v:#x}" if v >= 0 else f"-{-v:#x}"


def analyse(address: int, src: Path, want: str | None) -> int:
    qualname = next((q for a, q in annotations(src) if a == address), None)
    base_path, log = compile_source(src, out_dir="obj")
    if base_path is None:
        sys.exit(f"compile failed:\n{log}")
    dbg_path, dlog = compile_source(src, DEBUG_FLAGS, out_dir="dbg")
    if dbg_path is None:
        sys.exit(f"debug compile failed:\n{dlog}")
    base = parse_object(base_path.read_bytes(), base_path.name)
    dbg = parse_object(dbg_path.read_bytes(), dbg_path.name)

    picked, err = select_function(dbg, want, qualname)
    if not picked:
        sys.exit(f"cannot pick the function: {err}")
    name, sec, start, end = picked
    debug_data = sec.data[start:end]
    base_picked, err = select_function(base, want, qualname)
    if not base_picked or base_picked[1].data[base_picked[2]:base_picked[3]] != debug_data:
        print("warning: /Z7 changed this function's code bytes; slots may not line up")

    orig = Original()
    size = orig.sizes.get(address, len(debug_data))
    theirs_bytes = orig.read(address, size)
    in_image = lambda v: orig.base <= v < orig.end
    lo, hi = address, address + size

    data, mask = debug_data, sec.mask()[start:end]
    while data and data[-1] in PADDING and mask[-1]:
        data, mask = data[:-1], mask[:-1]
    ours_ins = disasm(link_placeholders(orig, sec, start, end, data, address, size), address)
    theirs_ins = disasm(theirs_bytes, address)

    stack, regs = [], []
    dbg_sec = find_debug_section(dbg, name, qualname.split("::")[-1] if qualname else None)
    if dbg_sec is not None:
        stack, regs = parse_locals(dbg_sec)

    ofpo, bfpo = object_fpo(dbg, name), fpo_records(orig).get(address)
    ours_rest = (ofpo[0] + ofpo[2]) if ofpo else None
    theirs_rest = (bfpo[0] + bfpo[2]) if bfpo else None
    ours_delta = stack_deltas(ours_ins, ours_rest)
    theirs_delta = stack_deltas(theirs_ins, theirs_rest)
    mapping, pairs = pair_slots(ours_ins, theirs_ins, ours_delta, theirs_delta, lo, hi, in_image)

    our_slots: set[int] = set()
    for i, ins in enumerate(ours_ins):
        our_slots.update(d - ours_delta[i] for d in esp_disps(ins.op_str))
    their_slots: set[int] = set()
    for i, ins in enumerate(theirs_ins):
        their_slots.update(d - theirs_delta[i] for d in esp_disps(ins.op_str))

    ratio = difflib.SequenceMatcher(
        None, [normalise(i, lo, hi, in_image) for i in theirs_ins],
        [normalise(i, lo, hi, in_image) for i in ours_ins], autojunk=False).ratio()

    print(f"{address:#x}  {name}  original {size} bytes, ours {len(data)} bytes  ->  {ratio * 100:.1f}%")
    ours_frame = f"{ofpo[0]:#x} locals + {ofpo[2]:#x} saved" if ofpo else "unknown"
    orig_frame = f"{bfpo[0]:#x} locals + {bfpo[2]:#x} saved" if bfpo else "unknown"
    print(f"frame: ours {ours_frame}   original {orig_frame}")
    print(f"aligned slot pairs: {pairs}")

    # A local at a frame offset may be reached through several instructions; take
    # the original offset that most of its aligned accesses agree on.
    by_offset: dict[int, list[str]] = {}
    for local, value in stack:
        by_offset.setdefault(value, []).append(local)

    rows, wrong = [], []
    for value in sorted(by_offset, reverse=True):
        names = "/".join(by_offset[value])
        if value not in our_slots:
            rows.append((names, signed(value), "-", "unused"))
            continue
        hits = mapping.get(value)
        if not hits:
            rows.append((names, signed(value), "-", "our only"))
            wrong.extend(by_offset[value])
            continue
        target, count = hits.most_common(1)[0]
        status = "ok" if target == value else "moved"
        if status == "moved":
            wrong.extend(by_offset[value])
        rows.append((names, signed(value), f"{signed(target)} ({count})", status))

    if rows:
        width = max(len(r[0]) for r in rows + [("local",)])
        print(f"\n  {'local':<{width}}  {'our slot':>10}  {'original':>14}  status")
        for names, ours_s, orig_s, status in rows:
            print(f"  {names:<{width}}  {ours_s:>10}  {orig_s:>14}  {status}")
    else:
        print("\nno stack locals in CodeView (all in registers or optimised away)")

    missing = sorted(s for s in their_slots if s not in by_offset and s not in our_slots)
    if missing:
        print("\noriginal slots no local of ours reaches: " + ", ".join(signed(s) for s in missing))
    if regs:
        print("\nin registers: " + ", ".join(f"{n}={REG_NAMES.get(r, f'r{r}')}" for n, r in regs))
    if wrong:
        print("\npermute --stack " + ",".join(dict.fromkeys(wrong)))
    return 0


# --- /Z7 neutrality across the matched set ------------------------------------

def verify_one(row) -> tuple[str, str]:
    address = int(row["address"], 16)
    src = ROOT / row["file"]
    qualname = next((q for a, q in annotations(src) if a == address), None)
    obj_path, log = compile_source(src, DEBUG_FLAGS, out_dir=f"dbgverify/{address:#x}")
    if obj_path is None:
        return row["address"], "compile failed"
    obj = parse_object(obj_path.read_bytes(), obj_path.name)
    res = compare(Original(), obj, address, None, qualname, load_symbols(), quick=True)
    return row["address"], "ok" if res.bytes_match else f"differs ({res.status})"


def verify_all(jobs: int) -> int:
    import csv
    from concurrent.futures import ProcessPoolExecutor
    rows = [r for r in csv.DictReader((ROOT / "data/progress.csv").open()) if r["status"] == "matched"]
    print(f"compiling {len(rows)} matched functions with /Z7 ...")
    bad = []
    with ProcessPoolExecutor(max_workers=jobs) as pool:
        for n, (address, result) in enumerate(pool.map(verify_one, rows), 1):
            if result != "ok":
                bad.append((address, result))
            if n % 250 == 0:
                print(f"  {n}/{len(rows)}")
    print(f"{len(rows) - len(bad)}/{len(rows)} unchanged by /Z7")
    for address, result in bad[:40]:
        print(f"  {address}  {result}")
    return 1 if bad else 0


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("address", nargs="?", help="function address, e.g. 0x405980")
    ap.add_argument("source", type=Path, nargs="?")
    ap.add_argument("--sym", help="substring of the mangled name, if the annotation can't be used")
    ap.add_argument("--verify-all", action="store_true",
                    help="check that /Z7 leaves every matched function's code bytes alone")
    ap.add_argument("--jobs", type=int, default=8, help="parallel compiles for --verify-all")
    args = ap.parse_args()

    if args.verify_all:
        sys.exit(verify_all(args.jobs))
    if not args.address:
        ap.error("an address is required unless --verify-all is given")
    address = int(args.address, 16)
    src = args.source or find_source(address)
    if src is None:
        sys.exit(f"no file under src/ has '// FUNCTION: {address:#x}'")
    sys.exit(analyse(address, src, args.sym))


if __name__ == "__main__":
    main()
