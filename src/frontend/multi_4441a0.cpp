// Decompiled by Claude Opus 5.5. Names are provisional.
// Stays out of multi.cpp: a gap region (docs/split-modules.md).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../gui/gadget.h"
#include "../gui/layer.h"
#include "../gui/gui.h"

#include "../network/provider_conn.h"

#include "../network/link_info.h"

#include "../game/game.h"

extern Game* g_game;
// GLOBAL: 0x5127c8
extern LinkInfo g_linkInfo[];

void __cdecl GameFreeThunk(void* p);
int __stdcall IsCurrentGadgetNamed(void* menu, char* name);
Gadget* __stdcall FindGadgetChecked(void* entries, char* name);
int __stdcall FindGadgetIndex(void* entries, const char* name, int flag);
int __stdcall SelectConnection(int index);
void __stdcall PlaySoundByName(char* name, int flag);
void __stdcall SetCursorMode(int value);
int __stdcall OnlineProcessButtonCommand(int button, char* message, unsigned int size);
void OnlineUnload();
char* __stdcall Translate(char* text);
void __stdcall OpenMessageBox(void* menu, char* text, int size, int param_4, int param_5);
void __stdcall ClearSelectedGadget(void* menu);
void OpenOptionsPanel();

// The SELPROV.GUI menu's handler.
// FUNCTION: 0x4441a0
void __stdcall HandleSelectProviderClick(Gui* menu)
{
    Gadget* entries = menu->layer->entries;
    int i;
    // Unused id and cur, and the names link/message/result/msg, set the stack slot order.
    int id;
    int cur;
    if (menu->hotGadgetIndex == -1) {
        if (g_game->sessions != 0) {
            GameFreeThunk(g_game->sessions);
            for (i = 0; i < 10; i++) {
                if (g_game->conns[i].pConnectionBlob != 0) {
                    GameFreeThunk(g_game->conns[i].pConnectionBlob);
                    g_game->conns[i].pConnectionBlob = 0;
                }
            }
            GameFreeThunk(g_game->conns);
            GameFreeThunk(g_game->data[0]);
            g_game->sessions = 0;
            g_game->data[0] = 0;
        }
        return;
    }
    if (IsCurrentGadgetNamed(menu, "DPLAY") || IsCurrentGadgetNamed(menu, "SELECT")) {
        SelectConnection(FindGadgetChecked(entries, "DPLAY")->u.list.selected);
        g_game->frontendSubstateRequest = 2;
        PlaySoundByName("BigButton", 0);
        return;
    }
    if (_strnicmp(entries[menu->hotGadgetIndex].name, "SERVICE", 7) == 0) {
        PlaySoundByName("BigButton", 0);
        int link = atoi(entries[menu->hotGadgetIndex].name + 7);
        char message[0x140];
        message[0] = 0;
        int result = 2;
        try {
            SetCursorMode(0x14);
            result = OnlineProcessButtonCommand(g_linkInfo[link].id, message, sizeof(message));
            SetCursorMode(0x13);
        } catch (...) {
            SetCursorMode(0x13);
        }
        if (result == 0) {
            OnlineUnload();
            exit(0);
            return;
        }
        if (message[0] == 0)
            sprintf(message, "The %s service failed or took%stoo long. Please see the readme%sfile for more information.",
                    g_linkInfo[link].name, "\n\n", "\n\n");
        char* msg = Translate(message);
        OpenMessageBox(menu, msg, sizeof(message), 1, 1);
        ClearSelectedGadget(menu);
        return;
    }
    if (FindGadgetIndex(entries, "PREVMENU", 0xe) == menu->hotGadgetIndex) {
        g_game->frontendSubstateRequest = 3;
        PlaySoundByName("Previous", 0);
        return;
    }
    if (IsCurrentGadgetNamed(menu, "SETTINGS")) {
        PlaySoundByName("Options", 0);
        ClearSelectedGadget(menu);
        OpenOptionsPanel();
        return;
    }
    ClearSelectedGadget(menu);
}
