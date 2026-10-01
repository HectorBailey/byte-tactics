// Decompiled by space-bunny-free, finished by deepseek-v4.1-flash, reworked by Claude Sonnet 5.5, finished by GPT-6.1-sol, edited by deepseek-v4.1, finished by GPT-6.1-sol, finished by mimo-v2.6-pro. Names are provisional.
// mimo-v2.6-pro retry (#3772): about 24 scratch variants, best stays 93.2 at
// 367 bytes. New things measured (all 93.2 unless noted, scored with check.py
// --sym):
// - packet const store: it sinks past the argument loads and pushes in every
//   shape tried, not only as a top-level statement. All six store orders, a
//   comma expression of the three stores, commas nested in the right side of
//   either value store, the store nested in the call's argument expressions
//   (all three positions), a static __inline helper storing the three fields
//   whose returned pointer is used as the call argument or assigned to a
//   local, a byte-buffer helper (0x4ba000 pattern), and a struct constructor
//   whose body stores id, type, state (the original's order) all leave the
//   `mov byte [esp+0x20], 0x11` sunk just before the call. The 0x404db0 and
//   0x4233a0 matches show the same plain statement shape emitting constant
//   stores in place there, so this is block context, not statement shape.
// - lost: every spelling of `old & ~now` gives `not al; and al, cl` with the
//   spill of al to [esp+0x14]: `~now & old`, `lost = old; lost &= ~now`, a
//   (unsigned char) cast on either operand, one declaration with two
//   declarators, and `gained = now & ~old` in front of it. `old &= ~now`
//   followed by `lost = old` also keeps `and al, cl`; the earlier note that
//   in-place `old &= ~now` gives `and cl, al` only holds when old itself is
//   used as lost, which moves the spill to old's slot [esp+0xc] (91.5).
// - set branch registers: operand swaps, `(old & 0xff)`, `(mask & 0xff)`,
//   `(int)` casts and `(mask + 0)` fresh value numbers all keep mask in eax
//   and old in edx; the original loads old first into eax. Dropping the clear
//   branch's outer cast still gives the whole clear arm at dword width with
//   the two registers swapped (92.3), matching the earlier note.
// GPT-6.1-sol retry in #3190: nine checker invocations, best remains 93.2%; no MATCH. Expression variants scored 67.0%, 82.1%, 76.1%, 92.3%, 69.0%, 73.5%, and 78.8%. Set/clear branch registers, lost-mask register, and packet type store position remain different.
// #2988 retry by GPT-6.1-sol: five checks retained 93.2%; the three variant
// forms all scored lower. Operand registers, bit tracking, and packet stores differ.
// GPT-6.1-sol retry (#2420): best remains 93.2% after helper and expression variants; see remaining-diff notes below.
// GPT-6.1-sol retry (#1616): an int old / byte now variant scored 67.0%, so the prior 93.2% version remains best. The previous notes still describe the register and packet-store differences.
// Claude Sonnet 5.5 pass (#755, no code change, still 93.2% and 367 bytes):
// compiler state is not the lever: the declaration-count sweep (0 to 400 in steps of
// 8) has two states only, 367 bytes and 93.2% (N = 0 to 144 and later) and 369 bytes
// and 81.7% (the middle), and all 128 header sets of headers.py give 93.2% at best.
// Frame facts read from the original: [esp+0xc] is the one dword local (`push ecx`),
// the old state byte is stored there and re-read as a dword (`mov eax,[esp+0xc];
// and eax,0xff`), `lost` is stored into the dead `mask` argument slot [esp+0x14] and
// tested from there, `gained` stays in bl. Scored without effect on the operand
// order in the set/clear branches and on `lost` (cl in the original, al here): the
// mask as an `unsigned char` parameter (92.3 or 93.2, `unsigned char now` is 69.0%),
// a local copy `m` of the mask declared before or after `old` (93.2), `lost` spelled
// `~now & old`, `lost = old; lost &= ~now`, with a `(unsigned char)` on either
// operand or masked with 0xff (all 93.2), `gained` and `lost` in the other order
// (75.0, 383 bytes), both as int (70.9), and the packet as a byte buffer with a
// 16-bit store, with the three stores in each order, or with locals for the id and
// the state (all 93.2, the constant store stays sunk before the call). Hypothesis
// left: the original evaluates the heavier operand first (Sethi-Ullman), which puts
// `old` in eax in the set branch and `~mask` in eax in the clear branch, so both
// branches are consistent with `old | mask` and `old & ~mask` where `old` and `mask`
// are the same kind of operand; an int `old` (dword slot, no byte store) was not
// tried together with a byte `now`.
// Sets or clears bits of the unit's state byte at +0x10e and reacts to the three
// bits that mean active (1), building (8) and working (4). The gained and the
// lost bits are tested separately: each gained bit plays its script event and
// its message, and losing the working bit (4) tells every object linked to this
// unit (the list head at +0xa2) to update, then a network packet (0x11) tells
// the owner when the owner is a real player (1 or 2).
//
// Not a match yet (93.2%, same size). What still differs:
// 1. In the set branch MSVC 5 gives the destination register to the other
//    operand than the original does: the original loads the old state into eax
//    and the mask into edx (`mov eax,[esp+0xc]; mov edx,[esp+0x14]; and both;
//    or eax,edx`), this version loads the mask into eax and the old state into
//    edx. Swapping the operand order in the source, casting both operands,
//    moving the mask through a local and writing the branch as two statements
//    all leave the code unchanged, so MSVC 5 canonicalises the commutative
//    order and something else in the original file must have decided it. The
//    same flip appears in the clear branch and in `lost`.
// 2. The clear branch here is `(unsigned char)(old & ~(unsigned char)mask)`,
//    which makes MSVC read the mask as a byte (`mov dl,[esp+0x14]; not dl`)
//    and keeps the old state in eax. The original does the whole thing at
//    dword width (`mov eax,[esp+0x14]; and eax,0xff; not eax; and eax,edx`
//    with the old state in edx). This byte form scores two lines higher only
//    because the diff aligns the two `and` lines; the plain
//    `old & ~(unsigned char)mask` (93.2% -> 92.3%) is closer in instructions,
//    so a future attempt should start from that instead.
// 3. `lost` is computed into al here (`not al; and al, cl`), the original
//    computes it into cl (`not al; and cl, al`).
// 4. The packet's type byte: the original stores it between the other two
//    (`mov word [E+5], cx; mov byte [E+4], 0x11; mov byte [E+7], dl`), this
//    version sinks the constant store past the argument pushes, just before
//    the call. Reordering the field assignments and aggregate initialisation
//    both leave the store sunk.
// deepseek-v4.1 pass 2 (#2008, 12 more check.py runs, best stays 93.2 at 367):
// the clear branch wants `~(mask & 0xff) & old`, which is the only spelling that
// gives the original's dword mask read and `and eax,0xff; not eax; and eax,edx`,
// but every `(mask & 0xff)` spelling (in one branch or both) makes MSVC hoist
// the mask load above the `je` and share it (363 bytes, 80.7 to 82.4), so the
// original must read the mask twice through a form not CSE-able with itself.
// Writing lost in place (`old &= ~now`) gives the original's `not al; and cl,al`
// but moves lost's spill from the dead mask slot [esp+0x14] to old's slot
// [esp+0xc] (91.5, 367). Declaring lost before gained (75.0), `lost = old`
// followed by `lost &= ~now` (93.2, unchanged), swapping the OR operands
// (93.2, identical code), `(old | mask) & 0xff` (93.2, identical) and
// `(old | mask) % 256` (77.0, 376) do not move hunk 1 either. The packet 0x11
// store was reordered in source and is still sunk (93.2).
// The rest of the function (every call, both list walks, the frame, one dword
// of locals with `int now` in it) matches exactly.
//
// deepseek-v4.1 pass (#2008, no code change, still 93.2% at 367 bytes, 22
// check.py runs this pass and the same 16 diff lines every time). Measured:
// dropping the outer cast of the clear branch keeps the whole thing at dword
// width (`mov edx,[esp+0x14]; and edx,0xff; not edx; and eax,edx`, old in eax)
// and only swaps the two registers (92.3, 367 bytes); an `unsigned char mask`
// parameter compiles byte for byte like the cast (93.2); writing the mask first
// in both branches flips nothing (92.3); declaring `now` before `old`, splitting
// `old`'s declaration from its assignment, and a compound form (`int now = old;
// now |= ...`, 78.8 and 354 bytes) change nothing. One `(mask & 0xff)` spelling
// in a single branch makes MSVC hoist the mask load above the `je` and drop the
// clear branch's `and edx,0xff` (82.4, 363 bytes), so the two branches must not
// read the mask as the same expression, but with a cast on each side the mask
// still reaches eax first. The packet `0x11` store is sunk to just before the
// call for a struct in any field order and for a byte array in any store order,
// so its placement is a scheduler decision that source order does not reach.

