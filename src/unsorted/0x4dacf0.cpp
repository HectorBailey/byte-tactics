// Decompiled by deepseek-v4.1, finished by deepseek-v4.1-flash and mimo-v2.6-pro. Names are provisional.
// mimo-v2.6-pro retry (best 55.6%, 777 of 780 bytes): adopted int wraps (the original's
// `cmp esi, 2 / jge` is signed, ours had unsigned `jae`) and unsigned map->count (the
// `count <= wraps` compare is unsigned `jbe`), and swapped the split-path statements to
// `len = cur.ptr->length; key = cur.ptr->key;` which reproduces the original's
// `mov ebp,[eax+0x10]` before `mov esi,[eax+0xc]` emission order. Score flat at 55.6.
// The one residual is still the register rotation caused by the const-0 register (ebp):
// the original materialises NO zero register at all (res=0 is an immediate `mov [esp+0x24],0`,
// the size fixup is `test esi,esi`, and the later zero sites use the just-zeroed esi (wraps)
// or immediate forms), while every spelling tried makes MSVC hoist `xor ebp,ebp` to the size
// compare and fold every zero use into ebp. Bisect findings this run (asm in
// build/scratch/0x4dacf0/): with the for-loop removed (v8) or the lower_bound prelude removed
// (v9) the const register disappears entirely and `q.length = 0` compiles to the wraps
// register exactly like the original, so the trigger is the COUNT of register-needing zero
// uses (res store, q.length, dbe10 push, DAT_005289d4=0, dbd80 push, record push): 4 uses
// give no const register, 6 do. Removing res=0 alone (43.3%) does not remove the register,
// and `map->count <= 0` is byte-identical to `<= wraps` (board #3738). Once the zero register
// exists, want is pushed out of ebp to ebx and need spills to memory, and every later
// difference (cmp vs test, push ebp vs push 0, len/key register homes, the b iterator slot,
// the lea ecx vs eax in the pad branch) is that one allocator decision cascading, so this
// looks like the same allocator state class as 0x4a6ae0, 0x4866d0 and 0x4db1c0 (board: the
// callee-saved tie-break is not reachable from ordinary source shapes).
// Earlier attempts (deepseek-v4.1-flash and GPT-6, all scored): res-zero placement is not the
// lever (55.6 flat, first-local gives 51.5), declaring wraps early drops to 42.5 (frame moves),
// spelling the dbe10 argument or q.length as wraps is byte-flat, a dummy-declaration sweep of
// N = 0..400 flips only between 55.6 and 46.8, headers.py (128 sets) and the per-declaration
// permutations of res/need/want are all flat. Pair_004dacf0 must have no zeroing constructor,
// FUN_004da780 returns the CRITICAL_SECTION pointer and the size argument is not written back
// (`unsigned int size = n; if (size == 0) size = 1;`), and the head test is a value compare
// through operator== (sete/test). An inline Pair constructor for the give-back call scores
// 55.2 (worse), so the p stores before FUN_004db000 stay.
// Carve a block out of the reservation allocator: enter the lock, round the request up (need)
// and double it (want), then walk the free-block map for a block of at least want bytes,
// splitting it around the allocation point and committing the pages with VirtualAlloc.
// Same std::map idiom as 0x4db000, 0x4db450 and 0x4db1c0: the block is the map's value_type,
// a base and a length.

#include <windows.h>
#include <memory.h>

extern unsigned int DAT_005289d4;
extern unsigned int DAT_00528a00;
extern unsigned int DAT_005289f0;
extern unsigned int DAT_005289d0;
extern unsigned int DAT_00528a04;

struct Node_004dacf0 {
    Node_004dacf0* left;   // +0x0
    Node_004dacf0* parent; // +0x4
    Node_004dacf0* right;  // +0x8
    unsigned int key;      // +0xc
    int length;            // +0x10
    int color;             // +0x14
};

class Class_004dbe10 {
  public:
    Node_004dacf0* ptr;

    Class_004dbe10() {}
    Class_004dbe10(Node_004dacf0* q) : ptr(q) {}
    bool operator==(const Class_004dbe10& o) const { return ptr == o.ptr; }
    Class_004dbe10* FUN_004dbe10(Class_004dbe10*, int);
    Class_004dbe10* FUN_004dbd80(Class_004dbe10*, int);

    // The original tests the iterators as a value (sete; neg; sbb; inc; test),
    // which MSVC 5 only does for a `!` applied to a bool-returning member.
    bool Neq(Class_004dbe10 a, Class_004dbe10 b) { return !(a == b); }
};

struct Pair_004dacf0 {
    unsigned int offset; // +0x0
    int length;          // +0x4
};

struct Ins_004dacf0 {
    Node_004dacf0* ptr;
    unsigned char inserted;
};

class Class_004db610 {
  public:
    char unknown_0[4];
    Node_004dacf0* head; // +0x4
    char unknown_8[4];
    unsigned int count; // +0xc
    int total; // +0x10
    char unknown_14[20];

    void FUN_004dbbc0(Ins_004dacf0* out, Pair_004dacf0* v);
};

class Class_004db450 {
  public:
    bool FUN_004db450(unsigned int);
};
class Class_004db000 {
  public:
    Node_004dacf0* FUN_004db1c0(unsigned int);
    void FUN_004db000(Pair_004dacf0);
};
class Class_004dbd00 {
  public:
    Class_004dbe10 FUN_004dbd00(Class_004dbe10);
};
class Class_004dbeb0 {
  public:
    Class_004dbe10* FUN_004dbeb0(Class_004dbe10*);
};
class Class_004dc620 {
  public:
    Class_004dbe10* FUN_004dc620(Class_004dbe10*, Pair_004dacf0*);
};

