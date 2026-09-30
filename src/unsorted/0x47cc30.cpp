// Decompiled by deepseek-v4.1. Names are provisional.
// PARTIAL 75.1% (best kept here, 1213 bytes vs the original 1199). Body, frame,
// loops and the inlined FUN_0047cb60 owner surgery all match. What still
// differs:
// 1) The two sum tests at the top. The negative tests and the two movsx now
//    match only with the condition written inline and `size` first
//    (`size.x + obj->pos.x >= g_game->width`, separate ifs): that variant gets
//    `mov ebp,g_game / movsx ebx,dx / movsx edx,ax` right and the pos.x/pos.y
//    registers right (ax/cx), but MSVC still builds the sum in a fresh eax
//    (`mov eax,ebx / add eax,edx / mov edx,width / cmp eax,edx`) instead of
//    reusing edx and loading width into eax, and it spills size.x before the
//    add instead of after the cmp; the second sum keeps height in eax instead
//    of ecx and adds through a copy of size.y. `int sx, sy` locals (any
//    declaration order, combined or separate ifs), short locals, operand
//    order and reversed comparisons (`width <= ...`) all give the same shape
//    or worse; the sx/sy locals cannot be declared after the first `goto
//    remove` (C2362).
// 2) The a-loop's rec block. The b and c loops now match byte for byte in
//    layout (tests, then arms) but MSVC sinks the a-loop's "bad" arm to the
//    end of the function (0x47d0c5) so both of its tests become 6-byte near
//    jumps, +8 bytes over the original's in-place arm. Tried: plain
//    `active == 0 || type != 3`, `active != 0 && type == 3` with
//    `goto a_good`, nested ifs, explicit else, duplicated write. All compile
//    to the same block order.
// 3) `test bl,al` is emitted as `test al,bl` (same encoding length, the
//    checker still flags it).
// 4) The owner index does `lea eax,[esi+0x6a]` style addressing while the
//    original does `lea ecx,[esi+0x6a]; mov edx,ecx` and indexes through edx.
// Kept here: the 75.1% build. A 74.7% variant with the correct top negative
// tests and correct movsx order is in build/scratch/0x47cc30/v9.cpp.
#pragma pack(push, 1)

struct Obj_0047cc30;

struct Owner_0047cc30 {
    char unknown_0[6];
    Obj_0047cc30* first;                // +0x6
};

struct Unit_0047cc30 {
    char unknown_0[0x14e];
    unsigned char* mask;                // +0x14e
};

struct Player_0047cc30 {
    int active;                         // +0x0
    char unknown_4[0x73 - 0x4];
    unsigned char type;                 // +0x73
};

struct UnitRec_0047cc30 {               // 0x118 bytes
    char unknown_0[0x92];
    void* def;                          // +0x92
    Player_0047cc30* owner;             // +0x96
    char unknown_9a[0x110 - 0x9a];
    unsigned int flags;                 // +0x110
    char unknown_114[0x118 - 0x114];
};

struct Cell_0047cc30 {
    unsigned short field_0;             // +0x0
    unsigned short field_2;             // +0x2
    char unknown_4[0xc - 0x4];
    unsigned char field_c;              // +0xc
};

struct Point_0047cc30 {
    short x;
    short y;
};

struct Position_0047cc30 {
    int x;
    int y;
    int z;
};

union Flags_0047cc30 {
    struct {
        unsigned int unknown_0 : 27;
        unsigned int flag27 : 1;
        unsigned int unknown_1 : 4;
    } bits;
    int all;
};

struct Obj_0047cc30 {
    unsigned char* field_0;             // +0x0
    char unknown_4[0x26 - 0x4];
    int field_26;                       // +0x26
    char unknown_2a[0x6a - 0x2a];
    Position_0047cc30 position;         // +0x6a
    Point_0047cc30 pos;                 // +0x76
    char unknown_7a[4];
    Point_0047cc30 size;                // +0x7e
    Owner_0047cc30* owner;              // +0x82
    int field_86;                       // +0x86
    char unknown_8a[4];
    Obj_0047cc30* next;                 // +0x8e
    Unit_0047cc30* unit;                // +0x92
    char unknown_96[0xa8 - 0x96];
    unsigned short field_a8;            // +0xa8
    char unknown_aa[0x10f - 0xaa];
    unsigned char bit0 : 1;             // +0x10f
    unsigned char bit1 : 1;
    unsigned char bit2 : 1;
    Flags_0047cc30 flags;               // +0x110
};

struct Game_0047cc30 {
    char unknown_0[0x14233];
    int width;                          // +0x14233
    int height;                         // +0x14237
    char unknown_1423b[0x14287 - 0x1423b];
    Cell_0047cc30* cells;               // +0x14287
    char unknown_1428b[0x1429f - 0x1428b];
    Owner_0047cc30* owners;             // +0x1429f
    int ownerCols;                      // +0x142a3
    char unknown_142a7[0x142b7 - 0x142a7];
    Owner_0047cc30* defaultOwner;       // +0x142b7
    char unknown_142bb[0x14357 - 0x142bb];
    UnitRec_0047cc30* units;            // +0x14357
    char unknown_1435b[0x38a47 - 0x1435b];
    int field_38a47;                    // +0x38a47
};
#pragma pack(pop)

