// Decompiled by Space Bunny Free, finished by deepseek-v4.1-flash, finished by space-bunny-free, edited by deepseek-v4.1, finished by deepseek-v4.1-flash, finished by mimo-v2.6-pro. Names are provisional.
//
// mimo-v2.6-pro, 2026-10-01: 59.8% (original 920 bytes, ours 908), up from
// 59.0. The lever: the two-step `Vec3 t = f(); Vec3 v; v.x = t.x; v.y = t.y;
// v.z = t.z;` combined with the reference-returning inline MaxRef for the b19
// clamp. Neither works alone (the two-step alone coalesces, the clamp alone
// does not force v into memory): taking the address of v.y in the clamp gives v
// real homes, which materialises the FUN_0043e060 return temporary and turns
// the copy into three real stores, exactly the 6-instruction copy the original
// has. `Vec3 v; v = f();` + MaxRef also materialises the copy (interleaved
// load-store pairs through the returned pointer, like the original) but puts v
// above the temp and scores 56.3; `v = t` (whole-struct copy) scores 59.8.
//
// WHAT STILL DIFFERS (this is what I could not fix):
// a. Frame is 0x24, the original's is 0x28 (one more dword of locals).
// b. The copy is load-load-load-store-store-store (t's fields into edi/ecx/ebx,
//    stores interleaved with the b19 test); the original interleaves
//    load-store pairs through the returned pointer eax (`mov edi,[eax]; mov
//    [esp+0x20],edi` ...) and keeps v.x in edi, v.y in ecx for the clamp.
// c. The clamp spills lim unconditionally before the branch and computes
//    &v.y first (`lea; jg` over the false arm); the original spills lim only
//    inside the false arm (`mov [esp+0x3c],eax; lea eax,[esp+0x3c]`) with
//    `lea eax,[esp+0x24]; jmp` on the true arm. That exact shape is MSVC 5's
//    codegen for a class-type ternary (see 0x46e160's `? End() : p` and
//    0x4dbb40): the true arm is a materialised object and the false arm a
//    register-resident object spilled where its address is needed. All int
//    ternaries I probed (lvalue ternary, reference-returning max, const-ref
//    binding, macro) hoist the true-arm lea before the branch and spill lim
//    eagerly, so the clamp operands are probably 4-byte class objects in the
//    original, not plain ints.
// d. The lim value: the original computes (draft<<16)-draft+seaLevel and then
//    `shl eax,0x10`. Every plain integer spelling of `(x * 0xffff + s) << 16`
//    folds to `(s - x) << 16` in MSVC 5 (the <<16 truncation lets it drop the
//    x*0xffff term). The 0x4853b0 bitfield trick (union {int value; struct
//    {unsigned frac:16; int whole:16} parts;} with frac=0) blocks the fold
//    when the Fixed is returned (probe: `return MakeFixed(x).value;` gives the
//    original's exact shl/sub/add/shl chain) but NOT when the result is used
//    in the clamp compare: there MSVC still folds to (s-d)<<16 and emits
//    `sub edx,ecx; shl edx,0x10`.
// e. u->type is loaded before the copy stores (the original loads it after).

#include <string.h>` changes codegen: 52.2 / 166 / 893.
//   - declaration order alone moves nothing: `Point draft;` declared early and
//     assigned late, or `Vec3 pos;` first, compile byte-identically; inline
//     accessors for the two `mode` reads, `memcpy(&v, &t, sizeof(Vec3))` and
//     three field assignments for the copy are byte-identical too; putting the
//     null path first (source inverted) collapses to 39.1.
//   - making `pp` a real variable fails: `(Vec3*)((char*)this + 8)`, a
//     function-scope `Vec3* pp;` assigned in the null path, `Vec3& pp = p1;`,
//     a fresh `Vec3* qq = &p1;` inside the flags block and `(void)&pp;` are
//     all byte-identical, MSVC rematerialises this+8. Splitting either path
//     into an in-class inline member helper is byte-identical too.
// What is left, in order of size:
//   a. `this` is in edi (original `mov ebp,ecx` at 0x43d6db), and the whole
//      second block follows: original nx=edi, nz=ebx, ours nx=ebx, nz=ebp.
//   b. the first block's copy is load-load-load-store-store-store (inlined user
//      copy ctor) where the original interweaves load-store pairs, reusing edi
//      for t.x straight into the argument build.
//   c. small-local slots: original m@[esp+0x10], draft@[esp+0x14], a dead
//      `&p1` store at [esp+0x18] (dereferenced only for p1.z) and cell in the
//      parameter home slot [esp+0x3c]; ours cell@0x10, m@0x14, draft@0x18 and
//      the ny spill at [esp+0x3c]. Note the original's pp=&p1 is NOT copy
//      propagated and keeps a home, while ours is.