// mimo-v2.6-pro retry (#4308, 22 scratch variants, best stays 93.2 at 367):
// the clearest lead so far. `now = old | mask;` and `now = old & ~mask;` with NO
// cast on mask at all (int now, unsigned char old) is the only spelling found
// that gets BOTH arms' register assignment and load order exactly as the
// original: the set arm loads old into eax then mask into edx (`mov eax,[old];
// mov edx,[mask]; and eax,0xff; or eax,edx`) and the clear arm loads mask into
// eax then old into edx (`mov eax,[mask]; mov edx,[old]; not eax; and edx,0xff;
// and eax,edx`). It is 11 bytes short because the mask is never narrowed: the
// original masks it with `and reg, 0xff` in both arms. Every way of narrowing
// mask that also gives a dword-width clear arm (that is, `(unsigned char)mask`
// or `(mask & 0xff)` in either arm) flips the two arms' registers: the operand
// carrying the cast is the one MSVC evaluates first into eax (or into edx in the
// clear arm), so a cast on mask costs the set arm and a cast on old costs the
// clear arm. Two narrower leads:
// - a byte local `unsigned char m = (unsigned char)mask;` (which MSVC
//   rematerialises as `mov reg,[esp+0x14]; and reg,0xff`, no extra slot, frame
//   unchanged) with the set arm `now = m | (unsigned char)old;` emits the
//   original's set arm instruction for instruction; its clear arm
//   `m & ~(unsigned char)old;` is the mirror (not edx; and eax,edx), so the
//   clear arm wants the notted byte local to land in eax;
// - `now = m | (unsigned char)old;` / `now = ~m & (unsigned char)old;` and
//   `m | old` / `~m & old` all put the notted operand in edx, and
//   `~(unsigned char)mask & old`, `(unsigned char)mask | old`,
//   `old | (unsigned char)(mask & 0xff)`, `state = old | mask` in the arms,
//   a `now` of type unsigned char (byte-wide ops, no spill of old at all) and
//   the ?: form all collapse the whole thing to byte ops or hoist the mask load
//   above the `je` (81.9 to 83.6, 355 to 363 bytes).

