# Splitting huge functions: pilot on 0x43f0e0

The 139 unmatched functions over 1,000 bytes hold about 258 KB, the bulk of
what is left. This records a pilot of one way to split such a function between
agents, what it produced and what we learned. Nothing here is a match: the
function is still far from byte-identical.

## Idea

MSVC allocates registers and lays out blocks across the whole function, so a
fragment cannot be compiled and matched alone. The work is split instead of the
function:

1. **Skeleton** (one strong model): types, locals, the prologue and frame,
   control-flow skeleton, region markers.
2. **Regions** (cheap models, in parallel): each agent owns the code between
   `// REGION rN begin` and `// REGION rN end` and nothing else.
3. **Merge and integration** (orchestrator): merge branches, score, fix what
   is global.

Target budget for one function is 20 minutes of wall clock in total, so the
phases add up: skeleton 0 to 5, regions 5 to 14 (all launched together, about
9 minutes each), merge 14 to 17, integration 17 to 20.

## What was run

- **Function:** `0x43f0e0`, 4,420 bytes, never attempted before. A 14-case
  switch that returns an order name (`Class_00438760`) from a cursor mode. Only
  four callees.
- **Skeleton:** written by hand (Claude Sonnet 5.5) from the disassembly and
  Ghidra's pseudo-C. It compiles to 4,428 bytes against 4,420.
- **Regions** (address ranges in `data/regions/0x43f0e0.csv`):
  r1 case 1, r2 case 2, r3 case 3, r4 cases 4 to 11, 13, 14 and r5 case 12.
- **Agents:** five `opencode-go/deepseek-v4.1-flash` agents, one worktree and
  branch each (`pilot-r1` to `pilot-r5`), 3 then 2 at a time. Brief and tasks
  were kept in `build/scratch/pilot/`.
- **Tool:** `tools/regcheck.py` (new). `check.py` scores one aligned diff of the
  whole function, which reads about 11% for a draft with the right shape but a
  different frame and block order. `regcheck.py` instead matches each original
  basic block to its best block anywhere in our output and reports the average
  per address region, exactly and by "shape" (register names ignored). It is a
  progress signal only: small blocks inflate it and it is not a match check.
- **Timing:** the pilot ran about 33 minutes (16:47 to 17:20). The limits were
  loosened at the start (25 compile runs or 40 minutes) and tightened while
  running (20, then 9 to 10 minutes), so r1 ran far over.

## Results

Whole function, before and after merging all five branches:

| | `regcheck.py` exact | `regcheck.py` shape | `check.py` |
| --- | ---: | ---: | ---: |
| Skeleton | 56.8% | 72.1% | 11.2% |
| Merged | 63.4% | 79.2% | 14.1% |

Size went from 4,428 to 4,556 bytes (original 4,420).

Per region:

| Region | Start exact / shape | Merged exact / shape |
| --- | ---: | ---: |
| prologue | 40% / 64% | 43% / 67% |
| case 3 | 61% / 75% | 62% / 84% |
| cases 9, 8, 7 | 59% / 67% | 74% / 80% |
| case 12 | 55% / 75% | 62% / 78% |
| small cases | 62% / 71% | 71% / 82% |
| case 2 | 62% / 73% | 75% / 86% |
| case 1 | 56% / 74% | 61% / 79% |

Every region improved and no region reached a match. Merging was cheap: r2 to
r5 merged with no conflicts. r1 conflicted because it edited shared code.

## What we learned

**Regions are coupled through shared code.** Helpers and struct types used by
several regions (`Visible`, `IsVtol`, `Pick`, `Lookup`, struct fields) move
every region's score at once. r2's edits raised regions it did not own. r1's
edits to `Visible` and `IsVtol` raised case 1 but lowered case 3, case 2 and
the small cases, so the merge kept its case 1 body and reverted the helper
edits. Rule adopted mid-pilot: agents do not edit shared helpers or struct
fields, they propose them in the final report with evidence.

**The skeleton must get the frame and calling conventions right first.** I did
not, and every region inherited the errors. r4 and r5 traced the remaining gap
to them:

- `FUN_004899b0` and `FUN_00489a90` are `__thiscall` (unit in `ecx`, target
  pushed). The draft declared them `__stdcall`, which adds a push at every call
  site.
- The original has no locals and saves no extra register. The draft saves one
  more (`push ecx` and a `pop ecx` on every return). The original also loads
  `unit->def` once and reuses it; the draft re-reads it in every case.
- `(x >> n) & 1` compiles to `shr reg, n; test regl, 1`, but `x & mask`
  compiles to `test regH, ...`. The original uses the first form in several
  places.
- `Visible`: the original has no `& 0x1f` on the shift count and uses `int`
  locals with a 32-bit `sar`. `Lookup` differs in its multiply and its
  `< 0xfffb` branch.

These are global, so region agents cannot fix them and further region work is
wasted until they are.

**The case seams are weaker than expected.** The compiler merges the cases'
exit tails and jumps between them, and case 1 calls the function again with
other modes. Cases are a useful unit of work but not independent code.

**Repeated patterns are a better seam than cases.** The same unit-lookup code
(cell to unit through 0xfffe redirection) and the same visibility test appear
six times, and `IsVtol ? a : b` order-name selection appears dozens of times.
Extracting these as shared inline helpers first shrinks the problem for every
region.

**Process.** Limits set only by an agent's own step count let r1 run 35
minutes. The time limit must be in the brief from the start and enforced by the
orchestrator. Merge order matters little, but each merge should be scored
before it is committed.

## Recommended workflow

1. Skeleton: types, callee calling conventions, one cached `def`-style local,
   the frame (no extra saved register), the shared helpers as inline functions,
   region markers. Do not fan out until `check.py` size is within about 1% and
   the prologue shape is 100%.
