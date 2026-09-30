// Decompiled by deepseek-v4.1-flash, finished by space-bunny-free. Names are provisional.
// PARTIAL 71.3% (1641 of 1662 bytes), up from 67.7%. Two changes did it: (1) the
// los half of OnRadar is a single `&&` chain. As `if (inbounds) b = los[..] != 0;
// else b = 0;` MSVC 5 if-converts the value to `setne`; folded into the chain
// (`b = inbounds && los[..] != 0`) the value materialises branchy and every
// bound failure joins the one `xor edx,edx`, which is the original's shape, and
// the bitfield half's block order falls into place for free. `? 1 : 0` does NOT
// work, it if-converts exactly like `!= 0` does. (2) The named `Shot* shot`
// local had to go: with it MSVC hoists the `p->shot->flags` load above the y
// divide, and the inlined OnRadar is what puts the pressure there.
// Still open, roughly in order of size:
// (1) the projectile loop. The original homes `p` at [esp+0x1c] and reloads it
//     into ecx as the loop head (so the loop is `jmp`ped over that one reload
//     and the p store is the latch), while `q` keeps ebx. We keep `p` in ebx and
//     `q` in ecx, which also pushes the inlined OnRadar's PlayerInfo pointer
//     onto ebp instead of edx. Declaring q before p, reading player/owner
//     through q, and `p++` instead of the byte cast were all tried and are
//     neutral or worse, so the cause is not use counts or declaration order.
// (2) `enabled`: the original loads 16 bits (`mov ax, word ptr [esi+0x14281]`)
//     and that word load is what stops MSVC keeping the constant 1 in a
//     register, so the last store is an immediate. A union-typed field and a
//     named `unsigned short` local both leave a plain `test byte ptr` and a
//     register-held 1; a single-use local is folded away before codegen.
// (3) the two FUN_004b7f90 calls load `surface` after `push eax` there and
//     before it here.
// Tried and neutral: the `!= 0` spelling of the slot->field_e test, plain
// `p++` for the projectile stride, the union-typed +0x14281 field, a named
// 16-bit local for it. Tried and worse: declaring `q` before `p` (70.8), reading
// `p->player` and `p->owner` through `q` as the original's encoding does
// (70.9), the `&&` chain with the bitfield half chained too.
#pragma pack(push, 1)

struct Shot_00466dc0;

union Flags110_00466dc0 {
    unsigned int all;
    struct {
        unsigned int :4;
        unsigned int bit4 : 1;
        unsigned int :27;
    } bits;
};

struct Flags241_00466dc0 {
    unsigned int :29;
    unsigned int bit29 : 1;
    unsigned int :2;
};

union Flags111_00466dc0 {
    unsigned int all;
    struct {
        unsigned int :30;
        unsigned int bit30 : 1;
        unsigned int :1;
    } bits;
};

union Flags142f0_00466dc0 {
    unsigned char bytes[2];
    struct {
        unsigned char lo;                // +0x142f0
        unsigned char hi;                // +0x142f1
    } b;
    struct {
        unsigned short :8;
        unsigned short bit0 : 1;         // +0x142f1 bit 0
        unsigned short bit1 : 1;         // +0x142f1 bit 1
        unsigned short :6;
    } bits;
};

union Flags14281_00466dc0 {
    unsigned short all;
    struct {
        unsigned short bit0 : 1;
        unsigned short bit1 : 1;
        unsigned short :7;
        unsigned short bit9 : 1;
        unsigned short :5;
    } bits;
};

struct Player_00466dc0 {
    char unknown_0[0x96];
    unsigned char field_96;              // +0x96
};

struct PlayerInfo_00466dc0 {
    char unknown_0[0x27];
    Player_00466dc0* data;               // +0x27
    char unknown_2b[0x7c - 0x2b];
    unsigned char* los;                  // +0x7c
    int width;                           // +0x80
    int height;                          // +0x84
    char unknown_88[0x14b - 0x88];
};