extern Game_0047cc30* g_game;

void __stdcall FUN_00483210(Point_0047cc30 pos, Point_0047cc30 size);
void __stdcall FUN_00440a40(Point_0047cc30 pos, Point_0047cc30 size);

static void SetOwner_0047cc30(Obj_0047cc30* obj, Owner_0047cc30* nw)
{
    if (nw != obj->owner) {
        if (obj->field_86 == 0) {
            Owner_0047cc30* old = obj->owner;
            if (old != 0) {
                Obj_0047cc30** pp = &old->first;
                while (*pp != obj)
                    pp = &(*pp)->next;
                *pp = obj->next;
                obj->next = 0;
            }
            obj->next = nw->first;
            nw->first = obj;
        }
        obj->owner = nw;
    }
}

// FUNCTION: 0x47cc30
void __stdcall FUN_0047cc30(Obj_0047cc30* obj)
{
    Point_0047cc30 size = obj->size;
    if (obj->field_0 != 0)
        *(int*)(obj->field_0 + 0x26) = g_game->field_38a47;
    int sx, sy;
    if (obj->pos.x < 0 || obj->pos.y < 0)
        goto remove;
    sx = obj->pos.x + size.x;
    sy = obj->pos.y + size.y;
    if (sx >= g_game->width || sy >= g_game->height)
        goto remove;

    {
        Position_0047cc30 p = obj->position;
        SetOwner_0047cc30(obj,
            &g_game->owners[(p.x >> 23) + (p.z >> 23) * g_game->ownerCols]);
    }
    {
        Cell_0047cc30* cell = &g_game->cells[g_game->width * obj->pos.y + obj->pos.x];
        unsigned int f = obj->flags.all;
        int index = 0;

        if (f & 0x20000000) {
            for (int y = size.y; y > 0; y--) {
                for (int x = size.x; x > 0; x--) {
                    unsigned char m = obj->unit->mask[index++];
                    if (m & (obj->bit2 ? 2 : 4)) {
                        unsigned short id = cell->field_0;
                        if (id != 0) {
                            UnitRec_0047cc30* rec = &g_game->units[id];
                            if (rec->owner->active == 0 || rec->owner->type != 3) {
                                rec->flags |= 0x4000000;
                                obj->flags.all |= 0x8000000;
                                goto a_next;
                            }
                            rec->flags |= 0x8000000;
                            obj->flags.all |= 0x4000000;
                        }
                        cell->field_0 = obj->field_a8;
                    }
                a_next:
                    if (m & 1)
                        cell->field_c |= 2;
                    cell++;
                }
                cell += g_game->width - size.x;
            }
            Point_0047cc30 grown;
            grown.x = size.x + 2;
            grown.y = size.y + 2;
            Point_0047cc30 pad;
            pad.x = obj->pos.x - 1;
            pad.y = obj->pos.y - 1;
            FUN_00483210(pad, grown);
            FUN_00440a40(obj->pos, obj->size);
            return;
        }
        if ((f & 3) == 1) {
            for (int y = size.y; y > 0; y--) {
                for (int x = size.x; x > 0; x--) {
                    unsigned short id = cell->field_0;
                    if (id != 0) {
                        UnitRec_0047cc30* rec = &g_game->units[id];
                        if (rec->owner->active == 0 || rec->owner->type != 3) {
                            rec->flags |= 0x4000000;
                            obj->flags.all |= 0x8000000;
                            goto b_next;
                        }
                        rec->flags |= 0x8000000;
                        obj->flags.all |= 0x4000000;
                    }
                    cell->field_0 = obj->field_a8;
                b_next:
                    cell++;
                }
                cell += g_game->width - size.x;
            }
            return;
        }
        if ((f & 3) == 2) {
            for (int y = size.y; y > 0; y--) {
                for (int x = size.x; x > 0; x--) {
                    unsigned short id = cell->field_2;
                    if (id != 0) {
                        UnitRec_0047cc30* rec = &g_game->units[id];
                        if (rec->owner->active == 0 || rec->owner->type != 3) {
                            rec->flags |= 0x4000000;
                            obj->flags.all |= 0x8000000;
                            goto c_next;
                        }
                        rec->flags |= 0x8000000;
                        obj->flags.all |= 0x4000000;
                    }
                    cell->field_2 = obj->field_a8;
                c_next:
                    cell++;
                }
                cell += g_game->width - size.x;
            }
        }
    }
    return;

remove:
    SetOwner_0047cc30(obj, g_game->defaultOwner);
}
