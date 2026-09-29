// Decompiled by Sonnet 5.5, finished by space-bunny-free. Names are provisional.
// Fills a convex polygon with one colour, into `surface` or into the locked
// screen when `surface` is null. It finds the top and bottom vertices and the
// horizontal extent, rejects the polygon when it lies wholly outside the
// surface's clip rect, then walks the left edges (backwards from the top
// vertex to the bottom one) and the right edges (forwards) into a table of
// per-scanline x positions with 16.16 steps, and finally fills each scanline
// span clipped to the rect. Returns 1 when something was drawn.
//
// NOT MATCHED: 56.3%, 929 of 932 bytes. Every branch, both edge walks and the
// span fill follow the original exactly. What is still wrong is the frame: the
// original's is 0x14060 (twelve dword locals at [esp+0x10..0x3c], then the
// 0x30-byte locked-screen struct at 0x40 and the 0x800 x 0x28 span table at
// 0x70) against 0x1405c here, so every [esp+X] below the lock descriptor is 4
// bytes low. The two homes still missing are minX (the original keeps it in
// ebx AND in a slot, reloading the slot on every iteration of the bounds loop;
// here it is only in edi) and the edge index j (the original stores it at
// [esp+0x20] at the top of each walk and reloads it at the latch; here it
// stays in a register). Those two extra spills are also what demote minY from
// ebx to edi: the original has minY=edi, maxX=esi, minX=ebx, points=ebp,
// while this has minY=ebx, maxX=esi, minX=edi, points=ebp.
//
// Tried and rejected: hoisting `j` and `dy` to function scope on their own
// (48.0%), and giving the two span walks a `while (cnt)` countdown instead of
// a `for` over y (48.0%, and one byte over the original). Removing the
// `minIdx = 0` and `maxIdx = 0` initialisers is what took it from 49.4% to
// 56.3%: with them MSVC 5 folds the literal 0 into the `surface == 0` test and
// emits `xor ebp,ebp / cmp eax,ebp` where the original has `test eax,eax`,
// and two uninitialised index slots are exactly what the original has.
#include <string.h>

struct Rect_004c0330 {
    int left;
    int top;
    int right;
    int bottom;
};

class Class_004c6ae0 {
public:
    int unknown_0[2];
    int pitch;                         // +0x8
    unsigned char* pixels;             // +0xc
    int unknown_10[8];                 // 0x30 bytes, the lock descriptor

    Rect_004c0330* FUN_004c6ae0(Rect_004c0330* out);
};

struct Point_004c0330 {
    int x;
    int y;
};

struct Span_004c0330 {
    int left;                          // +0x0
    int right;                         // +0x4
    int unknown_8[8];
};

int __stdcall FUN_004c5e70(Class_004c6ae0* out);
int __stdcall FUN_004c5fa0(Class_004c6ae0* s);

// FUNCTION: 0x4c0330
int __stdcall FUN_004c0330(Class_004c6ae0* surface, Point_004c0330* points, int n, unsigned char color)
{
    Class_004c6ae0 screen;
    Rect_004c0330 clip;
    Span_004c0330 span[0x800];
    int locked;
    int minY;
    int maxY;
    int minX;
    int maxX;
    int minIdx;
    int maxIdx;
    int i;
    int j;
    int dy;

    if (surface == 0) {
        if (FUN_004c5e70(&screen) == 0)
            return 0;
        locked = 1;
        surface = &screen;
    } else {
        locked = 0;
    }

    minY = 999999;
    maxY = -999999;
    minX = 999999;
    maxX = -999999;
    for (i = 0; i < n; i++) {
        int y = points[i].y;
        if (y < minY) {
            minY = y;
            minIdx = i;
        }
        if (y > maxY) {
            maxY = y;
            maxIdx = i;
        }
        int x = points[i].x;
        if (x > maxX)
            maxX = x;
        if (x < minX)
            minX = x;
    }
    surface->FUN_004c6ae0(&clip);
    if (maxX < clip.left) {
        if (locked)
            FUN_004c5fa0(&screen);
        return 0;
    }
    if (minX > clip.right) {
        if (locked)
            FUN_004c5fa0(&screen);
        return 0;
    }
    if (maxY < clip.top) {
        if (locked)
            FUN_004c5fa0(&screen);
        return 0;
    }
    if (minY > clip.bottom) {
        if (locked)
            FUN_004c5fa0(&screen);
        return 0;
    }
    if (minY < clip.top)
        minY = clip.top;
    if (maxY > clip.bottom)
        maxY = clip.bottom;
    if (minY == maxY) {
        if (locked)
            FUN_004c5fa0(&screen);
        return 0;
    }

    Span_004c0330* sp = span;
    i = minIdx;
    do {
        j = i - 1;
        if (j < 0)
            j = n - 1;
        int y1 = points[j].y;
        int y0 = points[i].y;
        if (y1 > minY && y0 < y1) {
            int x0 = points[i].x;
            int x1 = points[j].x;
            dy = y1 - y0;
            int slope = ((x1 - x0) << 16) / dy;
            int fx = (x0 << 16) + 0xffff;
            if (y0 < minY) {
                fx += (minY - y0) * slope;
                y0 = minY;
            }
            if (y1 > maxY)
                y1 = maxY;
            for (int y = y0; y < y1; y++) {
                sp->left = fx >> 16;
                sp++;
                fx += slope;
            }
        }
        i = j;
    } while (i != maxIdx);

    sp = span;
    i = minIdx;
    do {
        j = i + 1;
        if (j >= n)
            j = 0;
        int y1 = points[j].y;
        int y0 = points[i].y;
        if (y1 > minY && y0 < y1) {
            int x0 = points[i].x;
            int x1 = points[j].x;
            dy = y1 - y0;
            int slope = ((x1 - x0) << 16) / dy;
            int fx = (x0 << 16) + 0xffff;
            if (y0 < minY) {
                fx += (minY - y0) * slope;
                y0 = minY;
            }
            if (y1 > maxY)
                y1 = maxY;
            for (int y = y0; y < y1; y++) {
                sp->right = fx >> 16;
                sp++;
                fx += slope;
            }
        }
        i = j;
    } while (i != maxIdx);

    Span_004c0330* s = span;
    for (i = minY; i < maxY; i++) {
        if (s->right > clip.right)
            s->right = clip.right;
        if (s->left < clip.left)
            s->left = clip.left;
        int w = s->right - s->left;
        if (w > 0)
            memset(surface->pixels + surface->pitch * i + s->left, color, w);
        s++;
    }
    if (locked)
        FUN_004c5fa0(&screen);
    return 1;
}