struct Slot_00466dc0 {
    Shot_00466dc0* shot;                 // +0x0
    char unknown_4[0xe - 4];
    unsigned char field_e;               // +0xe
    char unknown_f[0x1c - 0xf];
};

struct Shot_00466dc0 {
    char unknown_0[0xe0];
    int field_e0;                        // +0xe0
    char unknown_e4[0x111 - 0xe4];
    Flags111_00466dc0 flags;             // +0x111
};

struct UnitType_00466dc0 {
    char unknown_0[0x204];
    short field_204;                     // +0x204
    short field_206;                     // +0x206
    char unknown_208[0x20a - 0x208];
    short field_20a;                     // +0x20a
    short field_20c;                     // +0x20c
    char unknown_20e[0x241 - 0x20e];
    Flags241_00466dc0 flags_241;         // +0x241
    unsigned char field_245;             // +0x245
};

struct Unit_00466dc0 {
    char unknown_0[0x10];
    Slot_00466dc0 slots[3];              // +0x10
    char unknown_64[0x8];
    short field_6c;                      // +0x6c
    char unknown_6e[2];
    short field_70;                      // +0x70
    char unknown_72[2];
    short field_74;                      // +0x74
    char unknown_76[0x1c];
    UnitType_00466dc0* type;             // +0x92
    char unknown_96[0x10];
    short field_a6;                      // +0xa6
    short field_a8;                      // +0xa8
    char unknown_aa[0x50];
    unsigned char field_fa;              // +0xfa
    char unknown_fb[0x4];
    unsigned char field_ff;              // +0xff
    char unknown_100[0xe];
    unsigned char field_10e;             // +0x10e
    char unknown_10f[0x1];
    Flags110_00466dc0 flags_110;         // +0x110
    char unknown_114[0x4];
};

struct Projectile_00466dc0 {
    Shot_00466dc0* shot;                 // +0x0
    int posx;                            // +0x4
    int posy;                            // +0x8
    int posz;                            // +0xc
    char unknown_10[0x52 - 0x10];
    Unit_00466dc0* owner;                // +0x52
    char unknown_56[0x66 - 0x56];
    unsigned char player;                // +0x66
    char unknown_67[0x6b - 0x67];
};

struct Blip_00466dc0 {
    short id;                            // +0x0
    int x;                               // +0x2
    int y;                               // +0x6
};

struct Game_00466dc0 {
    char unknown_0[0x2a43];
    unsigned char currentPlayer;         // +0x2a43
    char unknown_2a44[0x2cba - 0x2a44];
    short field_2cba;                    // +0x2cba
    char unknown_2cbc[0x141f3 - 0x2cbc];
    int projectileCount;                 // +0x141f3
    Projectile_00466dc0* projectiles;    // +0x141f7
    char unknown_141fb[0x1422b - 0x141fb];
    int field_1422b;                     // +0x1422b
    int field_1422f;                     // +0x1422f
    char unknown_14233[0x14273 - 0x14233];
    unsigned short* field_14273;         // +0x14273
    char unknown_14277[0x14281 - 0x14277];
    unsigned short field_14281;           // +0x14281
    char unknown_14283[0x142db - 0x14283];
    void* field_142db;                   // +0x142db
    void* field_142df;                   // +0x142df
    char unknown_142e3[0x142e7 - 0x142e3];
    short field_142e7;                   // +0x142e7
    short field_142e9;                   // +0x142e9
    short field_142eb;                   // +0x142eb
    short field_142ed;                   // +0x142ed
    char unknown_142ef[1];
    Flags142f0_00466dc0 field_142f0;     // +0x142f0
    char unknown_142f2[0x14357 - 0x142f2];
    Unit_00466dc0* units;                // +0x14357
    Unit_00466dc0* unitsEnd;             // +0x1435b
    char unknown_1435f[0x14363 - 0x1435f];
    unsigned short* field_14363;         // +0x14363
    char unknown_14367[0x1436b - 0x14367];
    int field_1436b;                     // +0x1436b
    char unknown_1436f[0x147df - 0x1436f];
    void* field_147df;                   // +0x147df
    void* field_147e3;                   // +0x147e3
    void* field_147e7;                   // +0x147e7
    char unknown_147eb[0x37f2f - 0x147eb];
    Flags14281_00466dc0 field_37f2f;     // +0x37f2f
};
#pragma pack(pop)

