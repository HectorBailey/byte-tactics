// Decompiled by DeepSeek V4.1 Flash. Names are provisional.
#include <windows.h>

#include "../units/unit.h"

// game.h's Game, of which this method reads only the unit def array at
// +0x1439b; including that header here would move 0x408620.
#pragma pack(push, 1)
struct Game {
    char unknown_0[0x1439b];
    UnitDef* defs;                     // +0x1439b
};
#pragma pack(pop)

extern Game* g_game;

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
        QueueBuildOrder(g_game->defs[idx].unitname, unit, 1);
}