2. Fan out one agent per region, all at once, 9 minute limit, shared helpers
   and struct fields read-only. Agents report proposed shared changes.
3. Merge each branch, run `regcheck.py`, revert changes that lower the whole
   function score.
4. Apply the proposed shared changes one at a time, scoring each, then run
   `check.py` for the final result.

## Open questions

- Whether a good skeleton lets regions reach exact matches, or whether the last
  gap is always integration work for a strong model.
- Whether `regcheck.py` should become a real `check.py --region` with region
  stubs, or stay a scoring aid.
- Whether region issues fit the existing issue and pull request flow in
  `AGENTS.md`.

## Files

- `src/unsorted/0x43f0e0.cpp`: the merged draft, on branch `pilot-43f0e0`.
- `data/regions/0x43f0e0.csv`: region address ranges.
- `tools/regcheck.py`: per-region scoring.

## Pilot 2 plan: 0x4d8e60

Pilot 2 must test the workflow, not one function family, so it uses a function
with a different shape, area and helpers from 0x43f0e0. (An earlier draft of
this section chose 0x43e490, the cursor-id sibling of 0x43f0e0; it was dropped
because a good result there would owe too much to the copied helpers.)

**Function.** `0x4d8e60`, 2,644 bytes, never attempted, in the 0x4d0000 area
(compression, CRT and debug). It is the crash handler that writes
`ErrorLog.txt`: it builds a path, opens the file with `CreateFileA` and
`SetFilePointer`, then formats about 40 report lines with `sprintf` and appends
them with `WriteFile`, and finally calls `CloseHandle`. So it is flat, ordered
and string heavy, with no switch. It has a 20,732 dword frame (large stack
buffers, so `_alloca_probe`), 8 callees (`FUN_004d9c60`, `FUN_004d9ca0`,
`FUN_004ded60`, `sprintf`, the Win32 imports and a few small ones), two loops
and about 63 conditionals. The same "format a line, write it" step repeats
dozens of times, which is the repeated pattern the pilot 1 write-up says to
extract first.

**Regions.** The natural seams are the report sections, in address order. They
are small (about 250 to 500 bytes), so each agent gets a bounded piece.

| Region | Range | Content |
| --- | --- | --- |
| r1 | 0x4d8e60 to 0x4d90d1 | prologue, guard flag, build path, open file, header |
| r2 | 0x4d90d1 to 0x4d915f | "Exception handler called in", module and address lines |
| r3 | 0x4d915f to 0x4d9344 | ExceptionCode, access violation text, flags, address, parameters |
| r4 | 0x4d9344 to 0x4d9498 | Registers block |
| r5 | 0x4d9498 to 0x4d95ad | Bytes at CS:EIP (a loop) |
| r6 | 0x4d95ad to 0x4d96cf | Dr0 to Dr7 |
| r7 | 0x4d96cf to 0x4d98b4 | ContextFlags, floating point state, Cr0NpxState, write, close, return |

**Generic workflow, with the tools it needs (prep, not on the clock).**

1. `tools/regions.py <addr>`: propose region seams for any function. For a
   switch use the jump table; for flat code cut at anchors (string pushes,
   repeated calls, loop heads) into pieces of roughly 250 to 500 bytes. Output
   `data/regions/<addr>.csv`. For this pilot the table above is hand made and
   `regions.py` is written from what it needed.
2. `tools/regguard.py <branch>`: fail if a branch edited a line outside its own
   region markers. The skeleton's shared code (structs, helpers, the frame)
   sits between `// SHARED begin` and `// SHARED end`.
3. A launcher that creates one worktree per region, starts the agents, and
   kills each at its deadline.
4. `tools/regcheck.py` as built for pilot 1.

**On the clock (20 minutes, one function).**

| Minutes | Phase | Who |
| --- | --- | --- |
| 0 to 5 | Skeleton: structs (`EXCEPTION_POINTERS` and context), the frame and `_alloca_probe`, the log-line helper, region markers with stub bodies | strong model |
| 5 | Gate: `check.py` size within 1% of 2,644 and prologue shape 100. If missed, fan out anyway and keep the prologue for the strong model | orchestrator |
| 5 to 14 | Seven regions in parallel, 9 minute kill deadline, `SHARED` block read-only | flash agents |
| 14 to 17 | Merge, `regguard.py`, `regcheck.py` after each merge | orchestrator |
| 17 to 20 | Apply proposed shared changes one at a time, final `check.py` | strong model |

Seven agents at once is more than the usual two or three, and each compile
runs under Wine, so watch `uptime`. If the load average passes the core count,
run four then three with a 4 minute budget each.

**What to record.** Minutes per phase, `check.py` and `regcheck.py` scores at
the gate, after each merge and at the end, merge conflicts, whether the gate
was met, which shared changes the agents proposed and which raised the score,
and whether the log-line helper the skeleton defined survived unchanged.
Compare with pilot 1 (14.1% `check.py`, 63.4% exact and 79.2% shape).

**Success.** Finish inside 20 minutes with no conflict caused by an edit
outside a region, `check.py` at least 50%, and no region below 85% shape.
Stretch: a full MATCH. A result well under that with a clear reason is still
useful.

**Risks.** The large frame and `_alloca_probe` make the skeleton's prologue
hard, and pilot 1 showed the frame decides the score, so the gate may fail.
The `sprintf` argument pushes and the string order depend on how the skeleton
writes the log-line helper (macro, inline function or repeated code), and that
choice is shared by every region. Flat code has fewer natural seams than a
switch, so one region may end up needing pieces of another. Wine compile time
under seven agents may dominate the region phase.