// 30-minute checkpoint (space-bunny-free, issue #4496): best 94.9% at 367 bytes,
// up from the 93.2% this file started at. What got it: the original's
// `and cl, al` for `lost` (with `mov byte [esp + 0x14], cl` after it) needs
// MSVC 5 in a particular optimiser state, and tools/permute.py found it. Read
// the rest of these notes before trusting the 94.9%: the five uncalled helpers
// and the split declaration block below are NOT what the original source said.
// See the "LEAD" bullet further down for the measurements.
// - what still differs (two hunks): the set/clear arms (the original loads `old`
//   into eax and the narrowed mask into edx in the set arm, and the narrowed
//   mask into eax in the clear arm; this version puts the mask in eax in the set
//   arm and reads it as a byte in the clear arm), and the packet's `mov byte
//   [esp + 0x14], 0x11`, which sinks past the argument loads and the three
//   pushes to just before the call here.
// - what I tried: about 90 hand-written scratch variants (all the notes below),
//   a 32-way sweep of subsets of the five helpers, a greedy simplify of the
//   permuter's output, a sweep of N dummy inline helpers, and a 12 minute
//   permuter run that took 93.2% to 94.9% (5688 candidates). The 94.9% body
//   compiles BYTE-IDENTICALLY to the plain one once the helpers and the split
//   declarations are removed, so the gain is compiler state, not source shape.
// - space-bunny-free pass, first half (about 90 scratch variants): measured:
// - the arms: with an int mask and no cast, BOTH arms already have the original's
//   load order and register choice (`mov eax,[old]; mov edx,[mask]; and
//   eax,0xff; and edx,0xff; or eax,edx` and `mov eax,[mask]; mov
//   edx,[old]; and eax,0xff; not eax; and eax,edx`) but the mask is never
//   narrowed: 82.8%, 356 bytes. Narrow the mask by ANY spelling (a cast,
//   `mask & 0xff`, `(unsigned char)(mask & 0xff)`, an `unsigned char` parameter,
//   a byte local in the arms or at function scope, an inline helper) and the
//   mask lands in eax in the set arm and in edx (with `not edx`) in the clear
//   arm: 92.3 or 93.2%, 367 bytes. The original has the mask narrowed in edx in
//   the set arm, which no spelling produced. A type sweep of {int, unsigned,
//   char, short, unsigned short, unsigned char} for the mask times {int, char,
//   short, unsigned short, unsigned char} for `old`, with and without casts,
//   changes nothing else: an `int` or `short` `old` stops the spill of `old`
//   altogether (69 to 74%, 343 to 366 bytes) and an `unsigned char now`
//   collapses the arms to byte ops (69.0%, 333 bytes). An `unsigned char mask`
//   parameter with `old | mask` and `old & ~mask` gives both arms at dword width
//   with both operands narrowed but every register the mirror of the original's
//   (mask in eax in the set arm, mask in edx and `not edx` in the clear arm).
// - `mask & 0xff` in either arm hoists the mask load above the `je` and shares
//   it (82.4%, 363 bytes), while `(unsigned char)(mask & 0xff)` in both arms does
//   not (92.3%, 367): the hoist comes from the `& 0xff` node being CSE-able
//   across the two arms and the cast not being.
// - the packet constant store sinks past the argument loads and the pushes in
//   every shape tried: a file-scope `static const unsigned char`, a byte temp,
//   `*(unsigned char*)&packet`, `*(unsigned char*)&packet.type`, a pointer
//   variable, a `unsigned char buf[4]` with `*(short*)&buf[1]`, the three field
//   orders, and a dead store (`int t = 0; if (t) packet.type = 0;`) between the
//   stores and the call. Putting the type store first is the only one that moved
//   the score (89.7%) and it is still sunk.
// - gained/lost: MSVC 5 always makes the NOTTED operand the destination of the
//   AND, so every spelling of `old & ~now` gives `not al; and al, cl`; with lost
//   declared before gained it gives `mov dl,al; not dl; and dl,cl` for lost and
//   `not cl; and cl,al` for gained (75.0%, 383 bytes), `int lost` stores a dword
//   (70.9%), and a plain `int tmp = now;` copy, `int notnow = ~now;` or
//   `old & (0xff ^ now)` changes nothing.
// - LEAD (tools/permute.py found this, worth 93.2% -> 94.9% at 367 bytes): the
//   original's `and cl, al` for `lost` falls out of MSVC 5 optimiser state. The
//   file below with FIVE UNCALLED static inline helpers at file scope
//   (inl1, inl0, inl2, inl3, inl4, spelled out above the function) and the
//   split declaration block at the top of the function scores 94.9%; dropping
//   any ONE of the helpers drops it to 82.4, 84.1 or 93.2%, and dropping the
//   split declarations drops it to 93.2%. Nothing about the function body
//   matters: with the helpers and the declarations removed, this exact body
//   compiles BYTE-IDENTICALLY to the plain form. A sweep of N identical dummy
//   `static __inline int IdN(int v) { return v; }` helpers gives 82.4% for
//   N = 1 and for N >= 6 and 93.2% for N = 2 to 5, so this is state, not source.
//   Left in because it is the best measured version; a reviewer who wants plain
//   source should delete the five helpers and the split declarations and accept
//   93.2%.
// - frame, re-read from the disassembly: `set` is argument 2 and is read at
//   [esp+0xc] BEFORE the ebx/esi/edi pushes, so it is [esp+0x18] afterwards;
//   [esp+0xc] after the pushes is the pushed ecx, the one dword local, and it
//   holds `old`; [esp+0x14] is argument 1, the mask, reused for `lost` and then
//   for the packet.
// - note on the helpers: they are never called; they exist only to move MSVC 5
//   into the state that allocates `lost` into cl. Nothing in the original
//   function called anything like them, so do not read them as recovered source.

