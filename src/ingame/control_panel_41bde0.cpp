// Decompiled by DeepSeek V4.1 Flash, finished by Claude Opus 5.5. Names are provisional.
// Steps the selected unit's build menu to the next page (wrapping to page 1),
// or closes it after the last page when toggle is set.
// Needed: without it the sub-object pointer is allocated to edx, not esi.
// Kept its own file: in control_panel.cpp the include set and prelude move
// that pointer from esi to edx.
#include <stdlib.h>

#include "../units/unit.h"

#pragma pack(push, 1)
struct Game {
    char unknown_0[0x14357];
    Unit* units;                         // +0x14357
    char unknown_1435b[0x37e9c - 0x1435b];
    unsigned short unitIndex;            // +0x37e9c
    char unknown_37e9e[0x37ebe - 0x37e9e];
    unsigned char ordersPanelFlags;      // +0x37ebe
};
#pragma pack(pop)

extern Game* g_game;
void __stdcall PlaySoundByName(char* name, int param);

// FUNCTION: 0x41bde0
void __stdcall StepBuildMenuPage(int toggle)
{
    unsigned short index = g_game->unitIndex;
    if (index != 0) {
        Unit* unit = &g_game->units[index];
        if (unit->unitDefIndex == 0)
            unit = 0;
        if (unit != 0) {
            if (toggle != 0) {
                if (!unit->buildMenuOpen) {
                    unit->buildMenuOpen = 1;
                    unit->buildMenuPage = 1;
                } else if (unit->buildMenuPage == unit->def->buildMenuPageCount - 1) {
                    unit->buildMenuOpen = 0;
                } else {
                    unit->buildMenuPage++;
                }
            } else {
                // Compared as page == buildMenuPageCount - 1; keeps the original load order.
                if (unit->buildMenuPage == unit->def->buildMenuPageCount - 1)
                    unit->buildMenuPage = 1;
                else
                    unit->buildMenuPage++;
                unit->buildMenuOpen = 1;
            }
            g_game->ordersPanelFlags |= 0x10;
        }
    }
    PlaySoundByName("nextbuildmenu", 0);
}
