// SideDef: one side's definition (Thaldren's SideDef), the five-entry table at
// g_game+0x37f3d, stride 0x232, that LoadSideData (0x431a60) fills from
// gamedata\sidedata.tdf. Each SIDE<n> section gives the names, the font and
// the two colours, and its sub-sections the in-game panel rectangles; the
// fields are named after those keys. The one declaration for the files that
// share it.
#ifndef SIDE_DEF_H
#define SIDE_DEF_H

#include "../graphics/rect.h"

#pragma pack(push, 1)
struct SideDef {                        // 0x232 bytes
    char name[0x1e];                    // +0x00, "name"
    char namePrefix[4];                 // +0x1e, "nameprefix"
    char commander[0x20];               // +0x22, "commander"
    Rect logo;                          // +0x42, [LOGO]
    Rect energyBar;                     // +0x52
    Rect energyNum;                     // +0x62
    Rect metalBar;                      // +0x72
    Rect metalNum;                      // +0x82
    Rect totalUnits;                    // +0x92
    Rect totalTime;                     // +0xa2
    Rect energyMax;                     // +0xb2
    Rect metalMax;                      // +0xc2
    Rect energy0;                       // +0xd2
    Rect metal0;                        // +0xe2
    Rect energyProduced;                // +0xf2
    Rect energyConsumed;                // +0x102
    Rect metalProduced;                 // +0x112
    Rect metalConsumed;                 // +0x122
    Rect logo2;                         // +0x132
    Rect unitName;                      // +0x142
    Rect damageBar;                     // +0x152
    Rect unitEnergyMake;                // +0x162
    Rect unitEnergyUse;                 // +0x172
    Rect unitMetalMake;                 // +0x182
    Rect unitMetalUse;                  // +0x192
    Rect missionText;                   // +0x1a2
    Rect unitName2;                     // +0x1b2
    Rect damageBar2;                    // +0x1c2
    Rect nameBlock;                     // +0x1d2, [NAME]
    Rect description;                   // +0x1e2
    Rect reload[3];                     // +0x1f2, [RELOAD1] to [RELOAD3]
    int energyColor;                    // +0x222, "energycolor"
    int metalColor;                     // +0x226, "metalcolor"
    int sideNumber;                     // +0x22a, the section's index (Thaldren's nSideIndex)
    void* font;                         // +0x22e, the loaded "font" file
};
#pragma pack(pop)

#endif
