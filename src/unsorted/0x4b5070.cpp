// Decompiled by space-bunny-free. Names are provisional.
// Reads the installed DirectX version: first through dsetup.dll's
// DirectXSetupGetVersion, then, if that fails, through
// HKLM\Software\Microsoft\DirectX (the "InstalledVersion" DWORD on NT, the
// "Version" string on Win9x), and compares the result with the wanted version.
//
// NOT MATCHING YET (43.7% by true LCS over instructions, 606 of 619 bytes).
// What still differs, largest cause first:
//
// 1. THE FRAME IS 0xD0, THE ORIGINAL'S IS 0xCC, so every [esp+X] in the body
//    is 4 bytes high and the five argument slots land at +4. The original's
//    local area (0x10..0xdb) is exactly full with six dwords (0x10..0x24),
//    the 30-byte version buffer (0x28) and OSVERSIONINFOA (0x48), and it
//    shares slots pairwise: {dwMaj, hKey}, {dwMin, size30}, {lib, size4}.
//    This source instead gives dwMaj, dwMin and hKey a slot each (7 dwords),
//    so it needs one dword more. Moving dwMaj/dwMin/hKey/type into their
//    blocks does make MSVC share {dwMaj, hKey}, but then isNT shares with
//    type and a spill slot for minlo appears, and the score drops (27%).
// 2. REGISTER ROLES. The original holds the four version values in
//    ebx=majhi, ebp=majlo, edi=minhi, esi=minlo and keeps isNT, lib and
//    status in memory. Here lib is register-allocated, isNT lives in ebp and
//    one version value is spilled, which shifts every later choice. The
//    source order of the four assignments is the only lever found: all 24
//    permutations were scored, and (majhi, majlo, minlo, minhi) is the best
//    of them at 43.7%; (majhi, majlo, minhi, minlo) gives 32.9% and
//    (majhi, minhi, majlo, minlo) 26.9%.
// 3. THE ZEROING OF THE FIRST 16 BYTES OF THE version BUFFER. Both failure
//    exits (0x4b52ba) store 0 to [esp+0x28], [esp+0x2c], [esp+0x30] and
//    [esp+0x34] from one zeroed register, interleaved with the pops, which is
//    exactly how MSVC 5 expands memset(buf, 0, 16) (verified on a throwaway
//    function: one zeroed register, four dword stores). The fallback path
//    stores 0 to [esp+0x2c] alone, one dword into that same buffer. Nothing
//    in this source writes there, so the original's source still has a
//    variable or a memset that has not been identified. Not yet tried:
//    a 16-byte memset on each failure exit, and a DWORD[4] at that offset.
// 4. Small shapes: the original tests the LoadLibrary result with `test eax,
//    eax` and only then stores it (here ebp holds the handle across both
//    calls), and it compares call results against a zero register (`cmp eax,
//    ebp`, `cmp eax, eap`) where this source emits `test eax, eax`.
//
// Two things in the original look like Cavedog's own bugs, kept here as they
// are: the "installed version is older" arm at 0x4b5233 compares the major
// half against argument 2 (the minor half) instead of against argument 1, and
// the Win9x query passes a 30-byte size for a buffer the frame only has room
// for from 0x28 to 0x45.
#include <windows.h>
#include <string.h>
#include <stdlib.h>

typedef int (__stdcall *FN_DIRECTXSETUPGETVERSION)(DWORD* major, DWORD* minor);

// FUNCTION: 0x4b5070
int __stdcall FUN_004b5070(int want0, int want1, int want2, int want3, int want4)
{
    unsigned int majhi;
    unsigned int majlo;
    unsigned int minhi;
    unsigned int minlo;
    DWORD status = 0;
    int isNT;
    HMODULE lib;
    FARPROC proc;
    DWORD dwMaj;
    DWORD dwMin;
    OSVERSIONINFOA osvi;
    HKEY hKey;
    LONG err;
    DWORD type;
    char version[30];

    majhi = majlo = minhi = minlo = 0;
    lib = LoadLibraryA("dsetup.dll");
    if (lib) {
        proc = GetProcAddress(lib, "DirectXSetupGetVersion");
        if (proc) {
            dwMaj = 0;
            dwMin = 0;
            status = (DWORD)((FN_DIRECTXSETUPGETVERSION)proc)(&dwMaj, &dwMin);
            if (status) {
                majhi = dwMaj >> 16;
                majlo = dwMaj & 0xffff;
                minlo = dwMin & 0xffff;
                minhi = dwMin >> 16;
            }
        }
        FreeLibrary(lib);
    }
    if (!status) {
        majhi = minhi = minlo = 0;
        isNT = 0;
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
        if (GetVersionExA(&osvi)) {
            isNT = osvi.dwPlatformId == 2;
        }
        hKey = 0;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\DirectX", 0, KEY_READ, &hKey) == 0) {
            status = 0;
            if (isNT) {
                DWORD size = 4;
                err = RegQueryValueExA(hKey, "InstalledVersion", 0, &type, (LPBYTE)&status, &size);
            } else {
                DWORD size = 30;
                err = RegQueryValueExA(hKey, "Version", 0, &type, (LPBYTE)version, &size);
            }
            RegCloseKey(hKey);
            if (err) {
                return 0;
            }
            if (isNT) {
                majlo = status & 0xff;
            } else {
                majhi = atoi(strtok(version, "."));
                majlo = atoi(strtok(0, "."));
                minhi = atoi(strtok(0, "."));
                minlo = atoi(strtok(0, "."));
            }
        } else {
            return 0;
        }
    }
    if (isNT) {
        return majlo >= (unsigned int)want4;
    }
    if (majhi != (unsigned int)want0) {
        return majhi >= (unsigned int)want1;
    }
    if (majlo != (unsigned int)want1) {
        return majlo >= (unsigned int)want1;
    }
    if (minhi != (unsigned int)want2) {
        return minhi >= (unsigned int)want2;
    }
    return minlo >= (unsigned int)want3;
}
