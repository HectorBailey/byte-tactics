// Decompiled by space-bunny-free. Names are provisional.
// Draws a list box's frame. FUN_004a15c0 gives the entry's rectangle; when no
// bitmap arrives, the "Listbox" piece is looked up in the object's GAF and, if
// found, the rectangle is grown by 3 on every side. The destination is the
// surface at entries+0xbc. When FUN_004a18c0 finds a background cell for the
// entry, the area is tiled with it through FUN_004c6b70 and only the bevel
// (FUN_004b0160) is drawn; with no cell and no bitmap the rectangle is filled
// (FUN_004bf6f0) and bevelled; with a bitmap set of one child or less the child
// is blitted at the origin; otherwise the set is laid out as a 3x3 border
// around the rectangle, rows 0/3/6 and columns 0/1/2 of the set, stepping by the
// first child's width and height, the last row and column pinned to the far
// edges. The colours are the object's bytes at +0x8b2 (dark), +0x8c3 (light)
// and +0x8c6 (fill).
//
// NOT MATCHED (check.py 24.6%, 627 of 631 bytes; 232 instructions against the
// original's 232).  The body is structurally complete: every block of the
// original is present here.  The whole function is off by a one-register
// rotation that starts at the very first call and propagates everywhere, so
// fixing the head is worth far more than any later block.
//
// 1. THE HEAD (0x4b0233 and 0x4b0237) is the blocker, and it resisted every
//    source shape tried.  The original evaluates the three arguments of
//    FUN_004a15c0 in the order arg2, arg3, arg1 and gives them edx, ecx and
//    then eax/edi:
//        mov edx, [esp+0x2c]   ; arg2, hoisted before the register saves
//        lea ecx, [esp+0x14]   ; &rect
//        push ebx / push ebp / push esi
//        mov esi, [esp+0x34]   ; arg1
//        push edi
//        push ecx              ; &rect
//        mov eax, [esi+0x18]
//        push edx              ; arg2
//        mov edi, [eax+4]      ; entries
//        push edi
//        call
//    This file instead starts `lea eax, [esp+0x14]`, then puts arg2 in ebx
//    (a saved register, loaded after the first push) and the GAF entries in
//    ecx/edx.  Because ebx is taken at the top, the callee-saved registers
//    rotate: bmp ends up in edi instead of ebx, and the three colour arguments
//    (eax/ecx/edx in the original) come out as ecx/edx/eax.  Spelling arg2 as
//    unsigned/short/long, passing &rect as a reference or through a pointer
//    local, giving the callee a void*/Rect& first parameter, hoisting
//    obj->holder->entries into a local or a static inline getter, moving the
//    surface or the cell fetch above the first call, and reordering the local
//    declarations were all tried; none changes the head, and `#include
//    <windows.h>` changes nothing here.  Note this is NOT a missing include:
//    the file has no window types and the head bytes are a pure allocation
//    choice.  The wanted shape is "arg2 into a volatile register, then &rect
//    into another, and only then walk the entries chain", so the allocator
//    must not have reserved ebx before the call.
// 2. The five local slots below the rect are permuted: the original has
//    row 0x10, x0 0x14, ypos 0x18, height 0x1c, h 0x20 (ascending in the order
//    the code touches them, the loop latch loading height before h); this file
//    loads h before height there.  Note this offset is measured with esp after
//    the four register saves, so 0x10 is the first slot under the rect.
// 3. The row index `(y >= height - h + 1) ? 6 : 3` compiles here to
//    setl/dec/and 3/add 3 (the same value, opposite polarity).  The original
//    emits setge/dec/and 0xfffffffd/add 6.  The ?:, a nested if/else, a
//    chained else-if and a hoisted `last` local were all tried; none flips it.
// 4. At the top of the 3x3 block the original pushes both arguments of
//    FUN_004b7f30 before the branch to the single-child case, this file pushes
//    one before and one after.
// Rejected: spelling the x position as an accumulating `x0 += x` (with the
// call taking x0) reaches 52.5% and its inner loop is the original's plus one
// store, but the original reloads x0 from its slot in every iteration and adds
// x (the only stores to that slot are in the set-up block), so the accumulating
// form draws the last column in the wrong place and is not what the exe does.
// Also rejected: a separate `y` local instead of reusing the dead `index`
// parameter as the y counter.  It matches the same 24.6% and 627 bytes, so it
// is not an improvement, but it is the more faithful reading of the original
// (which reloads arg2 from its stack home at 0x4b0392 rather than keeping it
// in a register), so it is what this file uses.

struct Rect_004b0230 {
    int x0;                          // +0x0
    int y0;                          // +0x4
    int x1;                          // +0x8
    int y1;                          // +0xc
};

