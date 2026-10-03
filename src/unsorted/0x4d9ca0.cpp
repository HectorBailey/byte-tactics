// Decompiled by space-bunny-free, finished by muse-spark-1.3-free, finished by deepseek-v4.1-flash, finished by GPT-6.1-sol, finished by deepseek-v4.1-flash, finished by mimo-v2.6-pro, finished by Space Bunny Free. Names are provisional.
// Space Bunny Free: 82.8% (660 bytes), and the frame is now the original's:
// sub esp,0x18 with len@0x10, m@0x14, r/q@0x18, n/s@0x1c, this@0x20, pc@0x24. So the
// missing local the earlier notes kept missing is the pc copy the phase-2 loop walks,
// and it only appears if BOTH of these hold:
//   1. `unsigned long* q0 = pc;` exists at function scope (MSVC keeps it in a slot when
//      the phase-2 prefix reads `q0[i]`, and folds it away when it reads `*q`);
//   2. the two phase-2 arrays are INDEXED (`q0[i]` and `stack[i]`), not walked. That is
//      what makes MSVC strength-reduce both into the pointer walks the original has in
//      0x18/0x1c (a walked `q`/`s` pair puts the pc walk in 0x1c instead of 0x18).
// Getting there is worth +2.7 over the 80.1% 5-slot version; it came out of
// tools/permute.py run on build/scratch/0x4d9ca0/kFstart.cpp (79.7% by hand), best
// score in build/permute/0x4d9ca0/best_ratio.cpp.
// Still differs, and the first item is the one to try next:
//   - `this` is still in esi for the whole function (prologue `mov esi,ecx`, members
//     read off esi, a reload at the loop-1 latch). The original spills `this` to 0x20 in
//     the prologue and reads the members off ecx, so its esi is free for the loop temps.
//     Without the q0 local (same source with `pc[i]` instead of `q0[i]`, build/scratch/
//     0x4d9ca0/i2_idx.cpp, 626 bytes) MSVC DOES spill `this` and reads esi as the count,
//     exactly like the original, but then there is no 6th slot and MSVC indexes with
//     `[edx+ebp*4]` instead of strength-reducing. So the two requirements fight.
//   - loop 1 keeps n in a slot and reloads it after the sprintf; the original reloads it
//     into esi at the latch (`mov esi,[esp+0x1c]`) and does `dec esi` there.
//   - the phase-2 prefix loads the pc item with two instructions
//     (`mov ecx,[esp+0x18]; mov edx,[ecx]`) where the original has one (`mov edx,[esp+0x18]`),
//     i.e. the original prints the pc POINTER, not `*pc[i]`. See the BUG note below.
//   - the original's phase-2 separator compares i against [esp+0x20]-1, and 0x20 holds
//     `this`, so it compares i against this-1 (see BUG note). Ours reads m. Writing
//     `i == (int)ret - 1` moves m and this around in the slots and drops the score, so
//     that one is not just a spelling.
//   - the phase-2 latch and the prologue store order (scheduling only).
// Tried and byte-identical or worse in this shape: q0 typed void*/int*/const or written
// `&pc[0]`/`pc+0`; `q0 + i`/`*(q0+i)` spellings; `q0 = pc` at each of five declaration
// positions; `r = ret` before/after each strlen; the q/s assignment order in phase 2
// (all four orders); the separator operand order; reading count directly (no n local);
// one shared len instead of len2; and a `self` local holding this. A 512-point grid over
// those axes tops out at 79.7% by hand, and two 10-15 minute permuter runs on the
// 6-slot `unsigned long* q0 = pc;` + walked-q version topped out at 75.0%.
// mimo-v2.6-pro retry: 80.1% (641 bytes) via tools/permute.py inline-helper rewrites// mimo-v2.6-pro retry: 80.1% (641 bytes) via tools/permute.py inline-helper rewrites
// (inl0..inl8 wrap n-1, i%8, i, *s, s+1, m-1 etc; the odd if/else/do-while around the
// loop-2 preheader is a permuter artefact that happens to schedule better). Still differs:
// the original spills `this` to [esp+0x20] and keeps count/n transient in ESI with len in
// memory and i in EBP, frame 0x18 with pc spilled to [esp+0x24]; ours keeps `this` in ESI
// and len in EBP (prologue) so the loop-top reload and the n-1 register differ. The single
// unflipped decision is ESI holding n (original) vs this (ours). See the long notes below.
// mimo-v2.6-pro continuation (60-minute box, all below left ESI=this, nothing beat 80.1):
// clean rewrites (no helpers) 79.7; capturing q0=pc and/or r0=ret at function scope fixes
// frame 0x18 and the pc slot 0x24 but shifts m to 0x18 (73.0-73.4) and does NOT free ESI;
// deriving s as ((Class*)r0)->stack or (char*)r0+0x7c (so `this` dies in the prologue) still
// leaves the coalesced this/r0 value in ESI (72.5-73.0, type-punning r0 as void*/char* is
// identical); i=0 hoisted to function scope moves len to [esp+0x10] as
// `mov [esp+0x10], 0xa44c` exactly like the original (good) but score drops to 78.9 and
// ESI still holds this; redundant CSE-folding extra uses of n/i (guide register-priority
// trick) are byte-identical; member accesses wrapped in static inline getters (inlS/inlQ/
// inlR/inlM/inlN/inlP(this)) fold away byte-identically; void-helper out-param captures of
// s/q (inlPre(this,&s,&q)) before the phase-2 sprintf 79.7, after the strlens 78.4;
// count read direct with no n local 74.5 (this lands in EBP). Uninitialised-locals-get-
// callee-saved-in-declaration-order does not move the needle here (int i first is flat).
// Best remaining lead: the original's slot order (len=0x10, m=0x14, r=0x18, n=0x1c,
// r0/this=0x20, q0=0x24) reproduces exactly when q0 and r0 are captured at function scope
// AND m is declared uninit + assigned (not `int m = copied;`): with `int m = copied;` the
// compiler swaps m and r (r=0x14, m=0x18). Untested: capture q0/r0 with `int m; m = copied;`
// declaration style, then retune the tail scheduling (that shape was 72.5 raw).
// deepseek-v4.1-flash (seventh run, 10-minute box): still 79.2% (643 bytes). Tested, all worse
// or tied: dropping `const int n = count;` and reading count directly in both places 73.6 (637
// bytes); moving `int i = 0;` to last tied 79.2 but reverted to the known-best order.
// deepseek-v4.1-flash (sixth run, 10-minute box): 79.2% (643 bytes), up from 78.2.
// One lever gained the point: move `unsigned int len = 0xa44c;` to be the FIRST
// local, before `char* p = buf;`. Hoisting `q = pc;` and `int* s = stack;` to
// function scope (to kill `this` early) dropped to 75.1 (626 bytes), reverted:
// the rematerialised member leas reshuffle the whole tail.
// Still differs: original spills this to prologue slot 0x20, keeps count in ESI
// (tested at the top, no reload at the loop) and i in EBP, with len at [esp+0x10]
// and pc at [esp+0x24] (frame 0x18). Ours keeps this=ESI for the whole function,
// len in EBP only until its first post-call spill (frame 0x14), reloads count
// from [esp+0x1c] at the loop and never spills pc (phase 2 reloads [esi+0x2080]).
// Also probed this run, all worse or byte-flat: `q = pc;` alone at function scope
// 75.0, `int i = 0;` first 78.2, `const int n = count;` first 76.5, `int m = copied;`
// first 78.2, and `if (n > 0)` for the outer test byte-flat at 79.2 (kept).
// deepseek-v4.1-flash (fifth run): 78.2% (634 bytes), up from 70.6%. The whole gain came
// from one theme: make a value's live range START EARLIER, before the call it must
// survive, and let MSVC hoist the load into the prologue. Three edits, each measured:
//  (1) +4.0: `const int n = count;` as a function-scope local, used by BOTH the loop 1
//      bound (`i < n`) and the `i == n - 1` test (74.4%). `int n` (non-const) is the same
//      size but 2 points worse; using n in only one of the two places is worse (73.5/70.5).
//  (2) +3.0: `int* s = stack;` as the FIRST statement of the `if (m > 0 && len2 > 0x1e)`
//      block, i.e. BEFORE the header sprintf (77.8%). Pairing it with `unsigned long* q =
//      pc;` in the same place gave 75.2%, and both at function scope gave 73.9%.
//  (3) +0.4: put `unsigned long* q = pc;` AFTER the two strlen lines and `p += strlen(p)`,
//      so only `s` precedes the sprintf (78.2%). `s` after the sprintf, `q` after the
//      strlens is also 78.2; `q` before the sprintf is 74.8 to 77.8.
// Everything else tried this run was byte-identical or worse: `register int i`, dead extra
// locals, `const int m`, a fresh `const int k = m` as the loop 2 bound, moving `r = ret`
// before the phase 1 sprintf, `if (n > 0)` for the outer test, `&buf[0]`/`&stack[0]`/
// `&pc[0]`, a single shared `len` across both phases (61.1: the len2 copy is load-bearing),
// declaring len2 inside the block (78.2), function-scope q (73.5), a distinct
// function-scope pcv copied into q at phase 2 (31.3), and many local declaration orders.
// Remaining diff: the prologue still differs in framing and scheduling. Ours is
// `sub esp,0x14` with this in ESI (`mov esi,ecx`, then the members read from esi) and
// len materialised in EBP; the original is `sub esp,0x18`, reads copied/pc/count straight
// off ECX, spills ECX to [esp+0x20] and leaves i in EBP with len at [esp+0x10]. The
// loop-body scheduling differences (n reload, r++ ordering, the phase 2 pc temp at
// [esp+0x24]) are downstream of that one allocator decision.
// GPT-6.1-sol refinement: best is 70.6% after splitting second-phase len; still differs at prologue/frame and allocator placement, notably this held in ebp and the missing target spill frame.
// space-bunny-free retry: retained 69.5%; found the n/m/q plateau is one register
// SHORT of the original, not a different function. Declaring n=count, m=copied and
// q=pc as function-scope locals (build/scratch/0x4d9ca0/v1.cpp, 60.7%, 629 bytes)
// DOES reproduce the original's frame: `sub esp,0x18`, `this` spilled to a slot,
// and slots copied=0x14, r=0x18, count=0x1c, i=0x10 all matching the original's
// 0x14/0x18/0x1c and 0x10. Two things are left, and only one is structural:
//   (a) EBP holds `len` where the original holds `i`; i is memory-resident at 0x10.
//   (b) this/pc take slots 0x24/0x20 where the original takes 0x20/0x24.
// Ladder in v1: esi=this, edi=scratch, ebx=p, ebp=len, memory={i,r,n,m,q,this}.
// Ladder in the original: esi=count/scratch, edi=scratch, ebx=p, ebp=i,
// memory={len,r,n,m,q,this}. So the whole function is ONE allocator decision
// (does `i` outrank `len` for EBP?) and I could not move it. Levers tried today,
// all byte-identical to v1 or v0, i.e. MSVC5 ignores them here:
//   declaration order of p/i/n/m/q/len (12 orders, a1..b3: all 60.7, all 629 bytes,
//     so the ladder is NOT declaration-order driven);
//   `int i;` with no initialiser (f1, 60.7); one `int i` per block (e3, 59.8);
//   `register int i` (e8, 60.7); two live copies of count, guard on one and loop
//     bound on the other (f7, f8, 60.7); m moved into the second block (f6, 60.0);
//   `&len` taken into a local pointer (e1, 60.7) and len reached through a
//     one-field local struct plus a reference (h1, 60.7) or i likewise (h2, 60.7):
//     len still lands in ebp, so scalar replacement puts it back;
//   from the v0 (69.5) side, adding one more dead local (z/y/u/e/w: c6..c10) leaves
//     the code byte-identical, so dead locals are removed before the allocator and
//     are NOT a pressure lever; only `n` moves it (59.4%), and downwards.
// Conclusion for the next attempt: the lever has to change len's or i's LIVE RANGE,
// not its type, count or order. Nothing in the source above does that.
// GPT-6 retry: retained 69.5%. Whole-formatter wrappers and per-loop
// append helpers did not improve allocation; original spill pattern remains.
// NOT A MATCH: 69.5% best (v18). Correct shapes found this run, allocation still open.
//
// deepseek-v4.1-flash (second run): re-derived from the disassembly and re-tested the
// pressure hypothesis. Confirmed the original frame is len=0x10, m=0x14, r/q=0x18,
// n/s=0x1c, this=0x20, pc=0x24 with ONLY p (ebx) and i (ebp) in registers; every value
// used only at a loop boundary (this, len, n, m, pc) stays memory-resident. Declaring
// n/m/q0 as top locals (v1: 59.8%, 630 bytes) makes MSVC promote len into ebp and this
// into esi and gives i edi plus a slot, i.e. it does the exact opposite. Same result for
// a different declaration order (v2 59.8), all-locals-at-function-scope (v5 60.6), a
// free __fastcall copy (v3, identical to thiscall, 69.5) and /Gr /Gz /Gd flags (no
// change for a member fn). Declaring only q0 on top of the 69.5 body collapses to 33.8
// (v7), matching the earlier note. So the missing lever is not "declare n/m/pc": it is
// making i win ebp over len/this while the boundary-only locals stay spilled.
//
// deepseek-v4.1-flash (first run): re-derived the whole function from the disassembly and
// swept ~60 source shapes. The 69.5% body below is still the best. What I confirmed:
// - The original caches BOTH counts in stack slots: n=count at [esp+0x1c], m=copied at
//   [esp+0x14], and pc at [esp+0x24]; this is spilled at [esp+0x20] and i lives in ebp.
//   Frame is 6 dwords (sub esp,0x18); ours is 4 (sub esp,0x10) with this in ebp.
// - Declaring `int n = count;` (or n+m, or n+m+q) drops the score to 61.1 and shrinks the
//   code to 629 bytes: the extra local makes MSVC give len ebp and this esi, it does NOT
//   reproduce the original's spill. Direct member reads for count (below) stay at 69.5.
// - tools/headers.py: 128 sets tried, all 69.5 (and 72 failed to compile), so no header fix.
// - N-declarations sweep (0..400 extern int dummies) is FLAT at 69.5, so this is not
//   compiler state; the source shape or the allocator's choice is what differs.
// - Declaring r/s/q at function scope, shared vs per-loop i, int vs unsigned len, self
//   pointer, inverted if/else, while loops, m via a reference: all <= 69.5.
// Remaining single cause (per the brief's lesson): this must lose ebp to i, and len/n/m/pc
// must all stay memory-resident. Every construct I found that lowers this's register weight
// also promotes len into ebp; the two are one decision I could not split from the source.
//
// CERTAIN (byte-exact regions in v18, verified against disassembly):
// - The grouped-bit conditions are SIGNED `i % 8`, not abs(): `i % 8` compiles to
//   exactly cdq/xor/sub/and/xor/sub with ONE cdq (see build/scratch/0x4d9ca0/abstest2.cpp,
//   g1), while abs(abs(i)&7) emits TWO cdqs. Original has one cdq in both loops.
// - `len` is unsigned (original breaks with jbe on `len <= 0x1e`; signed gives jle).
// - Loop 2 advances both walkers by one element (`q++; s++;`): the latch shares one
//   `mov eax,4` for both adds. (`q += 4` would emit add 16.)
// - `m = copied` up front as a named local helps (+4 over direct reads); the loop 2
//   bound and last-line check then read the slot exactly like the original.
// - Loop bodies use walking pointers (r/q/s) with per-iteration slot reloads;
//   `len -= strlen(p); p += strlen(p);` in that order with two strlen calls each.
// - Separator default " " is loaded before the modulo (ternary materialization).
//
// STILL WRONG (the whole remaining diff is one register-allocation decision):
// - Original keeps `this` memory-resident (spilled to [esp+0x20] in the prologue,
//   reloaded for the walker reseeds) with i in ebp, a count temp in esi, p in ebx,
//   six stack slots (sub esp,0x18). Mine keeps `this` in ebp, i in esi+memory slot.
// - Tried: up-front n (count) hurts (62.2), up-front q0 (pc) collapses (33.8),
//   up-front n+m (61.1), base=ret pointer uniformly 60.7 with len taking ebp,
//   shared single `int i` across loops (+0.9), decl-order shuffle (no change),
//   __inline RetPtr/StackPtr accessors (no change).
// - Hypothesis for next attempt: the prologue's copied/pc/count loads may be
//   MSVC-hoisted invariant loads (m succeeds as a real local but n/q0 do not),
//   so forcing them all into locals is the wrong model; the spill of `this`
//   likely needs a different pressure lever (single shared upstream cause per
//   brief lesson: the ebp/esi swap and the missing spill are one decision).
//
// deepseek-v4.1-flash (fourth run, 10-minute box): retained 69.5%; nothing new matched.
// Re-derived the prologue and confirmed the target allocation again: original has
// p=ebx and i=ebp only, with this=slot 0x20, len=slot 0x10, count=slot 0x1c,
// copied=slot 0x14, pc=slot 0x24; ours keeps this=ebp and i in memory. Tested this
// run, all strictly worse or byte-identical to the 69.5 body:
//   - declaring all locals uninitialised at function scope in two orders (u1, u2:
//     59.4 each, 629 bytes), so the "uninitialised locals take callee-saved
//     registers in declaration order" lever does not reach the i/this pair here;
//   - `register int i` (69.5, byte-identical), an outer `for(;;) {...; break;}`
//     wrapper (59.8), two block-scoped loop counters i1/i2 (60.7), a foldable
//     `int ix = i; i = ix;` use inside each loop (69.5, folded away);
//   - caching count as an unsigned local (vD 59.4), caching only count and keeping
//     copied direct (59.4), unsigned m / unsigned long len types (both 69.5).
// The n-declaration variants keep landing frame 0x18 with this=esi, len=ebp and
// i=edi, i.e. MSVC ranks this/len above i; the original ranks i above both and
// spills them. No source lever found in the box.
#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

