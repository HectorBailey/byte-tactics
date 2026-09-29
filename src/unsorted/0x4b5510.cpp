// Decompiled by deepseek-v4.1-flash, finished by deepseek-v4.1-flash. Names are provisional.
// Switches the display between the DirectDraw fullscreen path (mode != 0) and
// the GDI/DIB windowed path (mode == 0), rebuilding the surfaces, clipper and
// palette. Returns 1 on success, 0 after releasing the 'MAIN' display lock.
//
// PARTIAL (67.5%). The structure, the Lock/Unlock helper bodies, the embedded
// DDraw member struct at +0x84 and the pragma pack(1) layout are right.
// What still differs, and it is one allocation decision:
//   - frame is 0x4cc, original 0x4d0, so every [esp+X] is 4 bytes low; the
//     original spills &display->dc in a slot and reloads it, here it stays in
//     edi and the DeleteObject import is cached in ebp, where the original
//     keeps the constant 0.
//   - original keeps 0 in ebp (xor ebp,ebp at entry, cmp eax,ebp / push ebp);
//     here 0 is immediates (test eax,eax, push 0) and ebp holds an import.
//   - the mode!=0 branch reloads display->width/height/bit1 from the global
//     where the original keeps them in a register the whole way.
// Levers that moved it most: the Lock()/Unlock(held) inline helpers (0 and
// `held` then land correctly), the DDraw sub-struct so ebx is &display->dd,
// and using the local `d` for hwnd/width/height/&dc (67.5%) while entries,
// field_98 and the GDI bmi widths stay global loads. Forcing MORE sites onto
// `d` (SetDisplayMode/ddsd/entries/bmi widths) or ALL sites (32%) loses points.
#include <windows.h>
#include <ddraw.h>

struct Surface_004b5510 {
    int data[12];
};

struct BitmapInfo_004b5510 {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[256];
};

#pragma pack(push, 1)
struct DDraw_004b5510 {
    IDirectDraw* ddraw;                // +0x84
    IDirectDrawSurface* primary;       // +0x88
    IDirectDrawSurface* back;          // +0x8c
    IDirectDrawClipper* clipper;       // +0x90
    IDirectDrawPalette* palette;       // +0x94
    void* field_98;                    // +0x98
    int field_9c;                      // +0x9c
};

struct Display_004b5510 {
    char unknown_0[0x40];
    HWND hwnd;                         // +0x40
    HBITMAP dib;                       // +0x44
    HDC dc;                            // +0x48
    HPALETTE hpalette;                 // +0x4c
    char unknown_50[0x84 - 0x50];
    DDraw_004b5510 dd;                 // +0x84
    char unknown_a0[0xd4 - 0xa0];
    int width;                         // +0xd4
    int height;                        // +0xd8
    char unknown_dc[0xf0 - 0xdc];
    unsigned short bit0 : 1;
    unsigned short bit1 : 1;
    unsigned short rest : 14;          // +0xf0
    char unknown_f2[0x214 - 0xf2];
    PALETTEENTRY entries[256];         // +0x214
};
#pragma pack(pop)

extern LONG DAT_0052a4e8;
extern LONG DAT_0052a4ec;
extern HANDLE DAT_0052a4f0;
extern Display_004b5510 *DAT_0051fbd0;

int __stdcall FUN_0049f710(int guid, void* display, int zero);
void __stdcall FUN_004b4ff0(Display_004b5510* d);
void __stdcall FUN_004c6a60(void* s, int width, int height, int a, int b);
int __stdcall FUN_004c5e70(Surface_004b5510* s);
void __cdecl FUN_004cbbe0(Surface_004b5510* dst, void* src, int x, int y);
int __stdcall FUN_004c5fa0(Surface_004b5510* s);
int __stdcall FUN_004ba200(PALETTEENTRY* entries, int start, int count);

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

