// Decompiled by DeepSeek V4.1 Flash, finished by GPT-6, finished by deepseek-v4.1-flash, finished by GPT-6.1-sol, finished by deepseek-v4.1-flash, finished by space-bunny-free. Names are provisional.
// 60.4% (difflib), 1562 bytes vs 1595. Retry notes from previous models still
// apply (the FUN_0043de30 guard is rec+0x27, the order count is rec+0x23).
// This run's change: the 3-piece copy at the end was decoding the wrong struct
// layout. From the disassembly the destination Piece_00487080 (0x1c bytes at
// unit+4) is f0 at +0, f8 at +8, an obj pointer at +0xc whose field +0x10a is
// written from the source's +8 byte, then +0x10, +0x14, +0x16, +0x18, +0x1a
// and the flag byte at +0x1b. With the correct layout the copy loop compiles
// instruction-for-instruction identical to the original; only the base register
// offsets differ (ours eax = dest+0, edi = src+8; original eax = dest+8,
// edi = src+4), which is MSVC's arbitrary displacement folding and not a
// source-shape difference I could find a lever for.
// Measured and rejected this run (free scratch scoring, --sym):
// `int found` instead of `bool found` (45.5, the first loop's allocation
// collapses); `sprintf(script, "Script%i", i)` taking the search index (46.3,
// the original really does use rec+0x3b there even though the slot it reads is
// the loop counter's); `char script[0x1c]` (58.5); typed `SrcPiece pieces[3]`
// inside the record (59.1, MSVC 5 then sizes the record 4 bytes larger, which
// shifts every local above it); `sprintf(name + 4, ...)` (59.1, the original
// passes the same pointer to sprintf and to FUN_0043a420).
// Still differing: the first loop's allocation (original n in esi, found in
// ebp, i spilled to [esp+0x10]; ours n in ebp, i in esi, found in a byte
// local), the FUN_00485f50 argument rotation, the 0x110 flags block rotation,
// the extra `lea edx,[esi+0x64]` in the field-copy run, and the failure
// epilogue's xor/pop order.
// order-count and FUN_0043de30 guard to the correct record fields: the count
// is rec+0x23 (f23) and the guard is rec+0x27 (f27), not rec+0x33. The old
// attempt read +0x33 for both, which is the second coordinate of rec.pos.
// Tried and rejected (all scored on scratch, worse than 60.6): removing the
// `player` local and recomputing (rec.flags>>4)&3 twice (41.3); modelling
// rec+0x2b..+0x3b as Vec3 pos plus an {int,short} sub-struct copied whole
// (47.1); `char` instead of `unsigned char` for rec+0x8d combined with the
// Script%i argument (59.6); an explicit `else p = 0;` after operator new
// (46.6); `int found` (46.6). Note `Script%i` in the original takes the search
// index i, not rec+0x3b, and rec+0x8d is sign-extended at the FUN_0048aac0
// call, but both changes only ever cost points at this compiler state.
// Remaining differences: first loop register allocation (original n in esi,
// found in ebp, i spilled to [esp+0x10]; ours n in ebp, i in esi, found in a
// byte local), the FUN_00485f50 argument register rotation, the 0x110 flags
// block rotation, the 3x piece-copy anchors, and the failure epilogue's
// xor/pop order.

extern "C" int __cdecl sprintf(char* buf, const char* fmt, ...);

struct Vec3_00487080 {
    int x, y, z;
};

#pragma pack(push, 1)
struct ObjPiece_00487080 {
    char gap[0x10a];
    unsigned char field_10a;
};

struct SrcPiece_00487080 {
    int f0;
    int f4;
    unsigned char f8;
    char gap_9[3];
    int fc;
    short f10;
    short f12;
    short f14;
    unsigned char f16;
    union { unsigned char flags; struct { unsigned char bit0:1,bit1:1,bits2:2,bit4:1,rest:3; }; };
};

struct SaveRec_00487080 {
    char name[0x20];
    unsigned char player;
    unsigned short id;
    int f23;
    int f27;
    int f2b;
    int f2f;
    int f33;
    int f37;
    short f3b;
    short f3d;
    short f3f;
    char pieces[3 * 0x18];
    short childA;
    short childB;
    unsigned char b8d;
    unsigned char b8e;
    int f8f;
    int f93;
    int f97;
    int f9b;
    int f9f;
    int fa3;
    int fa7;
    unsigned char bab;
    unsigned char bac;
    unsigned char bad;
    short bae;
    unsigned char bb0;
    unsigned char bb1;
    unsigned char bb2;
    unsigned char b3;
    unsigned int flags;
};

struct Piece_00487080 {
    int f0;
    int gap4;
    int f8;
    ObjPiece_00487080* obj;
    int f10;
    short f14;
    short f16;
    short f18;
    unsigned char f1a;
    union { unsigned char flags; struct { unsigned char bit0:1,bit1:1,bits2:2,bit4:1,rest:3; }; };
};

#pragma pack(push, 1)
#pragma pack(pop)

