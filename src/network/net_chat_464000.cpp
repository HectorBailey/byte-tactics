// Decompiled by Opus. Names are provisional.
#include "../game/game.h"

extern Game* g_game;
int ScrollToNextMessageUnit(void);

// Stays in its own file: the module's ScrollToNextMessageUnit would be inlined here, where the original calls it out of line.
// FUNCTION: 0x464000
void CycleMessageUnits(void)
{
    int i;
    for (i = 0; i < 30; i++)
        g_game->chatHudRing[i].flags &= ~0x20;
    if (ScrollToNextMessageUnit() == 0) {
        for (i = 0; i < 30; i++)
            g_game->chatHudRing[i].flags &= ~0x10;
        ScrollToNextMessageUnit();
    }
}