// FUNCTION: 0x4b5510
int __stdcall FUN_004b5510(int mode)
{
    LONG held = Lock();
    Display_004b5510* d = DAT_0051fbd0;

    d->dd.field_9c = 0;
    DDraw_004b5510* dd = &d->dd;
    FUN_004b4ff0(d);

    HDC* pdc = &d->dc;
    if (*pdc)
        DeleteDC(*pdc);
    if (d->hpalette)
        DeleteObject(d->hpalette);
    if (d->dib)
        DeleteObject(d->dib);
    d->hpalette = 0;
    d->dib = 0;
    *pdc = 0;
    SetWindowPos(d->hwnd, NULL, 0, 0, d->width,
                 d->height, SWP_NOZORDER | SWP_NOMOVE);

    if (mode != 0) {
        DAT_0051fbd0->bit1 = 1;

        if (FUN_0049f710(0, &dd->ddraw, 0) != 0)
            goto fail;
        if (dd->ddraw->SetCooperativeLevel(d->hwnd, 0x53) != DD_OK)
            goto fail;
        if (dd->ddraw->SetDisplayMode(DAT_0051fbd0->width, DAT_0051fbd0->height, 8) != DD_OK)
            goto fail;

        DDSURFACEDESC ddsd;
        ZeroMemory(&ddsd, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
        ddsd.dwWidth = DAT_0051fbd0->width;
        ddsd.dwHeight = DAT_0051fbd0->height;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
        ddsd.dwBackBufferCount = 1;
        if (dd->ddraw->CreateSurface(&ddsd, &dd->primary, NULL) != DD_OK)
            goto fail;

        DDSCAPS caps;
        caps.dwCaps = DDSCAPS_BACKBUFFER;
        if (dd->primary->GetAttachedSurface(&caps, &dd->back) != DD_OK)
            goto fail;

        if (dd->ddraw->CreateClipper(0, &dd->clipper, NULL) != DD_OK)
            goto fail;
        dd->field_9c = 1;
        if (dd->clipper->SetHWnd(0, d->hwnd) != DD_OK)
            goto fail;
        if (dd->primary->SetClipper(dd->clipper) != DD_OK)
            goto fail;

        if (dd->ddraw->CreatePalette(4, DAT_0051fbd0->entries, &dd->palette, NULL) == DD_OK) {
            if (dd->primary->SetPalette(dd->palette) != DD_OK)
                goto fail;
        }

        if (DAT_0051fbd0->dd.field_98) {
            Surface_004b5510 surf;
            FUN_004c5e70(&surf);
            FUN_004cbbe0(&surf, DAT_0051fbd0->dd.field_98, 0, 0);
            FUN_004c5fa0(&surf);
        }
    } else {
        DAT_0051fbd0->bit1 = 0;

        HDC hdc = GetDC(d->hwnd);
        d->dc = CreateCompatibleDC(hdc);
        ReleaseDC(d->hwnd, hdc);

        BitmapInfo_004b5510 bmi;
        void* bits;
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = DAT_0051fbd0->width;
        bmi.bmiHeader.biHeight = -DAT_0051fbd0->height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 8;
        bmi.bmiHeader.biCompression = 0;
        bmi.bmiHeader.biSizeImage = 0;
        bmi.bmiHeader.biClrUsed = 0;
        bmi.bmiHeader.biClrImportant = 0;
        ZeroMemory(bmi.bmiColors, sizeof(bmi.bmiColors));
        d->dib = CreateDIBSection(d->dc, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, &bits, NULL, 0);
        FUN_004c6a60(&d->unknown_50[0], DAT_0051fbd0->width, DAT_0051fbd0->height,
                     (DAT_0051fbd0->width + 3) & ~3, (int)bits);
        SelectObject(d->dc, d->dib);
        SetWindowPos(d->hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    FUN_004ba200(DAT_0051fbd0->entries, 0, 0x100);
    Unlock(held);
    return 1;

fail:
    Unlock(held);
    return 0;
}
