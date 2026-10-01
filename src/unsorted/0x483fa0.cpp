// Decompiled by deepseek-v4.1-flash, finished by GPT-6, edited by deepseek-v4.1, finished by deepseek-v4.1-flash, finished by mimo-v2.6-pro. Names are provisional.
// mimo-v2.6-pro retry: best 62.4% (1049 bytes against 1046). This session took
// the prologue to the original's arithmetic order (px, py, rx, ry, w1, w2,
// rem1, rem2) with int locals vw = g_game->viewW and vh = g_game->viewH so the
// rem expressions reuse the registers holding viewW/viewH instead of reloading
// (61.1%), then rebuilt block 3 as `int n = w2; int y = ay; unsigned short s =
// stride; int stride2 = s * 2; int offset = (py * s + px) * 2;` with the inner
// counter inside `if (m > 0) { int x = ax; ... }`, which restores the original
// `and eax, 0xffff` mask on stride, the `mov eax, edx` copy and the lea order
// (62.4%). Parenthesised p2 offsets and an uninitialised `int py;` declaration
// changed nothing.
// WHAT STILL DIFFERS: (1) a three-way slot rotation: ours py 0x1c, ay 0x20, ax
// 0x24 versus the original ay 0x1c, ax 0x20, py 0x24 (w1 0x10, px 0x14, w2
// 0x18, stride 0x28, p2 0x2c, rem1 0x30, rem2 0x34, n 0x38, stride2 0x3c all
// match); (2) block 1 p1/p2: the original shares the offset py*stride+px in
// eax and keeps mapValues in edx (`add eax, ebp; lea edx, [edx+eax*2-2]` at
// 0x4840b7), ours computes px+py*stride into ebp and reloads mapValues for p2,
// so one operand order breaks the CSE; (3) block 3 loop plumbing: the original
// keeps w1 in ebp and reloads it after the inner loop (`mov ebp, [esp+0x10]`
// at 0x484384) with `test ebp, ebp` at the outer top, ours reloads w1 at the
// loop top into edx and carries an entry `jmp`; (4) scattered register colours:
// tail imul edx vs imul esi for py, block 2 `add edx, [esp+0x14]` memory fold
// versus an explicit px load (ours keeps px in a register), w2 in eax versus
// esi at block 3 entry, and the block 3 preheader store interleave (original
// loads ay, stores n, stores y; ours stores n first). Declaration-order sweeps
// of ax/ay/py did not move the slot rotation; the remaining levers are the
// block 1 p2 operand order and the block 3 counter home.
#include <windows.h>

#pragma pack(push, 1)
struct IconSet_00483fa0 {
    char unknown_0[4];
    unsigned char* data;               // +0x4
};

struct Game_00483fa0 {
    char unknown_0[0x14233];
    int mapWidth;                      // +0x14233
    char unknown_14237[0x14283 - 0x14237];
    IconSet_00483fa0* iconSet;         // +0x14283
    char unknown_14287[0x1428b - 0x14287];
    unsigned short* mapValues;         // +0x1428b
    char unknown_1428f[0x1431f - 0x1428f];
    int scrollX;                       // +0x1431f
    int scrollY;                       // +0x14323
    char unknown_14327[0x37e27 - 0x14327];
    int viewX;                         // +0x37e27
    int viewY;                         // +0x37e2b
    char unknown_37e2f[0x37e37 - 0x37e2f];
    int viewW;                         // +0x37e37
    int viewH;                         // +0x37e3b
};

struct Bitmap_00483fa0 {
    unsigned short width;              // +0x0
    unsigned short height;             // +0x2
    short dx;                          // +0x4
    short dy;                          // +0x6
    unsigned char colour;              // +0x8
    unsigned char flag9;               // +0x9
    unsigned char count;               // +0xa
    unsigned char kind;                // +0xb
    int unknown_c;                     // +0xc
    unsigned char* data;               // +0x10
    int unknown_14;                    // +0x14
};
#pragma pack(pop)

extern Game_00483fa0* g_game;

void __stdcall FUN_004b8150(void* dst, Bitmap_00483fa0* bmp, int x, int y);
void __stdcall FUN_004c6e70(void* dst, int x, int y, unsigned char* pix);

// FUNCTION: 0x483fa0
void __stdcall FUN_00483fa0(void* surface)
{
    int ax = g_game->viewX;
    int sx = g_game->scrollX;
    int ay = g_game->viewY;
    int sy = g_game->scrollY;
    int px = sx / 32;
    int py = sy / 32;
    int rx = sx - px * 32;
    int ry = sy - py * 32;
    int vw = g_game->viewW;
    int w1 = (vw + rx) / 32;
    int vh = g_game->viewH;
    int w2 = (vh + ry) / 32;
    int rem1 = vw - w1 * 32 + rx;
    int rem2 = vh - w2 * 32 + ry;
    if (rem1 != 0)
        w1++;
    if (rem2 != 0)
        w2++;
    int stride = g_game->mapWidth / 2;

    Bitmap_00483fa0 bmp;
    bmp.width = 32;
    bmp.height = 32;
    bmp.dx = 0;
    bmp.dy = 0;
    bmp.flag9 = 0;
    bmp.count = 0;

    if (rx != 0 || rem1 != 0) {
        unsigned short* p1 = g_game->mapValues + py * stride + px;
        unsigned short* p2 = g_game->mapValues + py * stride + px + w1 - 1;
        int y = ay;
        int n = w2;
        if (n > 0) do {
            if (rx != 0) {
                bmp.data = g_game->iconSet->data + *p1 * 0x400;
                FUN_004b8150(surface, &bmp, ax - rx, y - ry);
            }
            if (rem1 != 0) {
                bmp.data = g_game->iconSet->data + *p2 * 0x400;
                FUN_004b8150(surface, &bmp, w1 * 32 + ax - rx - 32, y - ry);
            }
            p1 += stride;
            p2 += stride;
            y += 32;
        } while (--n);
    }

    if (ry != 0 || rem2 != 0) {
        unsigned short* p1 = g_game->mapValues + py * stride + px;
        unsigned short* p2 = g_game->mapValues + (py + w2 - 1) * stride + px;
        int n = w1;
        int x = ax;
        if (n > 0) do {
            if (ry != 0) {
                bmp.data = g_game->iconSet->data + *p1 * 0x400;
                FUN_004b8150(surface, &bmp, x - rx, ay - ry);
            }
            if (rem2 != 0) {
                bmp.data = g_game->iconSet->data + *p2 * 0x400;
                FUN_004b8150(surface, &bmp, x - rx, w2 * 32 + ay - ry - 32);
            }
            p1++;
            p2++;
            x += 32;
        } while (--n);
    }

    if (rx != 0) {
        w1--;
        ax += 32 - rx;
        px++;
    }
    if (ry != 0) {
        w2--;
        ay += 32 - ry;
        py++;
    }
    if (rem1 != 0)
        w1--;
    if (rem2 != 0)
        w2--;

    if (w2 > 0) {
        int n = w2;
        int y = ay;
        unsigned short s = stride;
        int stride2 = s * 2;
        int offset = (py * s + px) * 2;
        do {
            unsigned short* p = (unsigned short*)((char*)g_game->mapValues + offset);
            int m = w1;
            if (m > 0) {
                int x = ax;
                do {
                    FUN_004c6e70(surface, x, y, g_game->iconSet->data + *p * 0x400);
                    x += 32;
                    p++;
                } while (--m);
            }
            offset += stride2;
            y += 32;
        } while (--n);
    }
}
