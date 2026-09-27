// Decompiled by space-bunny-free. Names are provisional.
#include <stdio.h>

struct Unit;

// std::vector<Unit*> exactly as MSVC 5's <vector> declares it: _Ufill is
// protected, so a derived class is the only way to reach it. Declared, not
// defined, so the decorated name resolves to the out-of-line 0x406c40.
namespace std {
template <class T> class allocator { };
template <class T, class A = allocator<T> > class vector {
public:
    typedef T value_type;
protected:
    T* _Myfirst;                      // +0x0
    T* _Mylast;                       // +0x4
    void _Ufill(T* first, unsigned int count, const value_type& val);
};
}

struct Filler_00488310 : std::vector<Unit*> {
    static void Fill(Filler_00488310* v, Unit** first, unsigned int count, Unit* const& val)
    {
        v->_Ufill(first, count, val);
    }
};

struct Vec_00488310 {
    char flag;                    // +0x0
    char pad[3];
    int* ids;                     // +0x4
};

// FUN_00487bf0's third argument, as 0x487af0 uses it.
struct Table_00488310 {
    char unknown_0[4];
    int* ids;                         // +0x4
};

#pragma pack(push, 1)
struct Pos_00488310 {                // 12 bytes, by value
    int x;
    int y;
    int z;
};

struct Item_00488310 {                // 0x249 bytes
    char unknown_0[0x21e];
    unsigned short id;                // +0x21e
    char unknown_220[0x249 - 0x220];
};

struct Player_00488310 {              // 0x14b bytes
    int active;                       // +0x0
    char unknown_4[0x73 - 4];
    unsigned char type;               // +0x73
    char unknown_74[0x146 - 0x74];
    unsigned char f146;               // +0x146
    char unknown_147[0x14b - 0x147];
};

struct Entry_00488310 {               // 0x24 bytes
    char* name;                       // +0x0
    char* unknown_4;                  // +0x4
    char* extra;                      // +0x8
    Pos_00488310 pos;                 // +0xc
    short f18;                        // +0x18
    short f1a;                        // +0x1a
    char unknown_1c[0x22 - 0x1c];
    unsigned char player;             // +0x22
    unsigned char flags;              // +0x23
};

class Class_00435100 {
public:
    char unknown_0[0xdac];
    Entry_00488310* list;             // +0xdac
    int count;                        // +0xdb0
};

class Class_004904b0 {
public:
    char unknown_0[0x88];
    int field_88;

    void FUN_004904b0();
};

struct Def_00488310 {
    char unknown_0[0x1fa];
    int f1fa;                         // +0x1fa
};

struct Unit {
    char unknown_0[0x66];
    short f66;                        // +0x66
    char unknown_68[0x92 - 0x68];
    Def_00488310* def;                // +0x92
    char unknown_96[0x108 - 0x96];
    short f108;                       // +0x108
    char unknown_10a[0x110 - 0x10a];
    int flags;                        // +0x110
};

struct Game_00488310 {
    char unknown_0[0x1b63];
    Player_00488310 players[10];      // +0x1b63
    char unknown_2851[0x391e9 - 0x2851];
    Class_00435100* net;              // +0x391e9
    Class_004904b0* mission;          // +0x391ed
};
#pragma pack(pop)

extern Game_00488310* g_game;

Item_00488310* __stdcall FUN_00488a50(const char* name);
void __stdcall FUN_0047ddc0(Item_00488310* type, Pos_00488310* pos);
Unit* __stdcall FUN_00485f50(unsigned char player, unsigned short id,
                                      Pos_00488310 pos, int a, int b, int c);
void __stdcall FUN_00487bf0(Unit* unit, char* text, Table_00488310* table);
void __stdcall FUN_004b6290(char* message);

static inline bool BadPlayer(unsigned char pl)
{
    if (pl >= 10)
        return true;
    Player_00488310* p = &g_game->players[pl];
    return p->active == 0
        || (p->type != 1 && p->type != 2 && p->type != 3)
        || p->f146 == 10;
}

// FUNCTION: 0x488310
void __cdecl FUN_00488310()
{
    char src[4];
    char buf[0x40];
    int off = 0;
    Vec_00488310 vec;
    int count = g_game->net->count;
    vec.flag = src[3];
    Unit** units = new Unit*[count < 0 ? 0 : count];
    Filler_00488310::Fill((Filler_00488310*)&vec, units, count, (Unit*)&off);
    Unit** last = units + count;
    Unit** end = last;
    if (g_game->net->count > 0) {
        for (int i = 0; i < g_game->net->count; i++) {
            Entry_00488310* e = &g_game->net->list[i];
            Item_00488310* item = FUN_00488a50(e->name);
            if (item == 0) {
                units[i] = 0;
                continue;
            }
            unsigned char pl = (unsigned char)(e->player - 1);
            if (BadPlayer(pl)) {
                sprintf(buf, "Player number %d invalid for unit %s", e->player, e->name);
                FUN_004b6290(buf);
            }
            FUN_0047ddc0(item, &e->pos);
            Unit* u = FUN_00485f50((unsigned char)(e->player - 1), item->id, e->pos, 1, 1, 0);
            if (u) {
                u->flags = (u->flags & 0x7fff) | ((e->flags & 0x80) << 8);
                u->f108 = (unsigned short)((unsigned)(u->def->f1fa * e->f1a) / 100);
                u->f66 = e->f18;
                units[i] = u;
            }
        }
    }
    for (int j = 0; j < g_game->net->count; j++) {
        Entry_00488310* e = &g_game->net->list[j];
        if (e->extra && units[j])
            FUN_00487bf0(units[j], e->extra, (Table_00488310*)&vec);
    }
    if (g_game->net->count <= 0)
        g_game->mission->FUN_004904b0();
    delete[] units;
}
