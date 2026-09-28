#include <string.h>
// Decompiled by deepseek-v4.1-flash. Names are provisional.

#pragma pack(push, 1)

union Fixed_0049abb0 {
    int value;
    struct { unsigned short fraction; short whole; } parts;
};

struct Vec3_0049abb0 {
    int x;                             // +0x0
    Fixed_0049abb0 y;                  // +0x4
    int z;                             // +0x8
};

struct WeaponDef_0049abb0 {
    char unknown_0[0x68];
    int field_68;                      // +0x68
    char unknown_6c[0xc8 - 0x6c];
    int field_c8;                      // +0xc8
    char unknown_cc[0xdc - 0xcc];
    int range;                         // +0xdc
    char unknown_e0[0x111 - 0xe0];
    union {
        unsigned int value;
        struct {
            unsigned int bit0 : 1;
            unsigned int bit1 : 1;
            unsigned int bit2_15 : 14;
            unsigned int bit16 : 1;
            unsigned int bit17 : 1;
            unsigned int bit18_31 : 14;
        } bits;
    } flags;                           // +0x111
};

struct Weapon_0049abb0 {
    char unknown_0[8];
    WeaponDef_0049abb0* def;           // +0x8
    char unknown_c[0x1c - 0xc];
};

struct UnitDef_0049abb0 {
    char unknown_0[0x170];
    short field_170;                   // +0x170
    char unknown_172[0x241 - 0x172];
    int field_241;                     // +0x241
};

struct Unit_0049abb0 {
    char unknown_0[8];
    Weapon_0049abb0 weapons[3];        // +0x8, stride 0x1c
    char unknown_5c[0x6a - 0x5c];
    Vec3_0049abb0 pos;                 // +0x6a
    char unknown_76[0x92 - 0x76];
    UnitDef_0049abb0* def;             // +0x92
    char unknown_96[0x110 - 0x96];
    int field_110;                     // +0x110
};

struct Game_0049abb0 {
    char unknown_0[0x1427f];
    unsigned char field_1427f;         // +0x1427f
};

#pragma pack(pop)

extern Game_0049abb0* g_game;

short __stdcall FUN_0049a890(int dx, int dy, int dz, int a, int b);

// FUNCTION: 0x49abb0
int __stdcall FUN_0049abb0(Unit_0049abb0* a1, Unit_0049abb0* a2, int a3)
{
    WeaponDef_0049abb0* wdef = a1->weapons[a3 & 0xff].def;
    if (wdef->flags.bits.bit16) {
        UnitDef_0049abb0* d2 = a2->def;
        if (!(d2->field_241 & 0x80000)) {
            if (a2->pos.y.parts.whole > g_game->field_1427f)
                return 0;
        }
        if (d2->field_241 & 0x1000) {
            if (a2->pos.y.parts.whole + (d2->field_170 >> 1) > g_game->field_1427f)
                return 0;
        }
        int dz = (a2->pos.z - a1->pos.z);
        int dx = (a2->pos.x - a1->pos.x);
        int r = wdef->range;
        return (int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dz * dz) >> 32) <= r * r;
    }

    if (a1->pos.y.parts.whole + a1->def->field_170 <= g_game->field_1427f)
        return 0;
    if (a2->pos.y.parts.whole + a2->def->field_170 <= g_game->field_1427f)
        return 0;
    if ((wdef->flags.value & 0x20000) && ((a2->field_110 & 3) != 2))
        return 0;
    if (wdef->flags.bits.bit1) {
        Vec3_0049abb0 p1 = a1->pos;
        Vec3_0049abb0 p2 = a2->pos;
        if (FUN_0049a890(p1.x - p2.x, p1.y.value - p2.y.value,
                         p1.z - p2.z, wdef->field_68, wdef->field_c8) == (short)0x8000)
            return 0;
    }
    int dz = (a2->pos.z - a1->pos.z);
    int dx = (a2->pos.x - a1->pos.x);
    int r = wdef->range;
    return (int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dz * dz) >> 32) <= r * r;
}