class Class_004d9ca0 {
public:
    unsigned long ret[0x1e];
    int count;
    int stack[0x800];
    int copied;
    unsigned long* pc;
    char buf[0xa44c];

    void FUN_004d9ca0();
};

static inline unsigned long inl0(unsigned long*q0, int i) { return q0[i]; }

static inline int inl1(int m) { return (int)(m > 0); }

static inline char* inl5(char*p) { return (char*)p; }

// FUNCTION: 0x4d9ca0
void Class_004d9ca0::FUN_004d9ca0()
{
    unsigned long* q0;
    char* p;
    unsigned int len2;
    int i;
    unsigned int len;
    int m = copied;
    q0 = pc;
    const int n = count;
    len = 0xa44c;
    p = buf;
    i = 0;
    if (n > 0) {
        unsigned long* r;
        sprintf((char*)p, "Call stack:\n");
        len -= strlen(p);
        p = strlen(p) + (p);
        r = ret;
        for (; i < n; ) {
            if (len > 0x1e) {
                sprintf(p, "%08lX", *r);
                int m8 = i % 8;
                strcat(p, (i == n - 1 || m8 == 7) ? "\n" : " ");
                len -= strlen(p);
                ++i;
                p = strlen(p) + p;
                    r = 1 + r;
            } else break;
        }
    } else p[0] = 0;
    len2 = len;
    if (m > 0 && len2 > 0x1e) {
    } else goto skip1;
                        sprintf(p, "Stack dump:\n");
                        len2 = len2 - strlen(p);
                        p += strlen(((char*)p));
                        i = 0;
                        while ((i) < m) {
                            if (len2 > 0x1e) {
                                if (i % 8 == 0) {
                                    sprintf(p, "%08lX: ", (unsigned long)(q0 + i));
                                    len2 = len2 - strlen(p);
                                    p += strlen(p);
                                }
                                sprintf(p, "%08lX", (unsigned long)this->stack[i]);
                                strcat((p), (((int)i) == ((int)m) - 1 || (((int)i) % 8) == 7) ? "\n" : " ");
                                len2 = len2 - strlen(((char*)p));
                                p = p + strlen(p);
                                i = 1 + i;
                            } else break;
                        }
skip1:;
}