class Class_004d8820 {
  public:
    unsigned int base;    // +0x0
    unsigned int size;    // +0x4
    unsigned int count;   // +0x8
    char unknown_c[0x20]; // +0xc
    unsigned int tag;     // +0x2c

    Class_004d8820(unsigned int a, unsigned int b, unsigned int c, unsigned int d, const char* e);
};

class Class_004dc680 {
  public:
    void FUN_004dc680(Ins_004dacf0* out, Class_004d8820* v);
};

class CritSec_004da780 {
  public:
    CRITICAL_SECTION cs;
};

CritSec_004da780* FUN_004da780();
unsigned int __cdecl FUN_004da8a0(unsigned int size);
unsigned int __cdecl FUN_004da8c0(unsigned int size);
Class_004db610* FUN_004db610();
Class_004dc680* FUN_004da8d0();
void __cdecl FUN_004da7d0(unsigned int size);
char FUN_004db760();
int FUN_004db7c0();
void __cdecl FUN_004d82c0(void* at, int value, unsigned int count);

// FUNCTION: 0x4dacf0
unsigned int __cdecl FUN_004dacf0(unsigned int n, unsigned int arg2) {
    CritSec_004da780* lock = FUN_004da780();
    EnterCriticalSection(&lock->cs);
    unsigned int size = n;
    if (size == 0)
        size = 1;
    unsigned int need = FUN_004da8c0(size);
    unsigned int want = FUN_004da8a0(size);
    if (want < need) {
        LeaveCriticalSection(&lock->cs);
        return 0;
    }

    Class_004db610* map = FUN_004db610();
    int wraps = 0;
    unsigned int base;
    unsigned int len;
    unsigned int key;
    Pair_004dacf0 p;
    Pair_004dacf0 q;
    Ins_004dacf0 ins;
    Class_004dbe10 n2;
    Class_004dbe10 b;
    Class_004dbe10 cur;
    unsigned int res = 0;

    if (map->count <= wraps)
        goto alloc_new;

    q.offset = DAT_005289d4;
    q.length = 0;
    ((Class_004dc620*)map)->FUN_004dc620(&n2, &q);
    ((Class_004dbeb0*)map)->FUN_004dbeb0(&b);
    if (n2.Neq(n2, b)) {
        b.ptr = n2.ptr;
        b.FUN_004dbe10((Class_004dbe10*)&p, 0);
        if (DAT_005289d4 >= b.ptr->key && want + DAT_005289d4 <= b.ptr->key + b.ptr->length)
            n2.ptr = b.ptr;
    }
    cur.ptr = n2.ptr;

    for (;;) {
        if (cur == (Class_004dbe10(map->head))) {
            ((Class_004dbeb0*)map)->FUN_004dbeb0(&b);
            cur.ptr = b.ptr;
            DAT_005289d4 = 0;
            DAT_00528a00++;
            wraps++;
        }
        if (cur.ptr->length >= want)
            break;
        cur.FUN_004dbd80((Class_004dbe10*)&p, 0);
        if (wraps >= 2)
            goto alloc_new;
    }

    len = cur.ptr->length;
    key = cur.ptr->key;
    ((Class_004dbd00*)map)->FUN_004dbd00(cur);
    base = DAT_005289d4;
    if (base == 0) {
        base = key;
        DAT_005289d4 = base;
    }
    if (base < key)
        base = key;
    if (base + want > key + len)
        base = key;
    if (base > key) {
        p.offset = key;
        p.length = base - key;
        map->FUN_004dbbc0(&ins, &p);
    }
    if (base + want < key + len) {
        p.offset = base + want;
        p.length = key + len - base - want;
        map->FUN_004dbbc0(&ins, &p);
    }
    DAT_005289d4 = base + want;
    goto commit_block;

alloc_new:
    if (((Class_004db450*)map)->FUN_004db450(want))
        base = (unsigned int)((Class_004db000*)map)->FUN_004db1c0(want);
    else
        base = 0;
commit_block:
    if (base) {
        res = (unsigned int)VirtualAlloc((void*)base, need, MEM_COMMIT, PAGE_READWRITE);
        if (res == 0) {
            p.offset = base;
            p.length = want;
            ((Class_004db000*)FUN_004db610())->FUN_004db000(p);
        }
    }

    if (res == 0) {
        LeaveCriticalSection(&lock->cs);
        return 0;
    }
    unsigned int pad = (0 - (n & 0xfff)) & 0xfff;
    if (FUN_004db760()) {
        FUN_004d82c0((void*)res, FUN_004db7c0(), pad);
        res += pad;
    } else {
        FUN_004d82c0((void*)(res + n), FUN_004db7c0(), pad);
    }
    {
        Class_004d8820 rec(res, n, DAT_00528a04, arg2, 0);
        FUN_004da8d0()->FUN_004dc680(&ins, &rec);
    }
    FUN_004da7d0(n);
    DAT_005289f0 += (n + 0xfff) & 0xfffff000;
    if (DAT_005289f0 > DAT_005289d0)
        DAT_005289d0 = DAT_005289f0;
    LeaveCriticalSection(&lock->cs);
    return res;
}