extern Game_00466dc0* g_game;

void* __stdcall FUN_004b7f30(void* a, int index);
void __stdcall FUN_004b7f90(void* surface, void* bmp, int x, int y);
void __stdcall FUN_004bee60(void* surface, int x, int y, int color);
void __stdcall FUN_004c0070(void* surface, int x, int y, int radius, int color);
void __stdcall FUN_004c01a0(void* surface, int x, int y, int radius, int color,
                            int a6, int a7);
void __stdcall FUN_004c6b70(void* dst, void* bmp, int x, int y);

static PlayerInfo_00466dc0* PlayerInfo_00466dc0_Get(unsigned char p)
{
    return (PlayerInfo_00466dc0*)((char*)g_game + 0x1b63) + p;
}

// True when (px, py) is inside the current player's visible area. The two
// halves match the uint8 terrain bitmap and the packed 16-bit bitfield variant.
static inline int OnRadar_00466dc0(int px, int py)
{
    PlayerInfo_00466dc0* pi = PlayerInfo_00466dc0_Get(g_game->currentPlayer);
    int b;
    if ((g_game->field_14281 & 2) == 2) {
        b = (unsigned int)(px >> 5) < (unsigned int)pi->width &&
            (unsigned int)(py >> 5) < (unsigned int)pi->height &&
            pi->los[(py >> 5) * pi->width + (px >> 5)] != 0;
    } else {
        if ((unsigned int)(px >> 5) < (unsigned int)pi->width &&
            (unsigned int)(py >> 5) < (unsigned int)pi->height)
            b = (g_game->field_14273[(py >> 5) * pi->width + (px >> 5)] &
                 (1 << g_game->currentPlayer)) != 0;
        else
            b = 0;
    }
    return b;
}

// Scale a unit's world coordinate by the current zoom, keeping the source
// order of the multiply so the operand lands in the right register.
static inline int ScaleX_00466dc0(Unit_00466dc0* u)
{
    return (int)u->field_6c * (int)g_game->field_142eb;
}

static inline int ScaleY_00466dc0(Unit_00466dc0* u)
{
    return ((int)u->field_74 - ((int)u->field_70 >> 1)) * (int)g_game->field_142ed;
}