#include <string.h>

#pragma pack(push, 1)

struct Vec3 {
    int x, y, z;
    Vec3() {}
    Vec3(int a, int b, int c) : x(a), y(b), z(c) {}
};

struct Point {
    short x, y;
};

struct Short3 {
    short x, y, z;
};

struct Game_0043d6d0 {
    char unknown_0[0x1427f];
    unsigned char seaLevel;                         // +0x1427f
    char unknown_14280[0x38a47 - 0x14280];
    int field_38a47;                                // +0x38a47
};

struct UnitType_0043d6d0 {
    char unknown_0[0x192];
    int range;                                      // +0x192
    char unknown_196[0x22c - 0x196];
    unsigned char draft;                            // +0x22c
    char unknown_22d[0x241 - 0x22d];
    unsigned int low : 19;                          // +0x241
    unsigned int b19 : 1;
    unsigned int rest : 12;
};

struct Target_0043d6d0 {
    int field_0;                                    // +0x0
    char unknown_4[0x73 - 0x4];
    unsigned char type;                             // +0x73
};

struct TargetData_0043d6d0 {
    char unknown_0[8];
    Vec3 p1;                                        // +0x8
    char unknown_14[0x20 - 0x14];
    int field_20;                                   // +0x20
};

struct Path_0043d6d0 {
    TargetData_0043d6d0* field_0;                   // +0x0
};

struct Unit_0043d6d0 {
    char unknown_0[0x64];
    Short3 f64;                                     // +0x64
    Vec3 pos;                                       // +0x6a
    Point cell;                                     // +0x76
    char unknown_7a[0x7e - 0x7a];
    Point draft;                                    // +0x7e
    char unknown_82[0x86 - 0x82];
    Path_0043d6d0* obj;                             // +0x86
    char unknown_8a[0x92 - 0x8a];
    UnitType_0043d6d0* type;                        // +0x92
    Target_0043d6d0* target;                        // +0x96
    char unknown_9a[0xa8 - 0x9a];
    short a8;                                       // +0xa8
    char unknown_aa[0xf9 - 0xaa];
    signed char index;                              // +0xf9
    char unknown_fa[0x110 - 0xfa];
    unsigned int flags;                             // +0x110
};

#pragma pack(pop)

extern Game_0043d6d0* g_game;

Vec3 __stdcall FUN_0043e060(Path_0043d6d0* obj, int index);
Short3 __stdcall FUN_0043e180(Path_0043d6d0* obj, int index);
void __stdcall FUN_0048a9f0(Unit_0043d6d0* unit, Vec3 pos, int mode);
int __stdcall FUN_0047db70(UnitType_0043d6d0* type, short a8, Point cell, int mode);
void __stdcall FUN_0047d0e0(Unit_0043d6d0* unit);
void __stdcall FUN_0047cc30(Unit_0043d6d0* unit);
void __stdcall FUN_004827b0(Unit_0043d6d0* unit);
int __cdecl FUN_004b70ef(short angle, int scale);
int __cdecl FUN_004b7123(short angle, int scale);

