// Decompiled by Sonnet 5.5. Names are provisional.
// Redraws the software mouse cursor: reads the cursor position, computes the
// cursor's top left (position minus the sprite's origin), rebuilds the saved
// background and the composed image in the three off-screen surface
// descriptors at +0x1be, +0x1c2 and +0x1c6 (each resized to the sprite),
// restores what the old cursor covered, draws the sprite, and hands the union
// of the old and new cursor rectangles to FUN_004c60d0 to be shown.
//
// NOT MATCHED: 71.7%, size exact. The frame is 0x58 in the original and 0x64
// here: the 0x30-byte lock descriptor passed to FUN_004c5ff0 (at [esp+0x2c] in
// the original) shares its stack space with the union rect and one of the
// other rects, which are only written after it is dead, while here every local
// gets its own slot. Scoping the descriptor in a block and declaring the rects
// after it did not make MSVC overlap them. The rest (call order and
// arguments, field offsets) follows the original; FUN_004cbbe0 has no source
// yet, so its argument types are guesses.
#include <windows.h>

#pragma pack(push, 1)
struct Sprite_004c25e0 {
    unsigned short width;              // +0x0
    unsigned short height;             // +0x2
    short dx;                          // +0x4
    short dy;                          // +0x6
};

struct Desc_004c25e0 {
    int width;                         // +0x0
    int height;                        // +0x4
    int pitch;                         // +0x8
};

struct Rect_004c25e0 {
    int left;
    int top;
    int right;
    int bottom;
};

struct App_004c25e0 {
    char unknown_0[0x196];
    int cursorX;                       // +0x196
    int cursorY;                       // +0x19a
    char unknown_19e[0x1b2 - 0x19e];
    Sprite_004c25e0* sprite;           // +0x1b2
    int savedX;                        // +0x1b6
    int savedY;                        // +0x1ba
    Desc_004c25e0* saved;              // +0x1be
    Desc_004c25e0* under;              // +0x1c2
    Desc_004c25e0* work;               // +0x1c6
    char unknown_1ca[0x1d2 - 0x1ca];
    int enabled;                       // +0x1d2
};
#pragma pack(pop)

struct Info_004c25e0 {
    int data[11];
};

int __stdcall FUN_004c5ff0(Info_004c25e0* info);
void __cdecl FUN_004cbbe0(void* a, void* b, int x, int y);
void __stdcall FUN_004c69c0(Desc_004c25e0* d);
void __stdcall FUN_004b7f90(Desc_004c25e0* d, Sprite_004c25e0* s, int x, int y);
int __stdcall FUN_004c60d0(Rect_004c25e0* out, Rect_004c25e0* a, Rect_004c25e0* b);

// FUNCTION: 0x4c25e0
void __stdcall FUN_004c25e0(App_004c25e0* app)
{
    Info_004c25e0 info;
    Rect_004c25e0 r2;
    Rect_004c25e0 r1;
    Rect_004c25e0 r3;
    POINT pt;
    if (app->enabled == 0)
        return;
    if (FUN_004c5ff0(&info) == 0)
        return;
    GetCursorPos(&pt);
    int x = pt.x;
    int y = pt.y;
    app->cursorX = x;
    app->cursorY = y;
    x -= app->sprite->dx;
    y -= app->sprite->dy;
    app->under->width = app->sprite->width;
    app->under->height = app->sprite->height;
    app->under->pitch = app->sprite->width;
    app->work->width = app->sprite->width;
    app->work->height = app->sprite->height;
    app->work->pitch = app->sprite->width;
    FUN_004cbbe0(app->under, &r3, -x, -y);
    FUN_004cbbe0(app->under, app->saved, app->savedX - x, app->savedY - y);
    FUN_004cbbe0(app->work, app->under, 0, 0);
    FUN_004c69c0(app->work);
    FUN_004b7f90(app->work, app->sprite, app->sprite->dx, app->sprite->dy);
    FUN_004cbbe0(app->saved, app->work, x - app->savedX, y - app->savedY);
    r1.left = app->savedX;
    r1.top = app->savedY;
    r1.right = app->savedX + app->saved->width;
    r1.bottom = app->saved->height + app->savedY;
    r2.left = x;
    r2.top = y;
    r2.right = x + app->sprite->width;
    r2.bottom = y + app->sprite->height;
    FUN_004cbbe0(&r3, app->saved, app->savedX, app->savedY);
    FUN_004cbbe0(&r3, app->work, x, y);
    app->saved->width = app->sprite->width;
    app->saved->height = app->sprite->height;
    app->saved->pitch = app->sprite->width;
    FUN_004cbbe0(app->saved, app->under, 0, 0);
    app->savedX = x;
    app->savedY = y;
    FUN_004c60d0(&r3, &r1, &r2);
}