struct Pic_004b0230 {
    unsigned short width;            // +0x0
    unsigned short height;           // +0x2
    char unknown_4[0x28 - 0x4];
};

struct Bits_004b0230 {
    unsigned short count;            // +0x0
    char unknown_2[0x28 - 0x2];
    Pic_004b0230* child[1];          // +0x28
};

struct Gaf_004b0230 {
    char unknown_0[0x14];
};

struct Cell_004b0230 {
    int step_x;                      // +0x0
    int step_y;                      // +0x4
};

struct Surface_004b0230 {
    int tiles_x;                     // +0x0
    int tiles_y;                     // +0x4
    char unknown_8[0xbc - 0x8];
};

struct Holder_004b0230 {
    char unknown_0[4];
    char* entries;                   // +0x4
};

struct Object_004b0230 {
    char unknown_0[4];
    Gaf_004b0230* gaf;               // +0x4
    char unknown_8[0x18 - 0x8];
    Holder_004b0230* holder;         // +0x18
    char unknown_1c[0x8b2 - 0x1c];
    unsigned char dark;              // +0x8b2
    char unknown_8b3[0x8c3 - 0x8b3];
    unsigned char light;             // +0x8c3
    char unknown_8c4[2];
    unsigned char fill;              // +0x8c6
};

void __stdcall FUN_004a15c0(char* entries, int index, Rect_004b0230* out);
int __stdcall FUN_004a18c0(char* entries, int index);
Bits_004b0230* __stdcall FUN_004b8d40(Gaf_004b0230* gaf, const char* name);
void __stdcall FUN_004c6b70(Surface_004b0230* dst, Cell_004b0230* cell, int x, int y);
Pic_004b0230* __stdcall FUN_004b7f30(Bits_004b0230* bits, int index);
void __stdcall FUN_004b7f90(Surface_004b0230* dst, Pic_004b0230* pic, int x, int y);
void __stdcall FUN_004b0160(Surface_004b0230* surface, Rect_004b0230* rect, int dark, int light, int fill);
int __stdcall FUN_004bf6f0(Surface_004b0230* surface, Rect_004b0230* rect, int colour);

// FUNCTION: 0x4b0230
void __stdcall FUN_004b0230(Object_004b0230* obj, int index, Bits_004b0230* bmp)
{
    Rect_004b0230 rect;
    FUN_004a15c0(obj->holder->entries, index, &rect);

    if (bmp == 0) {
        if (obj->gaf == 0)
            return;
        bmp = FUN_004b8d40(obj->gaf, "Listbox");
        if (bmp == 0)
            return;
        rect.x0 -= 3;
        rect.y0 -= 3;
        rect.x1 += 3;
        rect.y1 += 3;
    }

    Surface_004b0230* surface = *(Surface_004b0230**)((char*)obj->holder->entries + 0xbc);
    Cell_004b0230* cell = (Cell_004b0230*)FUN_004a18c0(obj->holder->entries, index);

    if (cell != 0) {
        for (int x = 0; x < surface->tiles_x; x += cell->step_x) {
            for (int y = 0; y < surface->tiles_y; y += cell->step_y) {
                FUN_004c6b70(surface, cell, x, y);
            }
        }
        FUN_004b0160(surface, &rect, obj->dark, obj->light, obj->fill);
    } else if (bmp == 0) {
        FUN_004bf6f0(surface, &rect, obj->fill);
        FUN_004b0160(surface, &rect, obj->dark, obj->light, obj->fill);
    } else if (bmp->count > 1) {
        Pic_004b0230* sub = FUN_004b7f30(bmp, 0);
        int w = sub->width, h = sub->height;
        int height, ypos, x0, row, y0, width, y;
        if (index != 0) {
            y0 = rect.y0;
            x0 = rect.x0;
        } else {
            y0 = 0;
            x0 = 0;
        }
        height = rect.y1 - rect.y0 + 1;
        width = rect.x1 - rect.x0 + 1;
        y = 0;
        while (y < height) {
            if (y != 0)
                row = (y >= height - h + 1) ? 6 : 3;
            else
                row = 0;
            if (y + h > height)
                y = height - h;
            ypos = y0 + y;
            for (int x = 0; x < width; x += w) {
                int col;
                if (x + w >= width) {
                    x = width - w;
                    col = 2;
                } else {
                    col = (x != 0) ? 1 : 0;
                }
                Pic_004b0230* tile = FUN_004b7f30(bmp, row + col);
                FUN_004b7f90(surface, tile, x0 + x, ypos);
            }
            y += h;
        }
    } else {
        FUN_004b7f90(surface, FUN_004b7f30(bmp, 0), 0, 0);
    }
}
