// Decompiled by space-bunny-free. Names are provisional.
// Binary search of the unit definition table at g_game+0x1439b for the entry
// whose name matches `name`, returning that entry's short id at +0x21e, or 0
// when there is none. The search starts one element in, so a one-entry table
// is never searched.
#include <string.h>

#pragma pack(push, 1)
struct UnitType_00488b10 {           // 0x249 bytes
    char unknown_0[0x20];
    char name[0x21e - 0x20];         // +0x20
    short id;                        // +0x21e
    char unknown_220[0x249 - 0x220];
};

struct Game_00488b10 {
    char unknown_0[0x1438f];
    int count;                       // +0x1438f
    char unknown_14393[8];
    UnitType_00488b10* defs;          // +0x1439b
};
#pragma pack(pop)

extern Game_00488b10* g_game;

// FUNCTION: 0x488b10
short __stdcall FUN_00488b10(char* name)
{
    int count = g_game->count;
    UnitType_00488b10* last = g_game->defs + count;
    UnitType_00488b10* lo = g_game->defs + 1;
    int n = (int)((char*)last - (char*)lo) / (int)sizeof(UnitType_00488b10);
    while (n > 0) {
        int i = n / 2;
        UnitType_00488b10* mid = lo + i;
        if (_strcmpi(mid->name, name) < 0) {
            lo = mid + 1;
            n = n - i - 1;
        } else {
            n = i;
        }
    }
    if (lo == last || _strcmpi(name, lo->name) != 0)
        lo = 0;
    if (lo)
        return lo->id;
    return 0;
}
