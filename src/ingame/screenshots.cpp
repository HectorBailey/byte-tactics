// Decompiled by GPT-5.6-Terra, finished by Space Bunny Free. Names are provisional.
// g_game+0xc is the display context; the final test is bit 11 of its flags
// word (the high-byte bit 3 that App_PostWmDestroyOptionalMsg sets while a
// message box is up).
#include <stdio.h>

#include "../gui/mouse_event.h"
#include "../graphics/display_context.h"
#include "../map/mission.h"

#pragma pack(push, 1)
struct Game {
    char unknown_0[0xc];
    DisplayContext* displayContext;
    char unknown_10[0x519 - 0x10];
    char menu[0x10];
    char unknown_529[0x2c76 - 0x529];
    MouseEvent selected;
    char unknown_2c8e[0x37ebe - 0x2c8e];
    unsigned short ordersPanelFlags;
    char unknown_37ec0[0x37f2f - 0x37ec0];
    // The key test is a standalone if on bit 1 of this unsigned short bitfield.
    unsigned short screenBitA : 1;
    unsigned short screenBitB : 1;
    char unknown_37f31[0x38a37 - 0x37f31];
    int lastSimBudgetTick;
    char unknown_38a3b[0x38a51 - 0x38a3b];
    unsigned short otherFlags;
    char path[0x20c];
    char unknown_38c5f[0x391e9 - 0x38c5f];
    Mission* mapInfo;
    char unknown_391ed[0x391f5 - 0x391ed];
    void (__cdecl* callback)();
};
#pragma pack(pop)

extern Game* g_game;
extern char g_screenshotDirFormat[];
extern char g_screenshotPrefix[];

int PeekKey();
void PopKey();
void ToggleFullScreen();
void __stdcall PopUntilNamedLayout(int);
void __stdcall MakeDirectoryPath(char*);
int __stdcall GetTicks();
int __stdcall PeekMouseEvent(MouseEvent* out);
void __stdcall UpdateMenu(void*);
void UpdateCdCategoryByActivity();
void PlayNextSpeech();
int __stdcall PopMouseEvent(MouseEvent* out);
void UpdateTimers();
void __stdcall SaveScreenshot(char*, char*);

// FUNCTION: 0x499890
void MainFrameTick()
{
    MouseEvent first;
    MouseEvent second;
    char path[0x100];
    int key = PeekKey();

    if (key == 0x7e) {
        if (g_game->screenBitB) {
            PopKey();
            ToggleFullScreen();
        }
    }
    if ((g_game->ordersPanelFlags & 1) && key == 0xe3) {
        PopKey();
        PopUntilNamedLayout(0);
        g_game->ordersPanelFlags &= 0xfffe;
        if (g_game->mapInfo->GetGameType() != 3)
            g_game->otherFlags &= 0xfffe;
    }
    if (key == 0xd6) {
        PopKey();
        MakeDirectoryPath(g_game->path);
        sprintf(path, g_screenshotDirFormat, g_game->path);
        MakeDirectoryPath(path);
        SaveScreenshot(path, g_screenshotPrefix);
        g_game->lastSimBudgetTick = GetTicks();
    }
    PeekMouseEvent(&first);
    UpdateMenu(g_game->menu);
    PeekMouseEvent(&second);
    UpdateCdCategoryByActivity();
    PlayNextSpeech();
    if (first.msg == second.msg)
        PopMouseEvent(&g_game->selected);
    else if (first.msg == 0x205 || first.msg == 0x202)
        g_game->selected = first;
    else
        g_game->selected = second;
    UpdateTimers();
    if (!(g_game->displayContext->flags.word & 0x800))
        g_game->callback();
}
