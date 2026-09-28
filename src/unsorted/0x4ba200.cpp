// Decompiled by Sonnet 5.5. Names are provisional.
// Sets `count` palette entries from `start` (4 bytes each: three colour
// bytes and a zero) under the 'MAIN' lock. The entries are stored in the
// display's palette table, scaled by the display's brightness (+0x614) and
// clamped to 255 into a local palette, and that palette is then either
// installed through GDI (a new logical palette plus SetDIBColorTable on the
// display's DC, when the display has a window DC) or handed to the
// DirectDraw palette object, whose failure returns 0.
//
// NOT MATCHED: 61.7%, 703 of 689 bytes. The flow, calls and constants are
// right; the frame layout (held at [esp+0x14], brightness at 0x18, display at
// 0x1c, loop count at 0x20 in the original) and the loops' induction
// variables are not: the original takes &entries[start] before the range test
// and walks source and destination by one pointer plus a difference, and does
// the same for the two palette copies (ebp = local - src). Pointer-walk
// spellings of the loops scored lower.
#include <windows.h>
#include <ddraw.h>

#pragma pack(push, 1)
struct Display_004ba200 {
    char unknown_0[0x44];
    int field_44;                      // +0x44
    HDC dc;                            // +0x48
    HPALETTE palette;                  // +0x4c
    char unknown_50[0x94 - 0x50];
    LPDIRECTDRAWPALETTE ddPalette;     // +0x94
    char unknown_98[0xf0 - 0x98];
    unsigned short pad_f0 : 2;         // +0xf0
    unsigned short bit2 : 1;
    unsigned short rest_f0 : 13;
    char unknown_f2[0x214 - 0xf2];
    unsigned int entries[256];         // +0x214
    char unknown_614[0];
};
#pragma pack(pop)

extern LONG DAT_0052a4e8;
extern LONG DAT_0052a4ec;
extern HANDLE DAT_0052a4f0;

Display_004ba200* FUN_004b6220(void);
void* FUN_004d83b0(const char* name, unsigned int size);
void FUN_004d85a0(void* p);

static inline LONG Lock()
{
    while (1) {
        LONG r = InterlockedExchange(&DAT_0052a4e8, 0x4d41494e);
        if (r == 0) {
            DAT_0052a4ec = 0x4d41494e;
            return 0;
        }
        if (DAT_0052a4ec == 0x4d41494e)
            return r;
        WaitForSingleObject(DAT_0052a4f0, INFINITE);
    }
}

static inline void Unlock(LONG held)
{
    if (held == 0) {
        DAT_0052a4ec = 0;
        InterlockedExchange(&DAT_0052a4e8, 0);
        SetEvent(DAT_0052a4f0);
    }
}

// FUNCTION: 0x4ba200
int __stdcall FUN_004ba200(unsigned char* src, int start, int count)
{
    LONG held = Lock();
    Display_004ba200* d = FUN_004b6220();
    float brightness = *(float*)((char*)d + 0x614);
    int end = start + count;
    unsigned char local[0x400];
    unsigned char quad[0x400];
    int i;
    for (i = start; i < end; i++)
        d->entries[i] = ((unsigned int*)src)[i];
    for (i = start; i < end; i++) {
        float v0 = src[i * 4] * brightness;
        if (v0 > 255.0)
            v0 = 255.0f;
        local[i * 4] = (unsigned char)(int)v0;
        float v1 = src[i * 4 + 1] * brightness;
        if (v1 > 255.0)
            v1 = 255.0f;
        local[i * 4 + 1] = (unsigned char)(int)v1;
        float v2 = src[i * 4 + 2] * brightness;
        if (v2 > 255.0)
            v2 = 255.0f;
        local[i * 4 + 2] = (unsigned char)(int)v2;
        local[i * 4 + 3] = 0;
    }
    if (d->field_44 != 0) {
        if (d->palette)
            DeleteObject(d->palette);
        LOGPALETTE* lp = (LOGPALETTE*)FUN_004d83b0("Palette", 0x408);
        for (i = 0; i < 256; i++) {
            ((unsigned int*)lp->palPalEntry)[i] = ((unsigned int*)local)[i];
            quad[i * 4 + 2] = local[i * 4];
            quad[i * 4 + 1] = local[i * 4 + 1];
            quad[i * 4] = local[i * 4 + 2];
            quad[i * 4 + 3] = 0;
        }
        lp->palVersion = 0x300;
        lp->palNumEntries = 0x100;
        d->palette = CreatePalette(lp);
        SetDIBColorTable(d->dc, 0, 0x100, (RGBQUAD*)quad);
        FUN_004d85a0(lp);
    } else if (d->bit2) {
        if (d->ddPalette->SetEntries(0, start, count, (LPPALETTEENTRY)(local + start * 4)) != 0) {
            Unlock(held);
            return 0;
        }
    }
    Unlock(held);
    return 1;
}
