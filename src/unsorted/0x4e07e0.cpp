// Decompiled by space-bunny-free. Names are provisional.
// Reports the process working set into a caller-supplied buffer, with a
// psapi.dll QueryWorkingSet refresh at most once every ten calls.
//
// Still differs (84.5%, 916 of 934 bytes). Everything matches except one
// group of 15 bytes: the original stores ptn/sharedn/privn into their globals
// BOTH before the scan loop and again after it, while ours only keeps the
// second set. So the original's source must have the second set INSIDE the
// loop's "if (n)" block, which is the only way the first set stays live.
// Written that way it compiles to 951-952 bytes (76%): MSVC 5 then demotes the
// loop counter out of ebp into a stack slot and drops esi, so the whole
// register rotation of the loop goes wrong. That is a register pressure
// problem, not a shape problem, and it is unsolved here.
// A separate loop counter variable (int i = n) with the stores inside gets the
// SIZE almost exactly right (938 bytes) but scores lower, 83.6%.
#include <windows.h>
#include <stdio.h>

LPCRITICAL_SECTION FUN_004e06f0(void);

extern int DAT_00529508;
extern HMODULE DAT_005295bc;
extern char DAT_005295cc;
extern HANDLE DAT_00529530;
extern int DAT_00529528;
extern int DAT_005295b8;
extern int DAT_005295c0;
extern int DAT_005295c8;
extern int DAT_005295d0;
extern int DAT_005295d4;

typedef struct {
    DWORD NumberOfPages;
    unsigned int WorkingSetInfo[99999];
} WSInfo_004e07e0;

typedef BOOL (WINAPI *QueryWorkingSet_004e07e0)(HANDLE, PVOID, DWORD);

// Writes n as a decimal with thousands separators, then reverses it.
static void __inline fmt_004e07e0(char *buf, unsigned int n)
{
    char *w = buf;
    char *f = buf;
    int digits = 0;

    *w = 0;
    do {
        *w = (char)('0' + n % 10);
        w++;
        n /= 10;
        digits++;
        if (digits % 3 == 0) {
            if (n) {
                *w = ',';
                w++;
            }
        }
    } while (n);
    *w = 0;
    w--;
    for (; f < w;) {
        char a = *w;
        char b = *f;
        *f++ = a;
        *w-- = b;
    }
}

// FUNCTION: 0x4e07e0
char __cdecl FUN_004e07e0(char *dest)
{
    LPCRITICAL_SECTION cs = FUN_004e06f0();
    WSInfo_004e07e0 ws;
    char priv[20];
    char max[20];
    char shared[20];
    char total[20];
    char pt[20];
    int ptn;
    int privn;
    int sharedn;
    DWORD *p;
    DWORD w;
    DWORD lo;
    DWORD hi;
    DWORD n;

    EnterCriticalSection(cs);
    if (DAT_005295d0 < 0 || !(--DAT_005295d0 > 0)) {
        if (DAT_005295cc == 0) {
            DAT_005295bc = LoadLibraryA("psapi.dll");
            if (DAT_005295bc != 0) {
                DAT_00529508 = (int)GetProcAddress(DAT_005295bc, "QueryWorkingSet");
            }
            DAT_00529530 = GetCurrentProcess();
            DAT_005295cc = 1;
        }
        if (DAT_00529508 == 0) {
            LeaveCriticalSection(cs);
            return 0;
        }
        if (!((QueryWorkingSet_004e07e0)DAT_00529508)(DAT_00529530, &ws, 0x61a80)) {
            LeaveCriticalSection(cs);
            return 0;
        }
        n = ws.NumberOfPages;
        DAT_005295c8 = n;
        if (n > DAT_005295d4) {
            DAT_005295d4 = n;
        }
        ptn = 0;
        sharedn = 0;
        privn = 0;
        DAT_005295c0 = ptn;
        DAT_005295b8 = sharedn;
        DAT_00529528 = privn;
        p = (DWORD *)&ws.WorkingSetInfo[0];
        if (n) {
            do {
                w = *p;
                lo = w & 0xfff;
                hi = w & 0xfffff000;
                if (hi >= 0xc0000000 && hi <= 0xe0000000) {
                    ptn++;
                } else if (lo & 1) {
                    sharedn++;
                } else {
                    privn++;
                }
                p++;
            } while (--n);
        }
        DAT_005295c0 = ptn;
        DAT_005295b8 = sharedn;
        DAT_00529528 = privn;
        DAT_005295d0 = 10;
    }
    fmt_004e07e0(pt, DAT_005295c0 << 12);
    fmt_004e07e0(shared, DAT_005295b8 << 12);
    fmt_004e07e0(priv, DAT_00529528 << 12);
    fmt_004e07e0(max, DAT_005295d4 << 12);
    fmt_004e07e0(total, DAT_005295c8 << 12);
    sprintf(dest, "\r\nTotal Working set:   %13s\r\nMaximum Working set: %13s\r\nPrivate:             %13s\r\nShared:              %13s\r\nPage Tables:         %13s\r\n", total, max, priv, shared, pt);
    LeaveCriticalSection(cs);
    return 1;
}
