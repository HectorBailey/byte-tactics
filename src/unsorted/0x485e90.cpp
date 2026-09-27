// Decompiled by space-bunny-free. Names are provisional.
// Builds a unit of type `unitType`: the three leg objects (an inline
// constructor that only stores the vtable) are created, the type index is
// recorded, then the init helpers run. When the unit type's flag at +0x22f
// is 1 the unit's first member is replaced by a 0x2f-byte object built from
// the unit (that is FUN_00485e50 inlined here).
//
// The middle three arguments arrive as one 12-byte struct, which FUN_00485a40
// also takes by value, so the call copies the parameter into the argument slot
// rather than pushing its three fields one by one. That is what decides the
// register choice in the argument setup.

class Class_0043dc00 {
public:
    char unknown_0[0x2f];

    Class_0043dc00(void* unit);
};

class Class_00481490 {
public:
    char unknown_4[0x1c - 4];

    Class_00481490() { }

    virtual void FUN_00481490(int enable);
    virtual void FUN_004814b0(int a);
};

#pragma pack(push, 1)
struct UnitType_00485e90 {
    char unknown_0[0x210];
    short field_210;                   // +0x210
    char unknown_212[0x22f - 0x212];
    char field_22f;                    // +0x22f
    char unknown_230[0x249 - 0x230];
};

struct Vec_00485a40 {
    int x;                             // +0x0
    int y;                             // +0x4
    int z;                             // +0x8
};

struct Game_00485e90 {
    char unknown_0[0x1439b];
    UnitType_00485e90* defs;           // +0x1439b
};

struct Unit_00485e90 {
    Class_0043dc00* obj;               // +0x0
    char unknown_4[0x8 - 0x4];
    Class_00481490 legs[3];            // +0x8
    char unknown_5c[0x66 - 0x5c];
    short field_66;                    // +0x66
    char unknown_68[0x92 - 0x68];
    UnitType_00485e90* type;           // +0x92
    char unknown_96[0xa6 - 0x96];
    unsigned short field_a6;           // +0xa6
};
#pragma pack(pop)

extern Game_00485e90* g_game;

void __stdcall FUN_00485a40(Unit_00485e90* unit, Vec_00485a40 v, int flag);
void __stdcall FUN_00485d40(Unit_00485e90* unit);
void __stdcall FUN_0049e070(Unit_00485e90* unit);
void __stdcall FUN_00437840(Unit_00485e90* unit);

// FUNCTION: 0x485e90
void __stdcall FUN_00485e90(unsigned short unitType, Vec_00485a40 v, int flag,
                            Unit_00485e90* unit)
{
    UnitType_00485e90* def = &g_game->defs[unitType];

    if (unit) {
        for (int i = 0; i < 3; i++) {
            unit->legs[i].Class_00481490::Class_00481490();
        }
    }
    unit->field_a6 = unitType;
    FUN_00485a40(unit, v, flag);
    FUN_00485d40(unit);
    FUN_0049e070(unit);
    FUN_00437840(unit);
    if (def->field_22f == 1) {
        unit->obj = new Class_0043dc00(unit);
        unit->field_66 = unit->type->field_210;
    }
}