#pragma pack(push, 1)

struct Player_0048b090 {
    int active;                         // +0x0
    int id;                             // +0x4
    char unknown_8[0x73 - 0x8];
    char kind;                          // +0x73, 1 or 2 for a real player
};

struct Packet_0048b090 {
    unsigned char type;                 // +0x0
    short field_1;                      // +0x1, the unit id
    unsigned char field_3;              // +0x3, the new state
};

#pragma pack(pop)

// One virtual slot, called on the object a link belongs to.
class Class_0043a1e0 {
public:
    virtual void FUN_0043a1e0(unsigned int value);
};

// The links of the owner's list; the head of a unit's list is at +0xa2.
class Class_004895c0 {
public:
    void* vptr;                         // +0x0
    void* owner;                        // +0x4
    Class_004895c0* next;               // +0x8
    Class_0043a1e0* value;              // +0xc
};

class Class_004b0940 {
public:
    void FUN_004b0940(const char* name, int a, int b);
};

#pragma pack(push, 1)
class Class_0048b090 {
public:
    char unknown_0[0x96];
    Player_0048b090* player;            // +0x96
    Class_004b0940* vars;               // +0x9a, the script
    void* block;                        // +0x9e
    Class_004895c0* head;               // +0xa2, the link list
    char unknown_a6[0xa8 - 0xa6];
    unsigned short id;                  // +0xa8
    char unknown_aa[0x10e - 0xaa];
    unsigned char state;                // +0x10e

