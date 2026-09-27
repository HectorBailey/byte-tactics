// Decompiled by space-bunny-free. Names are provisional.
// Binary search of the unit definition table at g_game+0x1439b for the entry
// whose name matches, returning the entry or 0. It starts at the second
// entry, so the first one is never found, and its final comparison passes the
// arguments to _strcmpi the other way round from the loop's.
// The divisor is the literal 0x249, not sizeof: a size_t divisor promotes the
// difference to unsigned and MSVC picks a different divide magic.
#include <string.h>

#pragma pack(push, 1)
struct Elem_00488a50 {              // 0x249 bytes
    char unknown_0[0x20];
    char name[0x249 - 0x20];        // +0x20
};

struct Game_00488a50 {
    char unknown_0[0x1438f];
    int count;                      // +0x1438f
    char unknown_14393[8];
    Elem_00488a50* defs;            // +0x1439b
};
#pragma pack(pop)

extern Game_00488a50* g_game;

// FUNCTION: 0x488a50
Elem_00488a50* __stdcall FUN_00488a50(char* name)
{
    int count = g_game->count;
    Elem_00488a50* last = g_game->defs + count;
    Elem_00488a50* lo = g_game->defs + 1;
    int n = (int)((char*)last - (char*)lo) / 0x249;
    while (n > 0) {
        int i = n / 2;
        Elem_00488a50* mid = lo + i;
        if (_strcmpi(mid->name, name) < 0) {
            lo = mid + 1;
            n = n - i - 1;
        } else {
            n = i;
        }
    }
    if (lo != last && _strcmpi(name, lo->name) == 0)
        return lo;
    return 0;
}
