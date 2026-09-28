// Decompiled by deepseek-v4.1-flash. Names are provisional.
// Updates one projectile/weapon (the object at 0x499c70's class layout): tests
// the map cell under it against the sea level, clears the tracked projectile
// when it is the one the game tracked, plays a splash/hit sound, appends an
// effect, and, when a target is present, applies weapon damage through
// FUN_00499cd0 and FUN_00406f50.
//
// The flag set at +0x69 must be a 1-bit field of an `unsigned short` bitfield
// at +0x68, not an `unsigned char`: both write the same byte, but the byte
// bitfield goes through a register (`mov al; or al; mov`) while the short one
// is emitted in place as `or byte ptr [esi+0x69], 2`. The weapon def's flag
// word is a union so bit 10 reads as `shr eax, 10; test al, 1` while bit 22
// is tested with `test dword ptr [edi+0x111], 0x400000`.

#pragma pack(push, 1)

struct Vec3_499eb0 {
    int x;
    int y;
    int z;
};

struct Src_499eb0;                  // opaque source for FUN_00420a30
struct Unit_499eb0;

struct Cell_499eb0 {
    char unknown_0[5];
    unsigned char field_5;         // +0x5
};

union Flags_499eb0 {
    unsigned int value;
    struct {
        unsigned int bits0_9 : 10;
        unsigned int bit10 : 1;
        unsigned int bits11_21 : 11;
        unsigned int bit22 : 1;
        unsigned int bits23_31 : 9;
    };
};

struct Def_499eb0 {
    char unknown_0[0x78];
    Src_499eb0* field_78;          // +0x78
    Src_499eb0* field_7c;          // +0x7c
    char unknown_80[0xcc - 0x80];
    int field_cc;                  // +0xcc
    int field_d0;                  // +0xd0
    char unknown_d4[0xd6 - 0xd4];
    unsigned short field_d6;       // +0xd6
    char unknown_d8[0xf6 - 0xd8];
    unsigned short field_f6;       // +0xf6
    unsigned short field_f8;       // +0xf8
    char unknown_fa[0xfe - 0xfa];
    short field_fe;                // +0xfe
    char unknown_100[0x111 - 0x100];
    Flags_499eb0 flags;            // +0x111
};

struct Weapon_499eb0 {
    Def_499eb0* def;               // +0x00
    Vec3_499eb0 pos;               // +0x04
    char unknown_10[0x52 - 0x10];
    Unit_499eb0* attacker;         // +0x52
    char unknown_56[0x66 - 0x56];
    unsigned char owner;           // +0x66
    char unknown_67;
    unsigned short bits0_8 : 9;
    unsigned short field69_bit1 : 1;
    unsigned short bits_rest : 6;
};

class Class_00406f50 {
public:
    void FUN_00406f50(Weapon_499eb0* weapon, int a, int b);
};

struct APlayer_499eb0 {
    char unknown_0[0x74];
    Class_00406f50* field_74;      // +0x74
};

struct Unit_499eb0 {
    char unknown_0[0x96];
    APlayer_499eb0* player;        // +0x96
    char unknown_9a[0xff - 0x9a];
    unsigned char owner;           // +0xff
};

struct Player_499eb0 {
    int active;                    // +0x00
    char unknown_4[0x73 - 0x4];
    unsigned char type;            // +0x73
    char unknown_74[0x14b - 0x74];
};

struct Mode_499eb0 {
    char unknown_0[0xd48];
    int field_d48;                 // +0xd48
};

struct Game_499eb0 {
    char unknown_0[0x1b63];
    Player_499eb0 players[10];     // +0x1b63
    char unknown_2851[0x1427f - 0x2851];
    unsigned char seaLevel;        // +0x1427f
    char unknown_14280[0x142f7 - 0x14280];
    Weapon_499eb0* tracked;        // +0x142f7
    char unknown_142fb[0x1433f - 0x142fb];
    Vec3_499eb0 trackedPos;        // +0x1433f
    short trackedValue;            // +0x1434b
    char unknown_1434d[0x391e9 - 0x1434d];
    Mode_499eb0* mode;             // +0x391e9
};
#pragma pack(pop)

extern Game_499eb0* g_game;

Cell_499eb0* __stdcall FUN_004815a0(Vec3_499eb0* pos);
void __stdcall FUN_0041c640(int dx, int dy, int value);
void __stdcall FUN_0047f300(int sound, Vec3_499eb0* pos, int param_3);
void __stdcall FUN_00420a30(Vec3_499eb0* pos, Src_499eb0* src, int index, int flag);
void __stdcall FUN_00472810(Vec3_499eb0* pos, short index);
void __stdcall FUN_0049a120(Weapon_499eb0* weapon, Vec3_499eb0* pos);
unsigned short __stdcall FUN_00499cd0(Weapon_499eb0* weapon, Unit_499eb0* target,
                                      float scale);

// FUNCTION: 0x499eb0
void __stdcall FUN_00499eb0(Weapon_499eb0* weapon, Unit_499eb0* target)
{
    Def_499eb0* def = weapon->def;
    int low = 0;
    Cell_499eb0* cell = FUN_004815a0(&weapon->pos);
    if (cell != 0)
        low = cell->field_5 < g_game->seaLevel;
    if ((def->flags.value & 0x400000) == 0) {
        if (weapon == g_game->tracked) {
            g_game->trackedPos = g_game->tracked->pos;
            g_game->trackedValue = weapon->def->field_fe;
            g_game->tracked = 0;
        }
        weapon->field69_bit1 = 1;
    }
    if (g_game->mode->field_d48 != 0 && low != 0 && target == 0) {
        if (weapon == g_game->tracked) {
            g_game->trackedPos = g_game->tracked->pos;
            g_game->trackedValue = weapon->def->field_fe;
            g_game->tracked = 0;
        }
        weapon->field69_bit1 = 1;
        return;
    }
    FUN_0041c640(def->field_cc, def->field_cc, def->field_d0);
    if (low != 0 && target == 0) {
        FUN_0047f300(def->field_f8, &weapon->pos, 0);
        FUN_00420a30(&weapon->pos, def->field_7c, 0, low);
    } else {
        FUN_0047f300(def->field_f6, &weapon->pos, 0);
        if (def->flags.bit10)
            FUN_00472810(&weapon->pos, 9);
        else
            FUN_00420a30(&weapon->pos, def->field_78, 0, low);
    }
    unsigned int i = weapon->owner;
    Player_499eb0* p = &g_game->players[i];
    if (p->active == 0 || p->type != 3) {
        if (def->field_d6 > 0x10 || target == 0) {
            FUN_0049a120(weapon, &weapon->pos);
            return;
        }
        unsigned short damage = FUN_00499cd0(weapon, target, 1.0f);
        Unit_499eb0* attacker = weapon->attacker;
        if (attacker == 0)
            return;
        unsigned short a = 0;
        unsigned short b = 0;
        if (weapon->owner != target->owner)
            a = damage;
        else
            b = damage;
        attacker->player->field_74->FUN_00406f50(weapon, a, b);
    }
}
