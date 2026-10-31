// Decompiled by DeepSeek V4.1 Flash. Names are provisional.
#include <windows.h>

#include "../units/unit.h"

// g_game is declared before game.h: with the header's symbols ahead of it, its
// id moves 0x408620 (docs/c2-regalloc.md).
struct Game;
extern Game* g_game;
#include "../game/game.h"

unsigned short __stdcall ChooseBuildOption(int player, Unit* unit);
void __stdcall QueueBuildOrder(char* name, Unit* unit, int count);

class BuildTimer {
public:
    char unknown_0[0x10];
    int player;                        // +0x10

    void TryIssueIdleFactoryBuildOrder(Unit* unit);
};

// FUNCTION: 0x408620
void BuildTimer::TryIssueIdleFactoryBuildOrder(Unit* unit)
{
    if (unit->list != 0)
        return;
    unsigned short idx = ChooseBuildOption(player, unit);
    if (idx != 0)
        QueueBuildOrder(g_game->unitDefs[idx].unitname, unit, 1);
}
