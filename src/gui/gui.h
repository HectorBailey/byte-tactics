// Gui: the GUI system object (Thaldren's GuiRoot, 0xcf6 bytes) that sits at
// g_game+0x519: the fonts, the layer stack above it (+0x18), the cursor
// sprites and animation, the last mouse event, the index of the hot gadget
// (+0x60), the colour remap, the GUI file paths and the changed flag. The one
// declaration of the type, for the files that define or read it.
//
// The members are plain data. The only types named are Layer, Gaf (a font
// table: the GAF file LoadGafIntoSlot loads), Surface (the fallback draw
// surface) and MouseEvent (mouse_event.h: the last mouse event, and the one a
// knob drag began with). Every type or forward declaration the header adds
// moves the symbol ids of the small views it replaces (docs/c2-regalloc.md),
// and those views match in narrow windows: the files that broke when Gaf,
// MouseEvent and Surface came in each changed one filler block. The files
// that read the records behind the pointer members (the font tables, the
// cursor animation) keep their own view of them.
#ifndef GUI_H
#define GUI_H

#include "mouse_event.h"

#pragma pack(push, 1)

struct Gaf;
struct Surface;
struct Layer;

struct Gui {
    int font;                           // +0x00
    void* gaf;                          // +0x04
    Gaf* values[3];                     // +0x08, the font tables
    Gaf* language;                      // +0x14, the font table in use
    Layer* layer;                       // +0x18
    int cursorSharedFrame;              // +0x1c
    int cursorDefaultFrame;             // +0x20
    int cursorHoverFrame;               // +0x24
    void* src_28;                       // +0x28, the hover cursor animation
    void* src_2c;                       // +0x2c, the active cursor animation
    unsigned short cursorIndex;         // +0x30, the cursor animation reference
    unsigned short cursorValue;         // +0x32
    unsigned char cursorKind;           // +0x34
    char unknown_35[3];
    void* cursorSrc;                    // +0x38
    MouseEvent point;                   // +0x3c, the last mouse event
    int mouseKeyFlags;                  // +0x54
    int clickMode;                      // +0x58
    unsigned int cursorFlags;           // +0x5c, bit 0 set while the cursor animates
    int hotGadgetIndex;                 // +0x60
    // Must stay a second field after hotGadgetIndex (dialogs.cpp).
    int focus;                          // +0x64
    int hoverGadgetIndex;               // +0x68
    int prevHoverGadget;                // +0x6c
    int clearQuickKeys;                 // +0x70
    int cursor;                         // +0x74
    int knobDragging;                   // +0x78
    MouseEvent dragStart;               // +0x7c, the mouse when a knob drag began
    short dragKnobPos;                  // +0x94, the knob position then
    int time;                           // +0x96
    int animTimer;                      // +0x9a
    int inputEnabled;                   // +0x9e
    int dirty;                          // +0xa2 (nonzero = selection active)
    void* blinkWords;                   // +0xa6, the blinking words (file_list.cpp's BlinkWord array)
    int blinkCount;                     // +0xaa
    int blinkActive;                    // +0xae, nonzero while they are drawn
    unsigned int palette[0x100];        // +0xb2, the GUI palette copy (dialogs.cpp dest)
    char unknown_4b2[0x8b2 - 0x4b2];
    unsigned char colours[0x100];       // +0x8b2
    char unknown_9b2[0x9b6 - 0x9b2];
    char str_9b6[0x100];                // +0x9b6
    char str_ab6[0x100];                // +0xab6
    char str_bb6[0x110];                // +0xbb6
    int pathsReady;                     // +0xcc6
    int changed;                        // +0xcca
    int clickStatusCache;               // +0xcce
    Surface* fallback;                  // +0xcd2, the fallback draw surface
    char cachedBgName[0x20];            // +0xcd6, the cached background bitmap name
};

#pragma pack(pop)

#endif
