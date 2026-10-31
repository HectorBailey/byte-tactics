// DisplayContext, the display object g_display points at: the window and its
// class, the DIB and GDI objects, the DirectDraw surfaces, the clip and mode
// rectangles, the key queue, the frame counter and the palette table. One
// declaration for the modules that read it, at the offsets Thaldren's
// DisplayContext rows give (data/thaldren/structs.csv, size 0x828), with the
// Hungarian prefix dropped and the name display.cpp already uses where it has
// one. Records another module owns (the DIB surface, the DirectDraw objects,
// the surfaces, the open archive array) are a pointer here, so a module that
// reads the whole record keeps its own view.
#ifndef DISPLAY_CONTEXT_H
#define DISPLAY_CONTEXT_H

#include <windows.h>
#include "../gui/mouse_event.h"

struct Surface;
struct GafFrame;
// The bitmap font the font API reads at +0x204 (font.cpp defines it).
struct Font;

// Packed to 2: the frame counter at +0x1da, the mode rectangle at +0x1ea and
// the size at +0x1fa sit on 2-byte boundaries.
#pragma pack(push, 2)
struct DisplayContext {
    HINSTANCE hInstance;                     // +0x00
    int nCmdShow;                            // +0x04
    char* className;                         // +0x08
    char* title;                             // +0x0c
    char unknown_10[4];                      // +0x10, no reader in the exe
    unsigned short menuId;                   // +0x14
    char unknown_16[2];                      // +0x16, pads the window class to +0x18
    WNDCLASSA wc;                            // +0x18
    HWND hwnd;                               // +0x40
    HBITMAP dib;                             // +0x44
    HDC dc;                                  // +0x48
    HPALETTE hpalette;                       // +0x4c
    char unknown_50[0x80 - 0x50];            // +0x50, the DIB surface
    void (__cdecl *callback)(int, int, int); // +0x80
    void* ddraw;                             // +0x84, IDirectDraw
    void* primary;                           // +0x88, the primary surface
    void* back;                              // +0x8c, the back buffer
    void* clipper;                           // +0x90
    void* palette;                           // +0x94, the DirectDraw palette
    Surface* activeSurface;                  // +0x98
    int haveBackBuffer;                      // +0x9c
    int clipLeft;                            // +0xa0, the primary surface clip
    int clipTop;                             // +0xa4
    int clipRight;                           // +0xa8
    int clipBottom;                          // +0xac
    HFONT font;                              // +0xb0
    void* lastBackBuffer;                    // +0xb4
    char unknown_b8[4];                      // +0xb8
    Surface* overrideSurface;                // +0xbc
    unsigned char* alphaTable;               // +0xc0
    unsigned char* shadeTable;               // +0xc4
    unsigned char* lightTable;               // +0xc8
    unsigned char* grayTable;                // +0xcc
    unsigned char* blueTable;                // +0xd0
    int width;                               // +0xd4
    int height;                              // +0xd8
    int useOverrideSurface;                  // +0xdc
    int active;                              // +0xe0
    char unknown_e4[4];                      // +0xe4
    int tickScale;                           // +0xe8, ticks per second
    int wa_left;                             // +0xec, work area left
    // The state flags, as one word (the masks) or as bits (display.cpp's
    // DisplayFlags names; SetFullScreen takes bit 1).
    union {
        unsigned short word;
        struct {
            unsigned short opt : 1;          // bit 0
            unsigned short fullscreen : 1;   // bit 1
            unsigned short gdi : 1;          // bit 2
            unsigned short unknown_3 : 2;    // bits 3-4
            unsigned short has_c0 : 1;       // bit 5
            unsigned short has_c4 : 1;       // bit 6
            unsigned short has_c8 : 1;       // bit 7
            unsigned short has_cc : 1;       // bit 8
            unsigned short has_d0 : 1;       // bit 9
            unsigned short sound_opt : 1;    // bit 10
            unsigned short quitting : 1;     // bit 11
            unsigned short unknown_12 : 4;   // bits 12-15
        } bits;
    } flags;                                 // +0xf0
    int mappedKeyQueueCap;                   // +0xf2
    int mappedKeyQueue[30];                  // +0xf6
    int mappedKeyWriteIdx;                   // +0x16e
    int mappedKeyReadIdx;                    // +0x172
    char unknown_176[0x186 - 0x176];         // +0x176
    // The mouse: its event queue, the current event, and the cursor and its
    // thread (Thaldren's names; gui/mouse.cpp reads them).
    int mouseEventQueueCap;                  // +0x186
    MouseEvent* mouseEventQueue;             // +0x18a
    int mouseEventWriteIdx;                  // +0x18e
    int mouseEventReadIdx;                   // +0x192
    MouseEvent curMouseEvent;                // +0x196
    int cursorHideCount;                     // +0x1ae
    GafFrame* cursorGafFrame;                // +0x1b2
    int cursorBlitX;                         // +0x1b6
    int cursorBlitY;                         // +0x1ba
    Surface* saveMouse1;                     // +0x1be, the saved-background surfaces
    Surface* saveMouse2;                     // +0x1c2
    Surface* saveMouse3;                     // +0x1c6
    int mouseThreadActive;                   // +0x1ca, the thread's handle
    int cursorThreadEnabled;                 // +0x1ce
    int cursorOverlayEnabled;                // +0x1d2
    int mouseThreadStopReq;                  // +0x1d6
    int fpsAccum;                            // +0x1da
    unsigned int fpsTick;                    // +0x1de
    int fpsFrames;                           // +0x1e2
    int fpsRate;                             // +0x1e6
    int startLeft;                           // +0x1ea, the mode the display starts in
    int startTop;                            // +0x1ee
    int startRight;                          // +0x1f2
    int startBottom;                         // +0x1f6
    int startWidth;                          // +0x1fa
    int startHeight;                         // +0x1fe
    unsigned short videoFlags;               // +0x202
    Font* bitmapFont;                        // +0x204, the bitmap font
    int textFgColor;                         // +0x208, the set bit colour
    int textBgColor;                         // +0x20c, the clear bit colour
    int textSkipColor;                       // +0x210, the colour not written
    PALETTEENTRY entries[256];               // +0x214
    float scale;                             // +0x614
    void** items;                            // +0x618, the open archive files
    int itemCount;                           // +0x61c
    int availPhys;                           // +0x620, bytes of system memory
    int unknown_624;                         // +0x624
    char cwd[0x100];                         // +0x628
    char lastDir[0x100];                     // +0x728
};
#pragma pack(pop)

#endif