#pragma pack(push, 1)
class Class_0043d6d0 {
public:
    char unknown_0[8];
    Vec3 p1;                                        // +0x8
    char unknown_14[0x20 - 0x14];
    int field_20;                                   // +0x20
    char unknown_24[0x2a - 0x24];
    int field_2a;                                   // +0x2a
    unsigned char mode : 2;                         // +0x2e
    unsigned char flag : 1;                         // bit 2
    unsigned char unknown_2f : 5;

    void FUN_0043d6d0(Unit_0043d6d0* unit);
};
#pragma pack(pop)

static inline int& MaxRef_0043d6d0(int& a, int& b) { return a > b ? a : b; }

// FUNCTION: 0x43d6d0
void Class_0043d6d0::FUN_0043d6d0(Unit_0043d6d0* u)
{
    if (u->obj != 0) {
        Vec3 t = FUN_0043e060(u->obj, u->index);
        Vec3 v;
        v.x = t.x;
        v.y = t.y;
        v.z = t.z;
        if (u->type->b19) {
            int lim = (u->type->draft * 0xffff + g_game->seaLevel) << 16;
            v.y = MaxRef_0043d6d0(v.y, lim);
        }
        FUN_0048a9f0(u, v, mode);
        Short3 o = FUN_0043e180(u->obj, u->index);
        u->f64 = o;
        if (u->obj->field_0 != 0) {
            field_20 = u->obj->field_0->field_20;
            p1 = u->obj->field_0->p1;
        } else {
            field_20 = 0;
            Vec3 zero(0, 0, 0);
            p1 = zero;
        }
        u->flags &= 0xfffeffff;
        return;
    }

    Vec3* pp = &p1;
    int nx = pp->x + u->pos.x;
    int ny = pp->y + u->pos.y;
    int nz = pp->z + u->pos.z;
    int m = mode;
    Vec3 pos;
    pos.x = nx;
    pos.y = ny;
    pos.z = nz;
    if (nx == u->pos.x && nz == u->pos.z && ny == u->pos.y && m == (int)(u->flags & 3))
        return;

    field_2a = g_game->field_38a47;
    Point draft = u->draft;
    Point cell;
    cell.x = (nx - (draft.x << 19) + 0x80000) >> 20;
    cell.y = (nz - (draft.y << 19) + 0x80000) >> 20;
    if (cell.x == u->cell.x && cell.y == u->cell.y && m == (int)(u->flags & 3)) {
        u->pos.x = nx;
        u->pos.y = ny;
        u->pos.z = nz;
        u->flags |= 0x10000;
        return;
    }

    if (u->target->field_0 != 0 && (u->target->type == 1 || u->target->type == 2)) {
        flag = FUN_0047db70(u->type, u->a8, cell, m) == 0;
    }

    if (flag) {
        Point draft2 = u->draft;
        Point c = u->cell;
        int cx = (draft2.x + c.x * 2) << 19;
        int cz = (draft2.y + c.y * 2) << 19;
        if (nx > cx + 0x7ffff)
            pos.x = cx + 0x7ffff;
        else if (nx < cx - 0x7ffff)
            pos.x = cx - 0x7ffff;
        if (nz > cz + 0x7ffff)
            pos.z = cz + 0x7ffff;
        else if (nz < cz - 0x7ffff)
            pos.z = cz - 0x7ffff;
        int half = u->type->range / 2;
        if (field_20 > half) {
            field_20 = half;
            unsigned short angle = u->f64.y;
            Vec3 vec;
            vec.x = -FUN_004b70ef(angle, half);
            vec.y = 0;
            vec.z = -FUN_004b7123(angle, half);
            *pp = vec;
        }
        u->pos.x = pos.x;
        u->pos.y = pos.y;
        u->pos.z = pos.z;
        u->flags |= 0x10000;
        return;
    }

    FUN_0047d0e0(u);
    u->pos.x = nx;
    u->pos.y = ny;
    u->pos.z = nz;
    u->cell = cell;
    u->flags = (u->flags & 0xfffffffc) | (m & 3);
    FUN_0047cc30(u);
    u->flags |= 0x10000;
    FUN_004827b0(u);
}