// FUNCTION: 0x466dc0
void FUN_00466dc0(void)
{
    unsigned char* base = (unsigned char*)g_game + 0xdcb;
    unsigned short* out = g_game->field_14363;

    g_game->field_1436b = 0;
    void* surface = g_game->field_142db;
    FUN_004c6b70(surface, g_game->field_142df, 0, 0);

    int enabled = 1;
    if ((g_game->field_14281 & 3) != 0)
        enabled = 0;
    if (g_game->field_37f2f.bits.bit9)
        enabled = 1;

    Unit_00466dc0* u = g_game->units;
    Unit_00466dc0* end = g_game->unitsEnd;
    if (u <= end) {
        do {
            if (u->field_a6 != 0) {
                if (enabled != 0 || (u->flags_110.all & 0x300) != 0 ||
                    u->field_ff == g_game->currentPlayer) {
                    int x = ScaleX_00466dc0(u) / g_game->field_1422b;
                    int y = ScaleY_00466dc0(u) / g_game->field_1422f;
                    if (u->field_fa == 0 ||
                        (g_game->field_142f0.b.hi & 1) != 0) {
                        FUN_004b7f90(surface,
                            FUN_004b7f30(g_game->field_147df,
                                PlayerInfo_00466dc0_Get(u->field_ff)->data->field_96),
                            x, y);
                    }
                    if (u->field_a8 == g_game->field_2cba) {
                        FUN_004b7f90(surface,
                            FUN_004b7f30(g_game->field_147e3, 0), x, y);
                    }
                    if (u->flags_110.bits.bit4) {
                        if ((u->field_10e & 1) != 0 ||
                            (u->type->field_245 & 4) == 0) {
                            if (u->type->field_204 != 0)
                                FUN_004c0070(surface, x, y,
                                    (int)g_game->field_142eb * u->type->field_204 /
                                    g_game->field_1422b, base[0xa]);
                            if (u->type->field_206 != 0)
                                FUN_004c0070(surface, x, y,
                                    (int)g_game->field_142eb * u->type->field_206 /
                                    g_game->field_1422b, base[0xa]);
                            if (u->type->field_20a != 0)
                                FUN_004c0070(surface, x, y,
                                    (int)g_game->field_142eb * u->type->field_20a /
                                    g_game->field_1422b, base[0xc]);
                            if (u->type->field_20c != 0)
                                FUN_004c0070(surface, x, y,
                                    (int)g_game->field_142eb * u->type->field_20c /
                                    g_game->field_1422b, base[0xc]);
                        }
                        if (u->type->flags_241.bit29) {
                            Slot_00466dc0* slot = u->slots;
                            int n = 3;
                            do {
                                Shot_00466dc0* shot = slot->shot;
                                if (shot->flags.bits.bit30) {
                                    int r = ((int)g_game->field_142eb *
                                             (shot->field_e0 - 0x200)) /
                                            g_game->field_1422b;
                                    if (slot->field_e != 0)
                                        FUN_004c01a0(surface, x, y, r, base[0xf],
                                                     0x20,
                                                     g_game->field_142f0.b.hi & 1);
                                    else
                                        FUN_004c0070(surface, x, y, r, base[0xf]);
                                }
                                slot++;
                                n--;
                            } while (n != 0);
                        }
                    }
                    out[0] = u->field_a8;
                    *(int*)(out + 1) = g_game->field_142e7 + x;
                    *(int*)(out + 3) = g_game->field_142e9 + y;
                    out += 5;
                    g_game->field_1436b++;
                }
            }
            u = (Unit_00466dc0*)((char*)u + 0x118);
        } while (u <= g_game->unitsEnd);
    }

    Projectile_00466dc0* p = g_game->projectiles;
    int i = 0;
    if (g_game->projectileCount > 0) {
        short* q = (short*)((char*)p + 0xa);
        do {
            int px = q[-2];
            int x = (int)g_game->field_142eb * px / g_game->field_1422b;
            int py = q[2] - ((int)q[0] >> 1);
            int y = (int)g_game->field_142ed * py / g_game->field_1422f;
            if ((p->shot->flags.all & 0x60000000) == 0) {
                if ((p->shot->flags.all & 0x40) == 0) {
                    if (OnRadar_00466dc0(px, py) ||
                        p->player == g_game->currentPlayer) {
                        FUN_004bee60(surface, x, y, base[0xe]);
                    }
                }
            } else {
                if (OnRadar_00466dc0(px, py) ||
                    p->owner->field_ff == g_game->currentPlayer) {
                    FUN_004b7f90(surface,
                        FUN_004b7f30(g_game->field_147e7,
                            PlayerInfo_00466dc0_Get(p->player)->data->field_96),
                        x, y);
                }
            }
            i++;
            p = (Projectile_00466dc0*)((char*)p + 0x6b);
            q = (short*)((char*)q + 0x6b);
        } while (i < g_game->projectileCount);
    }

    g_game->field_142f0.bits.bit1 = 1;
}
