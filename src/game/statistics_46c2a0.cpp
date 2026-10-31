// Decompiled by Claude Opus 5.5. Names are provisional.
// <windows.h> is needed for the operand order of the player address
// ([eax + ebx + 0x1b63], g_game as the base); without it MSVC swaps them.
#include <windows.h>
#include <string.h>

// The aligned frame (`and esp, -8`) comes from the default flags, no /Op: the
// unsigned-to-float conversion needs an 8-byte stack temporary.

// The campaign object at g_game+0x391e9 (see 0x435da0.cpp).
#include "../map/mission.h"

#include "../network/player.h"

#include "../network/player_info.h"
#include "../game/game.h"

// Thaldren's online report records, the ones the stats DLL is handed: a player
// block with its allies, a score board with its entries, and one entry.
struct OnlineReportPlayerInfo {        // 0x18 bytes
    char* name;                        // +0x00
    int id;                            // +0x04
    int flags;                         // +0x08
    char* side;                        // +0x0c
    int numAllies;                     // +0x10
    OnlineReportPlayerInfo** allies;   // +0x14
};

struct OnlineReportStatEntry {         // 0x8 bytes
    char* name;                        // +0x0
    int value;                         // +0x4
};

struct OnlineReportScoreBoard {        // 0xc bytes
    int score;                         // +0x0
    int count;                         // +0x4
    OnlineReportStatEntry** scores;    // +0x8
};

extern Game* g_game;
extern OnlineReportPlayerInfo** g_onlineReportPlayers;
extern OnlineReportScoreBoard** g_onlineReportScoreBoards;
extern char* g_sideNames[2];           // "Arm", "Core"
extern char* g_scoreNames[9];          // "Kills", "Losses", ..., "I am Winner"

// Fills the player and score-board tables AllocScoreTables allocated (handed to
// the stats DLL by ReportGameEvent) for every player in the game, and returns
// how many there are.
// FUNCTION: 0x46c2a0
int FillScoreTables()
{
    // j declared first, so that p->allied[j] is addressed [p + j], and n
    // set before the allies pointer is read: both are needed for a match.
    int j;
    int count = 0;
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if ((p->active && (p->type == 1 || p->type == 2 || p->type == 3) && p->index != 10) || p->unitsCreated || p->active) {
            count++;
            g_onlineReportPlayers[i]->name = p->name;
            g_onlineReportPlayers[i]->id = p->id;
            g_onlineReportPlayers[i]->flags = 1;
            if ((p->active && p->type == 2) || (p->active && p->type == 3 && p->info->kind == 2))
                g_onlineReportPlayers[i]->flags |= 2;
            if (p->active && p->info->bit6)
                g_onlineReportPlayers[i]->flags |= 4;
            if (p->active && (p->info->flags_97 & 1))
                g_onlineReportPlayers[i]->flags |= 8;
            g_onlineReportPlayers[i]->side = g_sideNames[p->info->side];
            int n = 0;
            OnlineReportPlayerInfo** allies = g_onlineReportPlayers[i]->allies;
            OnlineReportPlayerInfo** a = allies;
            for (j = 0; j < 10; j++) {
                if (j != i && p->allied[j]) {
                    n++;
                    *a++ = g_onlineReportPlayers[j];
                }
            }
            g_onlineReportPlayers[i]->numAllies = n;
            for (; n < 10; n++)
                allies[n] = 0;
            Mission* c = g_game->mapInfo;
            // Unsigned, as Thaldren types dwGameTick; game.h's int keeps economy_401360.cpp's
            // signed compare.
            int score = (int)((unsigned int)g_game->gameTick / 60 * c->timeMul);
            score += (int)(p->kills * c->killMul);
            g_onlineReportScoreBoards[i]->score = score;
            g_onlineReportScoreBoards[i]->count = 9;
            OnlineReportStatEntry* s;
            s = g_onlineReportScoreBoards[i]->scores[0];
            s->name = g_scoreNames[0];
            s->value = p->kills;
            s = g_onlineReportScoreBoards[i]->scores[1];
            s->name = g_scoreNames[1];
            s->value = p->losses;
            s = g_onlineReportScoreBoards[i]->scores[2];
            s->name = g_scoreNames[2];
            s->value = (int)p->totalEnergyProduced;
            s = g_onlineReportScoreBoards[i]->scores[3];
            s->name = g_scoreNames[3];
            s->value = (int)p->totalMetalProduced;
            s = g_onlineReportScoreBoards[i]->scores[4];
            s->name = g_scoreNames[4];
            s->value = (int)p->energyWasted;
            s = g_onlineReportScoreBoards[i]->scores[5];
            s->name = g_scoreNames[5];
            s->value = (int)p->metalWasted;
            s = g_onlineReportScoreBoards[i]->scores[6];
            s->name = g_scoreNames[6];
            s->value = p->commanderKills;
            s = g_onlineReportScoreBoards[i]->scores[7];
            s->name = g_scoreNames[7];
            s->value = p->commanderLosses;
            s = g_onlineReportScoreBoards[i]->scores[8];
            s->name = g_scoreNames[8];
            // Bit 4 of the end-game flags, set when the victory conditions are met
            // (players_464290.cpp).
            s->value = (i == g_game->localPlayer || (p->active && p->type == 2)) ? (g_game->endGameFlags >> 4) & 1 : 0;
        }
    }
    return count;
}
