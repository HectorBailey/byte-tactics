// Decompiled by deepseek-v4.1-flash, finished by space-bunny-free, edited by deepseek-v4.1. Names are provisional.
// MATCH at 1013/1013 bytes. Two source shapes were the whole difference:
//  - the PE timestamp must be reached through IMAGE_NT_HEADERS and
//    &ntHeaders->FileHeader.TimeDateStamp; adding 8 to the char* instead
//    folds the two-step add into one lea and loses a byte.
//  - the processor-count block needs the two branches written differently:
//    `p = buf + strlen(buf);` in the >1 branch (keeps the count in edx and p
//    in eax, and pushes the count before the strlen) and an inline
//    `sprintf(buf + strlen(buf), "1 processor\n")` in the else branch. Two
//    identical `p = ...` branches let MSVC CSE the buffer lea to before the
//    cmp and shorten the function by 7 bytes.
// Original bug preserved: CreateFileA failure is tested against zero at
// 0x4deee1, so INVALID_HANDLE_VALUE reaches GetFileSize at 0x4deeec.
#include <windows.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern char DAT_005119b8;
extern char* DAT_0050d4d0;
extern "C" int __cdecl FUN_004d8df0(void);
extern "C" void* __cdecl FUN_004d8e20(void);

// FUNCTION: 0x4ded60
void __cdecl FUN_004ded60(char* dest, int destLen)
{
    WORD fatDate;
    time_t now;
    WORD fatTime;
    DWORD userNameSize;
    FILETIME ft;
    MEMORYSTATUS memStatus;
    SYSTEM_INFO sysInfo;
    struct tm gmTimeCopy;
    struct tm localTimeCopy;
    char userName[300];
    char buf[2000];
    char exeName[1000];
    char* p;

    buf[0] = DAT_005119b8;
    memset(buf + 1, 0, 1999);

    now = time(NULL);
    localTimeCopy = *localtime(&now);
    p = buf + strlen(buf);
    sprintf(p, "Time: %s", asctime(&localTimeCopy));

    userNameSize = 300;
    if (GetUserNameA(userName, &userNameSize) == 0) {
        strcpy(userName, "unknown user");
    }

    char* machine = getenv("computername");
    if (machine == NULL) {
        machine = "unknown machine";
    }

    if (GetModuleFileNameA(NULL, exeName, 1000) == 0) {
        strcpy(exeName, "Unknown");
    }

    sprintf(buf + strlen(buf), "%s, run by %s on %s\n", exeName, userName, machine);

    HANDLE hFile = CreateFileA(exeName, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != 0) {
        DWORD fileSize = GetFileSize(hFile, NULL);
        if (GetFileTime(hFile, NULL, NULL, &ft)) {
            if (FileTimeToLocalFileTime(&ft, &ft)) {
                if (FileTimeToDosDateTime(&ft, &fatDate, &fatTime)) {
                    p = buf + strlen(buf);
                    sprintf(p,
                        "Executable is %d bytes long and dated %d/%d/%d %02d:%02d:%02d\n",
                        fileSize, (fatDate >> 5) & 0xf, fatDate & 0x1f, (fatDate >> 9) + 1980,
                        fatTime >> 11, (fatTime >> 5) & 0x3f, (fatTime & 0x1f) * 2);
                }
            }
        }
        CloseHandle(hFile);
    }

    HANDLE hMod = GetModuleHandleA(NULL);
    IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)((char*)hMod + ((IMAGE_DOS_HEADER*)hMod)->e_lfanew);
    gmTimeCopy = *gmtime((time_t*)&ntHeaders->FileHeader.TimeDateStamp);
    p = buf + strlen(buf);
    sprintf(p, "UTC link time: %08lx - %s", ntHeaders->FileHeader.TimeDateStamp, asctime(&gmTimeCopy));

    p = buf + strlen(buf);
    sprintf(p, "Library version %d. Library date %s\n",
            996, DAT_0050d4d0 + strlen("Library date stamp: "));

    GetSystemInfo(&sysInfo);
    if (sysInfo.dwNumberOfProcessors > 1) {
        p = buf + strlen(buf);
        sprintf(p, "%d processors\n", sysInfo.dwNumberOfProcessors);
    } else {
        sprintf(buf + strlen(buf), "1 processor\n");
    }

    memStatus.dwLength = sizeof(memStatus);
    GlobalMemoryStatus(&memStatus);
    p = buf + strlen(buf);
    sprintf(p, "%d MBytes physical memory\n",
            (memStatus.dwTotalPhys + 900000) >> 20);

    p = buf + strlen(buf);
    sprintf(p, "Stack goes from %08lX to %08lX\n",
            FUN_004d8df0(), FUN_004d8e20());

    strncpy(dest, buf, destLen);
    dest[destLen - 1] = '\0';
}