#pragma pack(push, 1)
struct Unit_00487080 {
    void* vtable;                       // +0x0
    Piece_00487080 pieces[3];           // +0x4
    int field_58;                       // +0x58
    void* listHead;                     // +0x5c
    void* listTail;                     // +0x60
    int field_64;                       // +0x64
    short field_68;                     // +0x68
    char gap_6a[4];
    int field_6e;                       // +0x6e
    char gap_72[4];
    int field_76;                       // +0x76
    int field_7a;                       // +0x7a
    int field_7e;                       // +0x7e
    char gap_82[0x18];
    void* field_9a;                     // +0x9a
    char gap_9e[0xa];
    unsigned short id;
    char gap_aa[2];
    int field_ac;                       // +0xac
    int field_b0;                       // +0xb0
    char gap_b4[4];
    short field_b8;                     // +0xb8
    short field_ba;                     // +0xba
    char info[0x34];                    // +0xbc
    Unit_00487080* child;               // +0xf0
    unsigned char b_f4;
    unsigned char b_f5;
    unsigned char b_f6;
    unsigned char b_f7;
    unsigned char b_f8;
    unsigned char b_f9;
    unsigned char b_fa;
    char gap_fb[9];
    int field_104;                      // +0x104
    short field_108;                    // +0x108
    char gap_10a[4];
    unsigned char b_10e;
    unsigned char b_10f;
    unsigned int flags;                 // +0x110
    char gap_114[4];
};

#pragma pack(pop)
class Class_004b4560 {
public:
    int FUN_004b4560(char* name);
};
class Class_004b4800 {
public:
    int FUN_004b4800(char* name, int def);
};
class Class_004b4b50 {
public:
    int FUN_004b4b50(int a);
};
class Class_004b4c10 {
public:
    void FUN_004b4c10(int pos);
};
class Class_004b4c80 {
public:
    int FUN_004b4c80(void* buf, int len);
};
class Class_004b4ba0 {
public:
    int FUN_004b4ba0(char* name);
};

extern void* g_game;

unsigned short __stdcall FUN_00488b10(const char* name);
Unit_00487080* __stdcall FUN_00485f50(unsigned char player, unsigned short typeId,
                                      Vec3_00487080 pos, int param_5, int mode,
                                      unsigned short id);
void __stdcall FUN_0048aac0(Unit_00487080* unit, Unit_00487080* builder, char piece, char p4);
void __stdcall FUN_00480250(Unit_00487080* unit, int id);
#pragma pack(push, 1)
class Order_00487080 { public: char pad[0x42]; unsigned int flags; char gap[4]; Order_00487080* next; Order_00487080* FUN_0043a420(Unit_00487080*, Class_004b4560*, char*); };
#pragma pack(pop)
class Class_004388b0 { public: void FUN_004388b0(); };
void __stdcall FUN_0047db20(Unit_00487080* unit);
class Class_00401110 { public: void FUN_00401110(Unit_00487080*, Class_004b4560*); };
class Class_0043d210 { public: void FUN_0043de30(Unit_00487080*, Class_004b4560*); };
class Class_004b0610 { public: void FUN_004b2040(Class_004b4560*); };

