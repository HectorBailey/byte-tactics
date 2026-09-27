// Decompiled by space-bunny-free. Names are provisional.
// Adds a team name to this object's 512-bit team mask. A unit definition with
// that name contributes only its own team bit; any other name merges in the
// global mask FUN_00488c50 returns. *known says which of the two happened.
#include <string.h>

#pragma pack(push, 1)
struct Def_00488d30 {                // 0x249 bytes
    char unknown_0[0x20];
    char name[0x21e - 0x20];         // +0x20
    unsigned short team;             // +0x21e
    char unknown_220[0x249 - 0x220];
};

struct Game_00488d30 {
    char unknown_0[0x1438f];
    int count;                       // +0x1438f
    char unknown_14393[8];
    Def_00488d30* defs;              // +0x1439b
};
#pragma pack(pop)

extern Game_00488d30* g_game;

class Class_00488d30 {
public:
    unsigned int mask[16];           // +0x0

    void FUN_00488d30(char* name, int* known);
};

void* __stdcall FUN_00488c50(char* name);

// FUNCTION: 0x488d30
void Class_00488d30::FUN_00488d30(char* name, int* known)
{
    int count = g_game->count;
    Def_00488d30* last = g_game->defs + count;
    Def_00488d30* lo = g_game->defs + 1;
    int n = (int)((char*)last - (char*)lo) / 0x249;
    while (n > 0) {
        int i = n / 2;
        Def_00488d30* mid = lo + i;
        if (_strcmpi(mid->name, name) < 0) {
            lo = mid + 1;
            n = n - i - 1;
        } else {
            n = i;
        }
    }
    Def_00488d30* def;
    if (lo == last || _strcmpi(name, lo->name) != 0)
        def = 0;
    else
        def = lo;
    unsigned short team = def ? def->team : 0;
    if (team) {
        mask[team >> 5] |= 1 << (team & 0x1f);
        *known = 1;
        return;
    }
    unsigned int* teams = (unsigned int*)FUN_00488c50(name);
    for (int i = 0; i < 16; i++)
        mask[i] |= teams[i];
    *known = 0;
}
