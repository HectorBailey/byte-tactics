// Decompiled by Sonnet 5.5. Names are provisional.
// Draws `text` (a bitmap font string ended by NUL or newline) at x, y into
// `dst`, or into the locked screen when `dst` is null. When `maxWidth` is not
// -1 and the text is wider, a copy is cut back one character at a time until
// it fits. The text's rectangle is checked against the surface's clip rect
// with FUN_004b6750 and only then drawn by the glyph blitter FUN_004ccf60,
// which takes the surface's pixels and pitch, the font, the text, the
// position and the display's three colour fields at +0x208, +0x20c, +0x210.
//
// NOT MATCHED: 59.6%, 626 of 617 bytes. The control flow, the inlined width
// helper (as in 0x4c1830.cpp), the truncation loop and the two draw paths
// follow the original. What differs is the register assignment: the original
// keeps `text` in ebp and its iterator in esi and spills the display pointer
// to [esp+0x10], where this puts the display pointer in ebp, and the
// truncation loop stores its terminator through a zero register instead of an
// immediate. Iterating over a copy, and do/for/while spellings of the
// truncation loop, did not change it.
#include <string.h>

struct Font_004c14f0 {
    unsigned char glyphs[1];            // +0x0, first byte of a glyph is its width
    char unknown_1[2];
    unsigned char first;                // +0x3, first character with a glyph
    unsigned short offsets[1];          // +0x4, glyph offsets from the font start
};

struct Game_004c14f0 {
    char unknown_0[0x204];
    Font_004c14f0* font;                // +0x204
    int colour1;                        // +0x208
    int colour2;                        // +0x20c
    int colour3;                        // +0x210
};

struct Rect_004c14f0 {
    int left;
    int top;
    int right;
    int bottom;
};

class Class_004c6ae0 {
public:
    int unknown_0[2];
    int pitch;                          // +0x8
    unsigned char* pixels;              // +0xc
    int unknown_10[8];                  // 0x30 bytes, the lock descriptor

    Rect_004c14f0* FUN_004c6ae0(Rect_004c14f0* out);
};

Game_004c14f0* FUN_004b6220(void);
int __stdcall FUN_004b6750(Rect_004c14f0* a, Rect_004c14f0* b);
int __stdcall FUN_004c5e70(Class_004c6ae0* out);
int __stdcall FUN_004c5fa0(Class_004c6ae0* s);
void __cdecl FUN_004ccf60(unsigned char* pixels, int pitch, Font_004c14f0* font, unsigned char* text,
                          int x, int y, int c1, int c2, int c3);

static inline int WidthText(Font_004c14f0* font, unsigned char* text)
{
    int width = 0;
    unsigned char* p = text;
    if (p && font) {
        for (; *p && *p != '\n'; p++) {
            unsigned char c = *p;
            if (c >= font->first) {
                int d = c - font->first;
                unsigned int off = 0;
                off = font->offsets[(unsigned short)d];
                if (off)
                    width += font->glyphs[off];
            }
        }
    }
    return width;
}

// FUNCTION: 0x4c14f0
void __stdcall FUN_004c14f0(Class_004c6ae0* dst, unsigned char* text, int x, int y, int maxWidth)
{
    Game_004c14f0* game = FUN_004b6220();
    int width = WidthText(game->font, text);
    if (maxWidth != -1 && width > maxWidth) {
        unsigned char buf[0x12c];
        strncpy((char*)buf, (char*)text, 0x12b);
        text = buf;
        unsigned char* end = buf + strlen((char*)buf) - 1;
        do {
            *end = 0;
            if (end == buf)
                break;
            end--;
            width = WidthText(game->font, buf);
        } while (width > maxWidth);
    }
    Rect_004c14f0 r;
    r.left = x;
    r.top = y;
    r.right = x + width;
    r.bottom = y + FUN_004b6220()->font->glyphs[0];
    if (dst == 0) {
        Class_004c6ae0 screen;
        if (FUN_004c5e70(&screen) != 0) {
            Rect_004c14f0 clip;
            screen.FUN_004c6ae0(&clip);
            if (FUN_004b6750(&r, &clip))
                FUN_004ccf60(screen.pixels, screen.pitch, game->font, text, x, y, game->colour1,
                             game->colour2, game->colour3);
            FUN_004c5fa0(&screen);
        }
    } else {
        Rect_004c14f0 clip;
        dst->FUN_004c6ae0(&clip);
        if (FUN_004b6750(&r, &clip))
            FUN_004ccf60(dst->pixels, dst->pitch, game->font, text, x, y, game->colour1,
                         game->colour2, game->colour3);
    }
}