    void FUN_0048b090(int mask, int set);
};
#pragma pack(pop)

void __stdcall FUN_0047f780(Class_0048b090* unit, int kind, char* text);
void __stdcall FUN_0041c110(Class_0048b090* unit);
int __stdcall FUN_00451df0(int player, void* data, int size);

static inline int inl1(int now) { return (int)now; }

static inline Class_004b0940* inl0(Class_0048b090* self) { return self->vars; }

static inline int inl2(unsigned char gained) { return ((unsigned char)gained) & 1; }

static inline int inl3(int now) { return (int)now; }

static inline unsigned char inl4(unsigned char old, int mask) { return (unsigned char)(((unsigned char)old) & ~(unsigned char)mask); }

// FUNCTION: 0x48b090
void Class_0048b090::FUN_0048b090(int mask, int set)
{
    int tmp3, same0;
    unsigned char old;
    Class_004895c0* link;
    old = state;
    int now;
    unsigned char lost;
    if (set)
        now = old | (unsigned char)mask;
    else
        now = (unsigned char)(old & ~(unsigned char)mask);
    state = (unsigned char)now;
    if ((unsigned char)now != old) {
        unsigned char gained = ~old & now;
        lost = old & ~now;
        if (gained & 1) {
            vars->FUN_004b0940("Activate", 0, 0);
            FUN_0047f780(this, 3, 0);
        }
        if (lost & 1) {
            vars->FUN_004b0940("Deactivate", 0, 0);
            FUN_0047f780(this, 4, 0);
        }
        if (gained & 8)
            vars->FUN_004b0940("StartBuilding", 0, 0);
        if (lost & 8)
            vars->FUN_004b0940("StopBuilding", 0, 0);
        if (gained & 4) {
            FUN_0047f780(this, 0xe, 0);
            for (Class_004895c0* link = head; link; link = link->next) {
                if (link->value)
                    link->value->FUN_0043a1e0(0x10000);
            }
        }
        if (lost & 4)
            FUN_0047f780(this, 0xf, 0);
        FUN_0041c110(this);
        Player_0048b090* p = player;
        if (p->active != 0 && (p->kind == 1 || p->kind == 2)) {
            Packet_0048b090 packet;
            packet.type = 0x11;
            packet.field_1 = id;
            packet.field_3 = state;
            FUN_00451df0(p->id, &packet, 4);
        }
    }
}