// FUNCTION: 0x487080
Unit_00487080* __stdcall FUN_00487080(unsigned short id, Class_004b4560* file)
{
    Unit_00487080* unit;
    if (id == 0)
        unit = 0;
    else
        unit = (Unit_00487080*)(*(char**)((char*)g_game + 0x14357) + id * 0x118);
    if (unit == 0 || (unit->flags & 0x10000000))
        return 0;

    SaveRec_00487080 rec;
    char name[0x20];
    char script[0x20];
    int n = ((Class_004b4800*)file)->FUN_004b4800("Number of Units", 0);
    bool found = 0;
    int i;
    for (i=0;i<n;i++) {
            if (!((Class_004b4b50*)file)->FUN_004b4b50(i))
                return 0;
            ((Class_004b4c10*)file)->FUN_004b4c10(0);
            if (((Class_004b4c80*)file)->FUN_004b4c80(&rec, 0xb8) != 0xb8)
                return 0;
            if (rec.id == id) {
                found = 1;
                break;
            }
    }
    if (!found)
        return 0;

    int player = (rec.flags >> 4) & 3;
    unit = FUN_00485f50(rec.player, FUN_00488b10(rec.name), *(Vec3_00487080*)&rec.f2b, 1, (rec.flags >> 4) & 3, rec.id);
    if (unit == 0)
        return 0;

    unit->field_64 = rec.f37;
    unit->field_68 = rec.f3b;
    unit->field_108 = rec.f3d;
    unit->field_b8 = rec.f3f;
    unit->field_6e = rec.f2f;

    if (rec.childA != 0) {
        Unit_00487080* child = FUN_00487080(rec.childA, file);
        if (child != 0)
            FUN_0048aac0(unit, child, rec.b8d, player);
    }
    unit->child = FUN_00487080(rec.childB, file);
    unit->b_f9 = rec.b8d;
    unit->b_f4 = rec.b8e;
    unit->field_58 = rec.f8f;
    unit->field_76 = rec.f93;
    unit->field_7a = rec.f97;
    unit->field_7e = rec.f9b;
    unit->field_ac = rec.f9f;
    FUN_00480250(unit, rec.f9f);
    unit->field_104 = rec.fa7;
    unit->b_f5 = rec.bab;
    unit->b_f6 = rec.bac;
    unit->b_f7 = rec.bad;
    unit->field_ba = rec.bae;
    unit->b_f8 = rec.bb0;
    unit->b_fa = rec.bb1;
    unit->b_10e = rec.bb2;

    unsigned int f = rec.flags;
    unsigned char b = unit->b_10f;
    b = ((unsigned char)f ^ b) & 1 ^ b;
    unit->b_10f = b;
    b = (unsigned char)((rec.flags >> 1 & 1) << 1) | (b & 0xfd);
    unit->b_10f = b;
    b = (unsigned char)((rec.flags >> 2 & 1) << 2) | (b & 0xfb);
    unit->b_10f = b;
    b = (unsigned char)((rec.flags >> 3 & 1) << 3) | (b & 0xf7);
    unit->b_10f = b;

    unsigned int u = unit->flags;
    u = (rec.flags >> 4 & 0xc) | (u & 0xfffffff3);
    unit->flags = u;
    u = (rec.flags >> 4 & 0x10) | (u & 0xffffffef);
    unit->flags = u;
    u = (rec.flags >> 4 & 0x20) | (u & 0xffffffdf);
    unit->flags = u;
    u = (rec.flags >> 4 & 0xc0) | (u & 0xffffff3f);
    unit->flags = u;
    u = (rec.flags >> 4 & 0x100) | (u & 0xfffffeff);
    unit->flags = u;
    u = (rec.flags >> 4 & 0x200) | (u & 0xfffffdff);
    unit->flags = u;
    u = (rec.flags >> 4 & 0x400) | (u & 0xfffffbff);
    unit->flags = u;
    u = (rec.flags >> 4 & 0x800) | (u & 0xfffff7ff);
    unit->flags = u;
    unit->field_b0 = rec.fa3;
    u = (rec.flags >> 3 & 0x2000) | (u & 0xffffdfff);
    unit->flags = u;
    u = (rec.flags >> 6 & 0x4000) | (u & 0xffffbfff);
    unit->flags = u;
    u = (rec.flags >> 6 & 0x8000) | (u & 0xffff7fff);
    unit->flags = u;
    u = (rec.flags >> 6 & 0x10000) | (u & 0xfffeffff);
    unit->flags = u;
    u = (rec.flags >> 6 & 0x20000) | (u & 0xfffdffff);
    unit->flags = u;
    u = (rec.flags >> 6 & 0xc0000) | (u & 0xfff3ffff);
    unit->flags = u;
    u = (rec.flags >> 6 & 0x300000) | (u & 0xffcfffff);
    unit->flags = u;
    u = (rec.flags >> 6 & 0x400000) | (u & 0xffbfffff);
    unit->flags = u;
    u = (rec.flags >> 6 & 0x3800000) | (u & 0xfc7fffff);
    unit->flags = u;

    ((Class_00401110*)&unit->info)->FUN_00401110(unit, file);
    if (rec.f27 != 0)
        ((Class_0043d210*)unit->vtable)->FUN_0043de30(unit, file);

    Order_00487080** normal=(Order_00487080**)&unit->listHead;
    Order_00487080** special=(Order_00487080**)&unit->listTail;
    int k = 0;
    if (rec.f23 > 0) {
        do {
            sprintf(name, "u%04xm%04x", unit->id, k);
            Order_00487080* p = (Order_00487080*)operator new(0x56);
            if (p != 0)
                p = p->FUN_0043a420(unit, file, name);
            if (p->flags & 0x40000) { *special=p; special=&p->next; }
            else { *normal=p; normal=&p->next; }
            k++;
        } while (k < rec.f23);
    }
    if (unit->listHead != 0)
        ((Class_004388b0*)unit->listHead)->FUN_004388b0();
    {
        sprintf(script, "Script%i", rec.f3b);
        ((Class_004b4ba0*)file)->FUN_004b4ba0(script);
    }
    ((Class_004b0610*)unit->field_9a)->FUN_004b2040(file);

    SrcPiece_00487080* s = (SrcPiece_00487080*)rec.pieces;
    Piece_00487080* d = unit->pieces;
    int j = 3;
    do {
        d->f0 = s->f0;
        d->f8 = s->f4;
        d->obj->field_10a = s->f8;
        d->f10 = s->fc;
        d->f14 = s->f10;
        d->f16 = s->f12;
        d->f18 = s->f14;
        d->f1a = s->f16;
        d->flags = (s->flags ^ d->flags) & 1 ^ d->flags;
        d->flags = (s->flags ^ d->flags) & 2 ^ d->flags;
        d->flags = (s->flags ^ d->flags) & 0xc ^ d->flags;
        d->flags = (s->flags ^ d->flags) & 0x10 ^ d->flags;
        s++;
        d++;
        j--;
    } while (j != 0);

    if (unit->b_10f & 4)
        FUN_0047db20(unit);
    return unit;
}
