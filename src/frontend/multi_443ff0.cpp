// Decompiled by DeepSeek V4.1 Flash, finished by Space Bunny Free. Names are provisional.
// Stays out of multi.cpp: merged, the session list pointer lands in ebp (docs/split-modules.md).
// Resolves the connection index (searching the enumerated DirectPlay
// connections for the provider GUID selected in g_cmdlineDplayProvider when the
// incoming index is negative), then records that connection's GUID and
// copies its data block into a freshly allocated buffer.
#include <stdio.h>
#include <string.h>
extern "C" __declspec(dllimport) char* __stdcall lstrcpynA(char*, const char*, int);
// Kept: <guiddef.h> is not on this translation unit's include path; this is the Windows GUID layout.
struct GUID { unsigned long Data1; unsigned short Data2; unsigned short Data3; unsigned char Data4[8]; };
#include "../network/provider_conn.h"
struct ConnInfo_443ff0 { GUID guid; HapinetProviderConnEntry conn; };
#pragma pack(push,1)
struct Game {
    char unknown_0[0x445];
    GUID* guids;
    char unknown_449[0x4f9 - 0x449];
    int count;
    char unknown_4fd[0x2a9f - 0x4fd];
    GUID* sessions;
    HapinetProviderConnEntry* conns;
    char unknown_2aa7[0x2be3 - 0x2aa7];
    char name[0x40];
    char unknown_2c23[0x39201 - 0x2c23];
    ConnInfo_443ff0 info;
};
#pragma pack(pop)
extern Game* g_game;
extern int g_cmdlineDplayProvider;
extern char g_onlineLobbyPassword;
extern GUID g_dpspGuidTcpip;
extern GUID g_dpspGuidIpx;
extern GUID g_dpspGuidModem;
extern GUID g_dpspGuidSerial;
void* __cdecl GameAllocIgnoreTag(const char* name, unsigned int size);
int IsOnlineConfigLoaded();
// FUNCTION: 0x443ff0
int __stdcall SelectConnection(int index)
{
    if (index < 0) {
        GUID* guid = 0;
        switch (g_cmdlineDplayProvider) {
        case 0: break;
        case 1: guid = &g_dpspGuidTcpip; break;
        case 2: guid = &g_dpspGuidIpx; break;
        case 3: guid = &g_dpspGuidModem; break;
        case 4: guid = &g_dpspGuidSerial; break;
        }
        g_cmdlineDplayProvider = 0;
        if (guid != 0) {
            int i = 0;
            for (; i < g_game->count; i++) {
                if (memcmp(&g_game->guids[i], guid, sizeof(GUID)) == 0)
                    break;
            }
            if (i < g_game->count)
                index = i;
        }
    }
    if (index >= 0) {
        g_game->info.guid = g_game->sessions[index];
        g_game->info.conn = g_game->conns[index];
        int size = g_game->conns[index].dwCbConnectionBlob;
        g_game->info.conn.pConnectionBlob = GameAllocIgnoreTag("DPLAY CONNECTION INFO", size);
        if (g_game->info.conn.pConnectionBlob != 0) {
            memcpy(g_game->info.conn.pConnectionBlob, g_game->conns[index].pConnectionBlob, size);
        } else {
            g_game->info.conn.dwCbConnectionBlob = 0;
        }
        if (IsOnlineConfigLoaded() && g_onlineLobbyPassword != 0) {
            lstrcpynA(g_game->name, &g_onlineLobbyPassword, 0xb);
        }
        return 1;
    }
    return 0;
}
