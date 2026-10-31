// Decompiled by Claude Opus 5.5. Names are provisional.
#include <stdio.h>

// Two functions with aligned frames (`and esp, -8`) under the default flags,
// no /Op: the doubles they pass and receive are enough for MSVC to align.

#include "../network/player_info.h"
#include "game.h"

extern Game* g_game;

#include "../util/hapi_bank.h"

// Loads the "Players" section of a saved game: the human player, the game
// clock, and each player's resources, statistics, logo, side and alliances
// from its "Player%i" section (a player without one gets no controller).
// FUNCTION: 0x466050
void __stdcall LoadPlayers(HapiBank* file)
{
    char name[16];
    file->OpenAccount("Players");
    g_game->localPlayer = file->GetIntegerItem("Human Player", 10);
    g_game->playerIndex = g_game->localPlayer;
    file->OpenNamedBox("GameTime");
    if (file->ReadBox(&g_game->lastTick, 0x1c) != 0x1c)
        return;
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        sprintf(name, "Player%i", i);
        if (file->OpenAccount(name)) {
            p->energy = file->GetDoubleItem("Energy", 0);
            p->metal = file->GetDoubleItem("Metal", 0);
            p->totalEnergyProduced = file->GetDoubleItem("TotalEnergyProduced", 0);
            p->totalMetalProduced = file->GetDoubleItem("TotalMetalProduced", 0);
            p->totalEnergyConsumed = file->GetDoubleItem("TotalEnergyConsumed", 0);
            p->totalMetalConsumed = file->GetDoubleItem("TotalMetalConsumed", 0);
            p->energyWasted = file->GetDoubleItem("EnergyWasted", 0);
            p->metalWasted = file->GetDoubleItem("MetalWasted", 0);
            p->energyStorageBonus = file->GetDoubleItem("PlayerEnergyStorage", 0);
            p->metalStorageBonus = file->GetDoubleItem("PlayerMetalStorage", 0);
            // The storage flag is bit 0 of flags. The value and the old word in
            // locals, as the original's bit-field store reads them.
            int storage = file->GetIntegerItem("AddPlayerStorage", 0);
            unsigned short f = p->flags;
            p->flags = (f & ~1) | (storage & 1);
            p->kills = file->GetIntegerItem("Kills", 0);
            p->losses = file->GetIntegerItem("Losses", 0);
            p->updateTime = file->GetIntegerItem("UpdateTime", 0);
            p->winLoseTime = file->GetIntegerItem("WinLoseTime", 0);
            p->displayTimer = file->GetIntegerItem("DisplayTimer", 0);
            p->info->color = file->GetIntegerItem("Logo", 0);
            p->info->side = file->GetIntegerItem("Side", 0);
            file->OpenNamedBox("Alliances");
            if (file->GetBoxSize() == 11)
                file->ReadBox(p->allied, 11);
            p->allied[i] = 1;
        } else {
            p->type = 0;
        }
    }
}

// Saves what LoadPlayers loads, for every player that has a controller.
// FUNCTION: 0x4662f0
void __stdcall SavePlayers(HapiBank* file)
{
    char name[16];
    file->OpenAccount("Players");
    file->SetIntegerItem("Human Player", g_game->localPlayer);
    file->OpenNamedBox("GameTime");
    file->WriteBox(&g_game->lastTick, 0x1c);
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (p->type) {
            sprintf(name, "Player%i", i);
            file->OpenAccount(name);
            file->SetDoubleItem("Energy", p->energy);
            file->SetDoubleItem("Metal", p->metal);
            file->SetDoubleItem("TotalEnergyProduced", p->totalEnergyProduced);
            file->SetDoubleItem("TotalMetalProduced", p->totalMetalProduced);
            file->SetDoubleItem("TotalEnergyConsumed", p->totalEnergyConsumed);
            file->SetDoubleItem("TotalMetalConsumed", p->totalMetalConsumed);
            file->SetDoubleItem("EnergyWasted", p->energyWasted);
            file->SetDoubleItem("MetalWasted", p->metalWasted);
            file->SetDoubleItem("PlayerEnergyStorage", p->energyStorageBonus);
            file->SetDoubleItem("PlayerMetalStorage", p->metalStorageBonus);
            file->SetIntegerItem("AddPlayerStorage", p->flags & 1);
            file->SetIntegerItem("Kills", p->kills);
            file->SetIntegerItem("Losses", p->losses);
            file->SetIntegerItem("UpdateTime", p->updateTime);
            file->SetIntegerItem("WinLoseTime", p->winLoseTime);
            file->SetIntegerItem("DisplayTimer", p->displayTimer);
            file->SetIntegerItem("Controller", p->type);
            file->SetIntegerItem("Logo", p->info->color);
            file->SetIntegerItem("Side", p->info->side);
            file->OpenNamedBox("Alliances");
            file->WriteBox(p->allied, 11);
        }
    }
}
