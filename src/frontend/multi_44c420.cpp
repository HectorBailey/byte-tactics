// Decompiled by DeepSeek V4.1 Flash. Names are provisional.
// Stays out of multi.cpp: its operand order needs symbol ids the module cannot give it (docs/split-modules.md).
// Handler/teardown for the "Options" menu. When the menu closes (its current
// gadget is -1) it frees the DESCLIST/PICLIST scratch globals (0x5129b4,
// 0x5129b8, 0x5129c4) and unregisters the queued unit rectangles; otherwise it
// dispatches the gadget named Load/Save/Reset/OK/Cancel.

// <stdio.h>, not <windows.h>: fixes the operand order and scheduling.
#include <stdio.h>

#pragma pack(push, 1)

#include "../gui/gadget.h"
#include "../gui/gui.h"
#include "../gui/layer.h"
#include "../network/unit_sync.h"
#include "../units/unit_def.h"

struct Record_005129b4 {               // 0x62-byte slider/picture record
    char unknown_0[0x52];
    int unitIndex;                     // +0x52 unit type index
    int previousMax;                   // +0x56 previous value
    int max;                           // +0x5a current value
    int peerEnabled;                   // +0x5e
};

union Flags_0044c420 {                 // the dword at +0x245
    unsigned int raw;
    struct {
        unsigned int low : 15;
        unsigned int flag : 1;         // bit 15
        unsigned int high : 16;
    } bits;
};

// game.h's unitDefs is a UnitDef* (units/unit_def.h, the same 0x249-byte
// record); this view stays because the restriction list reads bit 15 of the
// flag word as a 1-bit field, and the shift-and-test the original emits only
// comes out of a bitfield (folded onto UnitDef's plain flag word the function
// comes out three bytes short).
struct Item_0044c420 {                 // 0x249-byte unit type instance
    char unknown_0[0x20];
    char name[0x225];                  // +0x20
    Flags_0044c420 flags2;             // +0x245
};

#pragma pack(pop)

#include "../game/game.h"
#include "../graphics/surface.h"

extern Game* g_game;
extern int* g_unitRestrictPics;
extern Record_005129b4* g_unitRestrictEntries;
extern int* g_unitRestrictOldCounts;

Gadget* __stdcall FindGadgetChecked(Gadget* entries, char* name);
int __stdcall IsCurrentGadgetNamed(Gui* gui, char* name);
void __stdcall ClearSelectedGadget(Gui* obj);
void __stdcall SetDescListCleanupFlag(int param_1, int param_2);
void __stdcall MarkChanged(Gui* obj);
void __stdcall PlaySoundByName(char* name, int param_2);
void __stdcall UpdateUnitSliders(Gui* menu, int value);
void OpenLoadListDialog();
void OpenSaveGameDialog();
void __stdcall FreeSurface(Surface* obj);
void __cdecl GameFreeThunk(void* p);
int IsHostLocal();

// FUNCTION: 0x44c420
void __stdcall HandleRestrictionsClick(Gui* menu)
{
    int i;
    int n;
    int type;
    int* p;
    Item_0044c420* item;
    Gadget* desc;
    Gadget* pic;

    desc = FindGadgetChecked(menu->layer->entries, "DESCLIST");
    if (menu->hotGadgetIndex == -1) {
        i = 0;
        p = g_unitRestrictPics;
        if (desc->u.list.rowCount > 0) {
            do {
                if (*p != 0)
                    FreeSurface((Surface*)*p);
                p++;
                i++;
            } while (i < desc->u.list.rowCount);
        }
        GameFreeThunk(desc->u.list.rows);
        GameFreeThunk(g_unitRestrictPics);
        g_unitRestrictPics = 0;
        GameFreeThunk(desc->u.list.listText);
        SetDescListCleanupFlag((int)menu, 1);
        if (IsHostLocal() != 0) {
            for (i = 0; i < g_game->unitDefCount; i++) {
                type = g_unitRestrictEntries[i].unitIndex;
                if (type != 0) {
                    item = (Item_0044c420*)&g_game->unitDefs[type];
                    if (item->flags2.bits.flag) {
                    } else {
                        if (g_unitRestrictEntries[i].max == 0)
                            g_game->sync->DisallowUnit((UnitDef*)item);
                        else
                            g_game->sync->AllowUnit((UnitDef*)item);
                    }
                }
            }
        }
        GameFreeThunk(g_unitRestrictEntries);
        GameFreeThunk(g_unitRestrictOldCounts);
        GameFreeThunk(desc->u.list.filebuf);
        pic = FindGadgetChecked(menu->layer->entries, "PICLIST");
        if (pic != 0 && pic->u.list.rows != 0)
            GameFreeThunk(pic->u.list.rows);
        g_unitRestrictEntries = 0;
        return;
    }

    if (IsCurrentGadgetNamed(menu, "Load") != 0) {
        PlaySoundByName("Options", 0);
        OpenLoadListDialog();
        ClearSelectedGadget(menu);
        return;
    }
    if (IsCurrentGadgetNamed(menu, "Save") != 0) {
        PlaySoundByName("Options", 0);
        OpenSaveGameDialog();
        ClearSelectedGadget(menu);
        return;
    }
    if (IsCurrentGadgetNamed(menu, "Reset") != 0) {
        PlaySoundByName("Options", 0);
        for (i = 0; i < g_game->unitDefCount; i++) {
            type = g_unitRestrictEntries[i].unitIndex;
            if (type != 0) {
                if ((((Item_0044c420*)g_game->unitDefs)[type].flags2.raw & 0x10000) == 0)
                    g_unitRestrictEntries[i].max = 100;
                else
                    g_unitRestrictEntries[i].max = 0;
                if (g_unitRestrictEntries[i].max != g_unitRestrictEntries[i].previousMax) {
                    g_game->sync->SetUnitLimit(
                        &g_game->unitDefs[g_unitRestrictEntries[i].unitIndex],
                        g_unitRestrictEntries[i].max);
                }
            }
        }
        UpdateUnitSliders(menu, 0);
        MarkChanged(menu);
        ClearSelectedGadget(menu);
        return;
    }
    if (IsCurrentGadgetNamed(menu, "OK") != 0) {
        PlaySoundByName("Options", 0);
        return;
    }
    if (IsCurrentGadgetNamed(menu, "Cancel") != 0) {
        PlaySoundByName("Previous", 0);
        n = 0;
        for (i = 1; i < g_game->unitDefCount; i++) {
            // The original also tests the address of items[i].name (an array at
            // +0x20, so its address can never be null); kept for byte fidelity.
            if (((Item_0044c420*)g_game->unitDefs)[i].flags2.bits.flag) {
            } else if (((Item_0044c420*)g_game->unitDefs)[i].name != 0) {
                g_game->sync->SetUnitLimit(
                    &g_game->unitDefs[i], g_unitRestrictOldCounts[n]);
                n++;
            }
        }
        return;
    }
    if (menu->hotGadgetIndex != -1)
        ClearSelectedGadget(menu);
}
