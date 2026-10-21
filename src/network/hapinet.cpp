// Decompiled by space-bunny-free, Haiku, Opus, Sonnet and DeepSeek V4.1 Flash. Names are provisional.
// The HAPINET DirectPlay wrapper: the error strings, the packet send and
// receive calls, the session, provider and player calls, the lobby and
// address calls, the interface setup and teardown, and the report interface
// stubs.

#include <windows.h>
#include <stdio.h>
#include <string.h>

// DirectPlay session description (DPSESSIONDESC2).
struct DPSESSIONDESC2 {
    unsigned long dwSize;            // +0x00
    unsigned long dwFlags;           // +0x04
    GUID guidInstance;      // +0x08
    GUID guidApplication;   // +0x18
    unsigned long dwMaxPlayers;      // +0x28
    unsigned long dwCurrentPlayers;  // +0x2c
    char* lpszSessionName;           // +0x30
    char* lpszPassword;              // +0x34
    unsigned long dwReserved1;       // +0x38
    unsigned long dwReserved2;       // +0x3c
    unsigned long dwUser1;           // +0x40
    unsigned long dwUser2;           // +0x44
    unsigned long dwUser3;           // +0x48
    unsigned long dwUser4;           // +0x4c
};

// One game in the list the EnumSessions callback fills.
struct GameRec_4c9c50 {
    int unknown_0;                   // +0x00
    int user1;                       // +0x04
    int user2;                       // +0x08
    int user3;                       // +0x0c
    int user4;                       // +0x10
    int maxPlayers;                  // +0x14
    char name[0x20];                 // +0x18
    GUID guidInstance;      // +0x38
    char unknown_48[0xc];            // +0x48
};

// A connection the EnumConnections callback records.
struct HapinetProviderConnEntry {
    void* connectionBlob;            // +0x0
    unsigned long connectionBytes;   // +0x4
};

// A connection's name, from the EnumConnections callback.
struct DPNAME {
    unsigned long dwSize;
    unsigned long dwFlags;
    char* lpszShortNameA;
    char* lpszLongNameA;
};

// The buffer holding the connection settings; +8 points at the session
// description.
struct DPLCONNECTION {
    char unknown_0[8];
    DPSESSIONDESC2* sessionDesc;     // +0x08
};

// IDirectPlay3-like COM interface: the slots this module calls.
class DirectPlay_4c97b0 {
public:
    virtual int __stdcall QueryInterface(void* iid, void** out);
    virtual unsigned long __stdcall AddRef();
    virtual unsigned long __stdcall Release();
    virtual int __stdcall AddPlayerToGroup(unsigned long group, unsigned long player);
    virtual int __stdcall Close();
    virtual int __stdcall Slot05();
    virtual int __stdcall CreatePlayer(unsigned long* id, DPNAME* name, void* event,
                                       void* data, unsigned long size, unsigned long flags);   // +0x18
    virtual int __stdcall Slot07();
    virtual int __stdcall Slot08();
    virtual int __stdcall DestroyPlayer(unsigned long id);                                    // +0x24
    virtual int __stdcall Slot10();
    virtual int __stdcall Slot11();
    virtual int __stdcall EnumPlayers(void* session, void* callback, void* context, unsigned long flags);   // +0x30
    virtual int __stdcall EnumSessions(void* desc, unsigned long timeout, void* callback, void* context, unsigned long flags);
    virtual int __stdcall Slot14();
    virtual int __stdcall Slot15();
    virtual int __stdcall Slot16();
    virtual int __stdcall Slot17();
    virtual int __stdcall GetPlayerAddress(unsigned long player, void* data, unsigned long* size);   // +0x48
    virtual int __stdcall Slot19();
    virtual int __stdcall GetPlayerData(unsigned long player, void* data, unsigned long* size, unsigned long flags);
    virtual int __stdcall GetPlayerName(unsigned long id, void* data, unsigned long* size);   // +0x54
    virtual int __stdcall GetSessionDesc(void* data, unsigned long* size);
    virtual int __stdcall Slot23();
    virtual int __stdcall Slot24(void* desc, unsigned long flags);
    virtual int __stdcall Receive(unsigned long* from, unsigned long* to, unsigned long flags, void* data, unsigned long* size);
    virtual int __stdcall Send(unsigned long from, unsigned long to, unsigned long flags, void* data, unsigned long size);
    virtual int __stdcall Slot27();
    virtual int __stdcall Slot28();
    virtual int __stdcall SetPlayerData(unsigned long player, void* data, unsigned long size, unsigned long flags);
    virtual int __stdcall SetPlayerName(unsigned long id, void* name, unsigned long flags);   // +0x78
    virtual int __stdcall SetSessionDesc(void* desc, unsigned long flags);
    virtual int __stdcall Slot32();
    virtual int __stdcall Slot33();
    virtual int __stdcall Slot34();
    virtual int __stdcall EnumConnections(GUID* application, void* callback, void* context, unsigned long flags);
    virtual int __stdcall Slot36();
    virtual int __stdcall Slot37();
    virtual int __stdcall Slot38(int val, int flag);                                          // +0x98
};

// IDirectPlayLobby-like COM interface: the slots this module calls.
class Lobby_4c9a70 {
public:
    virtual int __stdcall QueryInterface(GUID* riid, void** ppvObject);
    virtual unsigned long __stdcall AddRef();
    virtual unsigned long __stdcall Release();
    virtual int __stdcall Connect(unsigned long flags, void** dp, void* outer);
    virtual int __stdcall CreateAddress(GUID* sp, GUID* device, const void* address,
                                        unsigned long addressSize, void* dest,
                                        unsigned long* destSize);
    virtual int __stdcall EnumAddress(void* callback, const void* address,
                                      unsigned long length, void* context);
    virtual int __stdcall EnumAddressTypes(void* callback, GUID* sp, void* context,
                                           unsigned long flags);
    virtual int __stdcall EnumLocalApplications(void* callback, void* context,
                                                unsigned long flags);
    virtual int __stdcall GetConnectionSettings(unsigned long appID, void* connection,
                                                unsigned long* dataSize);                 // +0x20
    virtual int __stdcall Slot09();
    virtual int __stdcall Slot10();
    virtual int __stdcall Slot11();
    virtual int __stdcall SetConnectionSettings(unsigned long a, unsigned long b, DPLCONNECTION* conn);
    virtual int __stdcall Slot13();
    virtual int __stdcall CreateCompoundAddress(void* elements, unsigned long count, void* address,
                                                unsigned long* size);                     // +0x38
};

#pragma pack(push, 1)
// The HAPINET network object (Thaldren's HapinetSession, 0x505 bytes): the
// session and player names, the application and service provider GUIDs, the
// session description, the DirectPlay and lobby interfaces, the enumeration
// buffers and the counters.
struct HapinetSession {
    char sessionName[0x11];           // +0x0, aSessionName
    char playerLongName[16];          // +0x11
    char playerShortName[16];         // +0x21
    char unusedAfterPlayerShortName[0x431 - 0x31]; // +0x31
    GUID* providerGuids;              // +0x431
    HapinetProviderConnEntry* providerConnections;  // +0x435
    char* providerDescLines;          // +0x439
    GUID application;        // +0x43d
    GUID serviceProviderGuid;         // +0x44d
    DPSESSIONDESC2 sessionDesc;       // +0x45d
    int hwnd;                         // +0x4ad
    char unusedAfterHwnd[4];          // +0x4b1
    unsigned long recvFromId;         // +0x4b5
    unsigned long recvToId;           // +0x4b9
    GameRec_4c9c50* games;            // +0x4bd
    DirectPlay_4c97b0* dp1;           // +0x4c1
    DirectPlay_4c97b0* dp;            // +0x4c5
    Lobby_4c9a70* directPlayLobby;    // +0x4c9
    Lobby_4c9a70* directPlayLobby2;   // +0x4cd
    DPLCONNECTION* dplConnection;     // +0x4d1
    int connectionSize;               // +0x4d5
    char disableOutgoingCompression[4]; // +0x4d9, a dword: nonzero stops outgoing compression
    int maxPlayers;                   // +0x4dd
    int enumSessionsTimeout;          // +0x4e1
    int providerCount;                // +0x4e5
    int gamesCount;                   // +0x4e9
};
#pragma pack(pop)

#pragma pack(push, 1)
// The player data block HAPINET_addplayer passes to CreatePlayer.
struct PlayerData_004ca6a0 {
    char name[17];                     // +0x00
    short versionMajor;                // +0x11, the host rejects a joiner unless 0
    short versionMinor;                // +0x13, the host rejects a joiner unless 0x50
};
#pragma pack(pop)

// The DirectPlay interface pair HAPINET_createdplayinterface fills: the base
// object at +0x0 and the IDirectPlay3 interface at +0x4.
struct DPlayIfacePair {
    DirectPlay_4c97b0* dp;            // +0x0
    DirectPlay_4c97b0* dp3;           // +0x4
};

// DPLAYX.dll ordinal 2 (DirectPlayEnumerateA from the DirectX 5 dplay.h).
typedef BOOL (__stdcall* EnumSpCallback_4ca400)(GUID* sp, char* name, DWORD major, DWORD minor, HapinetSession* context);

extern int g_enumSessionsResult;
extern int g_guaranteePackets;
extern GUID* g_skippedProviders[4];
extern GUID DAT_004fdaf0;
extern GUID IID_IDirectPlayLobby2A;    // IID_IDirectPlayLobby2A
extern GUID IID_IDirectPlay3A;         // IID_IDirectPlay3A
extern int g_reporterReport;
extern int g_reporterNotify;

void __cdecl HapinetTrace(const char*);
void __cdecl HapinetTrace(int);
void* __cdecl GameAllocIgnoreTag(char* name, unsigned int size);
void __cdecl GameFreeThunk(void* p);
char* __stdcall Translate(char* text);
int __cdecl PeekKey();
int __cdecl PopKey();
char* __stdcall SkipTextLines(char* text, int n);
int GetDisplay();
extern "C" HRESULT __stdcall DirectPlayCreate(GUID* sp, DirectPlay_4c97b0** dp, void* unknown);
extern "C" HRESULT __stdcall DirectPlayLobbyCreateA(GUID* lpGUID,
                                                    Lobby_4c9a70** lplpDPL,
                                                    void* lpUnkOuter, void* lpReserved,
                                                    unsigned long dwFlags);
extern "C" HRESULT __stdcall DirectPlayEnumerateA(EnumSpCallback_4ca400 callback, void* context);
int __stdcall HAPINET_initmultiplay(HapinetSession* net, GUID* sp, GUID* application);

// FUNCTION: 0x4c9530
char* __stdcall HAPINET_GetDPErrorString(int error)
{
    HapinetTrace("HAPINET_GetDPErrorString\n");
    switch (error)
    {
    case 0x80004001:
        return "DPERR_UNSUPPORTED - The function is not available in this implementation. ";
    case 0x80004005:
        return "DPERR_GENERIC - An undefined error condition occurred. ";
    case 0x8007000e:
        return "DPERR_OUTOFMEMORY - There is insufficient memory to perform the requested operation. ";
    case 0x80070057:
        return "DPERR_INVALIDPARAMS - One or more of the parameters passed to the function are invalid.";
    case 0x88770005:
        return "DPERR_ALREADYINITIALIZED - This object is already initialized. ";
    case 0x8877000a:
        return "DPERR_ACCESSDENIED - The session is full or an incorrect password was supplied.";
    case 0x88770014:
        return "DPERR_ACTIVEPLAYERS - The requested operation cannot be performed because there are existing active players.";
    case 0x8877001e:
        return "DPERR_BUFFERTOOSMALL - The supplied buffer is not large enough to contain the requested data.";
    case 0x88770028:
        return "DPERR_CANTADDPLAYER - The player cannot be added to the session.";
    case 0x8877003c:
        return "DPERR_CANTCREATEPLAYER - A new player cannot be created. ";
    case 0x88770050:
        return "DPERR_CAPSNOTAVAILABLEYET - The capabilities of the DirectPlay object have not been determined yet.";
    case 0x8877005a:
        return "DPERR_EXCEPTION - An exception occurred when processing the request. ";
    case 0x88770078:
        return "DPERR_INVALIDFLAGS - The flags passed to this function are invalid. ";
    case 0x88770082:
        return "DPERR_INVALIDOBJECT - The DirectPlay object pointer is invalid. ";
    case 0x88770096:
        return "DPERR_INVALIDPLAYER - The player ID is not recognized as a valid player ID for this game session.";
    case 0x887700a0:
        return "DPERR_NOCAPS - The communication link underneath DirectPlay is not capable of this function.";
    case 0x887700aa:
        return "DPERR_NOCONNECTION - No communication link was established. ";
    case 0x887700be:
        return "DPERR_NOMESSAGES - There are no messages to be received. ";
    case 0x887700c8:
        return "DPERR_NONAMESERVERFOUND - No name server could be found or created. A name server must exist in order to create a player. ";
    case 0x887700d2:
        return "DPERR_NOPLAYERS - There are no active players in the session. ";
    case 0x887700dc:
        return "DPERR_NOSESSIONS - There are no existing sessions for this game. ";
    case 0x887700e6:
        return "DPERR_SENDTOOBIG - The message buffer passed to the IDirectPlay::Send method is larger than allowed. ";
    case 0x887700f0:
        return "DPERR_TIMEOUT - The operation could not be completed in the specified time. ";
    case 0x887700fa:
        return "DPERR_UNAVAILABLE - The requested service provider or session is not available. ";
    case 0x8877010e:
        return "DPERR_BUSY - The DirectPlay message queue is full. ";
    case 0x88770118:
        return "DPERR_USERCANCEL - The user canceled the connection process during a call to the IDirectPlay::Open method.";
    case 0x88770136:
        return "DPERR_SESSIONLOST - The session was lost. ";
    case 0x88770140:
        return "DPERR_UNINITIALIZED - An object was not initialized. ";
    case 0x00000000:
        return "DP_OK - The request completed successfully.";
    }
    return "DPERR_IHAVENOBLOODYIDEA - DirectPlay returned an undocumented return value.";
}

// The trace stub is empty, so the compiler would inline it into every caller
// in this file; the original calls it.
#pragma auto_inline(off)
// FUNCTION: 0x4c9740
void __cdecl HapinetTrace(int)
{
}
#pragma auto_inline(on)

// Returns the (translated) description of the last DirectPlay error: the
// text after " - " in HAPINET_GetDPErrorString's message, falling back to
// DPERR_GENERIC (0x80004005).
// FUNCTION: 0x4c9750
char* GetEnumSessionsErrorText()
{
    char* s = HAPINET_GetDPErrorString(g_enumSessionsResult);
    if (s == 0)
        s = HAPINET_GetDPErrorString(0x80004005);
    s = strstr(s, " - ");
    if (s == 0)
        return HAPINET_GetDPErrorString(0x80004005);
    return Translate(s + 3);
}

// FUNCTION: 0x4c9790
int __stdcall HAPINET_guaranteepackets(int enable)
{
    HapinetTrace("HAPINET_guaranteepackets\n");
    int result = g_guaranteePackets;
    g_guaranteePackets = enable;
    return result;
}

// HAPINET_sendpacket: IDirectPlay2::Send (+0x68); flags is
// DPSEND_GUARANTEED when the guaranteed-packets global is set. Returns
// 0x887700aa when there is no DirectPlay interface.
// FUNCTION: 0x4c97b0
int __stdcall HAPINET_sendpacket(HapinetSession* net, unsigned long from, unsigned long to, void* data, unsigned long size)
{
    HapinetTrace((int)"HAPINET_sendpacket\n");
    int result = 0x887700aa;
    if (net->dp != 0) {
        result = net->dp->Send(from, to, g_guaranteePackets != 0, data, size);
    }
    return result;
}

// HAPINET_sendpacketguaranteed: IDirectPlay2::Send (+0x68) with
// DPSEND_GUARANTEED; returns 0x887700aa when there is no DirectPlay
// interface.
// FUNCTION: 0x4c9800
int __stdcall HAPINET_sendpacketguaranteed(HapinetSession* net, unsigned long from, unsigned long to, void* data, unsigned long size)
{
    HapinetTrace((int)"HAPINET_sendpacketguaranteed\n");
    int result = 0x887700aa;
    if (net->dp != 0) {
        result = net->dp->Send(from, to, 1, data, size);
    }
    return result;
}

// HAPINET_receivepacket: IDirectPlay2::Receive (+0x64) with DPRECEIVE_ALL into
// the from/to player ids kept in the network object; returns
// 0x887700aa when there is no DirectPlay interface.
// FUNCTION: 0x4c9840
int __stdcall HAPINET_receivepacket(HapinetSession* net, void* data, unsigned long* size)
{
    HapinetTrace((int)"HAPINET_receivepacket\n");
    int result = 0x887700aa;
    if (net->dp != 0) {
        result = net->dp->Receive(&net->recvFromId, &net->recvToId, 1, data, size);
    }
    return result;
}

// HAPINET_updategameinfo: copies the game name into the network object, fills
// the session description (DPSESSIONDESC2 at +0x45d: name and the four user
// dwords) and applies it with IDirectPlay2::SetSessionDesc (+0x7c). Returns 1
// on success.
// FUNCTION: 0x4c9890
int __stdcall HAPINET_updategameinfo(HapinetSession* net, char* name, char* data, int d, int c, int b, int a)
{
    HapinetTrace("HAPINET_updategameinfo\n");
    if (net->dp != 0) {
        strncpy(net->sessionName, name, 0x10);
        net->sessionDesc.dwSize = 0x50;
        net->sessionDesc.dwUser4 = a;
        net->sessionDesc.dwUser3 = b;
        net->sessionDesc.dwUser2 = c;
        net->sessionDesc.dwUser1 = d;
        net->sessionDesc.lpszSessionName = name;
        if (net->dp->SetSessionDesc(&net->sessionDesc, 0) == 0)
            return 1;
    }
    return 0;
}

// FUNCTION: 0x4c9920
int __stdcall HAPINET_createnewgame(HapinetSession* net, char* name, int a3, int a4, int a5, int a6, int a7)
{
    HapinetTrace((int)"HAPINET_createnewgame\n");
    if (net->dp != 0) {
        strncpy((char*)net, name, 0x10);
        memset(&net->sessionDesc, 0, 0x50);
        net->sessionDesc.dwMaxPlayers = net->maxPlayers;
        net->sessionDesc.guidApplication = net->application;
        net->sessionDesc.dwSize = 0x50;
        net->sessionDesc.lpszSessionName = name;
        net->sessionDesc.dwUser4 = a7;
        net->sessionDesc.dwUser3 = a6;
        net->sessionDesc.dwUser2 = a5;
        net->sessionDesc.dwUser1 = a4;
        net->sessionDesc.lpszPassword = (char*)a3;
        int r = net->dp->Slot24(&net->sessionDesc, 2);
        unsigned long size = 0x50;
        int hr = net->dp->GetSessionDesc(0, &size);
        if (hr == (int)0x8877001e) {
            DPSESSIONDESC2* buf = (DPSESSIONDESC2*)GameAllocIgnoreTag("DP SESSION DATA2", size);
            if (buf != 0) {
                if (net->dp->GetSessionDesc(buf, &size) == 0)
                    net->sessionDesc.guidInstance = buf->guidInstance;
                GameFreeThunk(buf);
            }
        }
        if (r == 0)
            return 1;
    }
    return 0;
}

// FUNCTION: 0x4c9a70
int __stdcall HAPINET_createorjoinlobbygame(HapinetSession* net, char* password, unsigned long maxPlayers,
                           unsigned long user1, unsigned long user2, unsigned long user3,
                           unsigned long user4)
{
    HapinetTrace((int)"HAPINET_createorjoinlobbygame\n");
    int hr = 0;
    if (password != 0 || maxPlayers != 0 || user1 != 0 || user2 != 0 || user3 != 0 || user4 != 0) {
        DPSESSIONDESC2* d = net->dplConnection->sessionDesc;
        if (password != 0) {
            d->lpszPassword[0] = 0;
            strncat(d->lpszPassword, password, 0x10);
        }
        if (d->dwMaxPlayers == 0) d->dwMaxPlayers = maxPlayers;
        if (d->dwUser1 == 0) d->dwUser1 = user1;
        if (d->dwUser2 == 0) d->dwUser2 = user2;
        if (d->dwUser3 == 0) d->dwUser3 = user3;
        if (d->dwUser4 == 0) d->dwUser4 = user4;
        hr = net->directPlayLobby2->SetConnectionSettings(0, 0, net->dplConnection);
    }
    if (hr >= 0) {
        net->sessionDesc = *net->dplConnection->sessionDesc;
        hr = net->directPlayLobby2->Connect(0, (void**)&net->dp, 0);
    }
    return hr >= 0;
}

// HAPINET_uninitmultiplay: closes and releases the two DirectPlay interfaces,
// releases the two lobby interfaces and frees the connection buffer.
// FUNCTION: 0x4c9b70
void __stdcall HAPINET_uninitmultiplay(HapinetSession* net)
{
    HapinetTrace((int)"HAPINET_uninitmultiplay\n");
    if (net->dp != 0) {
        net->dp->Close();
        net->dp->Release();
        net->dp = 0;
    }
    if (net->dp1 != 0) {
        net->dp1->Close();
        net->dp1->Release();
        net->dp1 = 0;
    }
    if (net->directPlayLobby2 != 0) {
        net->directPlayLobby2->Release();
        net->directPlayLobby2 = 0;
    }
    if (net->directPlayLobby != 0) {
        net->directPlayLobby->Release();
        net->directPlayLobby = 0;
    }
    if (net->dplConnection != 0) {
        GameFreeThunk(net->dplConnection);
        net->dplConnection = 0;
        net->connectionSize = 0;
    }
}

// HAPINET_initmultiplaydefaults: sets two network defaults (0x10 and 1500).
// FUNCTION: 0x4c9c20
void __stdcall HAPINET_initmultiplaydefaults(HapinetSession* net)
{
    HapinetTrace((int)"HAPINET_initmultiplaydefaults\n");
    net->maxPlayers = 0x10;
    net->enumSessionsTimeout = 0x5dc;
}

// DPENUMSESSIONSCALLBACK2 for IDirectPlay3::EnumSessions (HAPINET_getgamescallback).
// Copies a session description into the next slot of the context's game array;
// returns FALSE on DPESC_TIMEDOUT (0x1) to stop the enumeration.
// FUNCTION: 0x4c9c50
int __stdcall HAPINET_getgamescallback(DPSESSIONDESC2* lpsd, int* lpdwTimeout, unsigned int dwFlags, HapinetSession* ctx)
{
    HapinetTrace((int)"HAPINET_getgamescallback\n");
    if ((dwFlags & 1) == 0)
    {
        GameRec_4c9c50* rec = &ctx->games[ctx->gamesCount];
        strcpy(rec->name, lpsd->lpszSessionName);
        rec->user1 = lpsd->dwUser1;
        rec->user2 = lpsd->dwUser2;
        rec->user3 = lpsd->dwUser3;
        rec->user4 = lpsd->dwUser4;
        rec->guidInstance = lpsd->guidInstance;
        rec->maxPlayers = lpsd->dwMaxPlayers;
        ctx->gamesCount++;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4c9d10
int __stdcall HAPINET_justone(int p1, int p2, int p3, int p4, void* p5)
{
    HapinetTrace("HAPINET_justone\n");
    int* addr = (int*)p5;
    addr[0x12] = p1;
    return 0;
}

// FUNCTION: 0x4c9d30
int __stdcall HAPINET_setplayerdatabuffer(HapinetSession* net, unsigned long player, void* data, unsigned long size)
{
    HapinetTrace((int)"HAPINET_setplayerdatabuffer\n");
    if (net->dp != 0 && net->dp->SetPlayerData(player, data, size, 2) == 0) {
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4c9d80
int __stdcall HAPINET_getplayerdatabuffer(HapinetSession* net, unsigned long player, void* data, unsigned long* size)
{
    HapinetTrace((int)"HAPINET_getplayerdatabuffer\n");
    if (net->dp != 0 && net->dp->GetPlayerData(player, data, size, 0) == 0) {
        return 1;
    }
    return 0;
}

// Returns the session's current player count (DPSESSIONDESC2 dwCurrentPlayers,
// +0x2c), reading the session description into a temporary buffer.
// The HRESULT goes through a local: a direct test gives `test eax, eax`
// instead of the original's `cmp eax, ebx`.
// FUNCTION: 0x4c9dd0
int __stdcall HAPINET_getcurrentplayers(HapinetSession* net)
{
    int hr;
    HapinetTrace((int)"HAPINET_getcurrentplayers\n");
    int players = 0;
    if (net->dp != 0) {
        unsigned long size = 0;
        hr = net->dp->GetSessionDesc(0, &size);
        if (hr == (int)0x8877001e) {
            DPSESSIONDESC2* desc = (DPSESSIONDESC2*)GameAllocIgnoreTag("DP SESSION DATA", size);
            hr = net->dp->GetSessionDesc(desc, &size);
            if (hr == 0)
                players = desc->dwCurrentPlayers;
            GameFreeThunk(desc);
        }
    }
    return players;
}

// HAPINET_getgames: enumerates the available DirectPlay sessions. The
// EnumSessions callback (HAPINET_getgamescallback) appends each session to the buffer at
// net+0x4bd and bumps the counter at net+0x4e9. The Windows message queue is
// pumped while the enumeration runs; ESC aborts it. Returns the number of
// sessions collected, or -1 when there is no DirectPlay interface or the
// enumeration failed.
// FUNCTION: 0x4c9e50
int __stdcall HAPINET_getgames(HapinetSession* net, void* sessions, int unused)
{
    HapinetTrace("HAPINET_getgames\n");
    net->gamesCount = 0;
    if (net->dp == 0)
        return -1;
    DPSESSIONDESC2 desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = 0x50;
    desc.guidApplication = net->application;
    desc.guidInstance = net->sessionDesc.guidInstance;
    net->games = (GameRec_4c9c50*)sessions;
    int result;
    do {
        result = net->dp->EnumSessions(&desc, net->enumSessionsTimeout, HAPINET_getgamescallback, net, 0x81);
        g_enumSessionsResult = result;
        MSG msg;
        if (PeekMessageA(&msg, 0, 0, 0, 0)) {
            GetMessageA(&msg, 0, 0, 0);
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        if (PeekKey()) {
            if (PopKey() == 0x1b)
                break;
        }
    } while (result == 0x8877015e);
    if (result == 0)
        return net->gamesCount;
    return -1;
}

// FUNCTION: 0x4c9f90
int __stdcall HAPINET_quitgame(HapinetSession* net)
{
    HapinetTrace((int)"HAPINET_quitgame\n");
    if (net->dp != 0 && net->dp->Close() != 0) {
        return 0;
    }
    return 1;
}

// HAPINET_joingame: fills the session description at +0x45d from the stored
// application GUID (+0x43d) and the requested instance GUID, opens it, then
// re-reads the negotiated description and copies the session name into the
// object's name buffer.
// FUNCTION: 0x4c9fd0
int __stdcall HAPINET_joingame(HapinetSession* net, GUID guid)
{
    HapinetTrace("HAPINET_joingame\n");
    // Nested with one trailing return 0: early returns give `size` its own slot.
    if (net->dp != 0) {
        memset(&net->sessionDesc, 0, 0x50);
        net->sessionDesc.dwSize = 0x50;
        net->sessionDesc.guidApplication = net->application;
        net->sessionDesc.guidInstance = guid;
        if (net->dp->Slot24(&net->sessionDesc, 1) == 0) {
            unsigned long size = 0x50;
            if (net->dp->GetSessionDesc(0, &size) == (int)0x8877001e) {
                DPSESSIONDESC2* p = (DPSESSIONDESC2*)GameAllocIgnoreTag("DP SESSION DATA2", size);
                if (p != 0) {
                    if (net->dp->GetSessionDesc(p, &size) == 0) {
                        net->sessionDesc.guidInstance = p->guidInstance;
                        if (p->lpszSessionName != 0)
                            lstrcpynA(net->sessionName, p->lpszSessionName, 0x11);
                    }
                    GameFreeThunk(p);
                }
            }
            return 1;
        }
    }
    return 0;
}

// DirectPlay EnumConnections callback: ignores the session's own service
// providers (the four GUIDs at 0x50a788), records the connection's GUID, copies
// the connection data into a freshly allocated block and stores the name.
// FUNCTION: 0x4ca100
int __stdcall HAPINET_enumconnections(GUID* guid, void* connection, unsigned long size,
                           DPNAME* name, unsigned long flags, HapinetSession* net)
{
    char local[200];
    HapinetTrace((int)"HAPINET_enumconnections\n");
    for (unsigned int i = 0; i < 4; i++) {
        if (memcmp(guid, g_skippedProviders[i], sizeof(GUID)) == 0)
            return 1;
    }
    net->providerGuids[net->providerCount] = *guid;
    net->providerConnections[net->providerCount].connectionBlob = GameAllocIgnoreTag("DPLAY CONNECTION", size);
    if (net->providerConnections[net->providerCount].connectionBlob == 0)
        return 0;
    memcpy(net->providerConnections[net->providerCount].connectionBlob, connection, size);
    net->providerConnections[net->providerCount].connectionBytes = size;
    sprintf(local, "%s", name->lpszShortNameA);
    char* dest = SkipTextLines(net->providerDescLines, net->providerCount);
    strcpy(dest, local);
    net->providerCount++;
    return 1;
}

// FUNCTION: 0x4ca250
int __stdcall HAPINET_getconnections(HapinetSession* net, int a, int b, int c, GUID* application)
{
    HapinetTrace((int)"HAPINET_getconnections\n");
    if (net->dp == 0) {
        GUID sp = DAT_004fdaf0;
        GUID app = *application;
        if (!HAPINET_initmultiplay(net, &sp, &app)) {
            return 0;
        }
    }
    net->providerGuids = (GUID*)a;
    net->providerCount = 0;
    net->providerConnections = (HapinetProviderConnEntry*)b;
    net->providerDescLines = (char*)c;
    int hr = net->dp->EnumConnections(application, HAPINET_enumconnections, net, 1);
    return hr == 0 ? 1 : 0;
}
// FUNCTION: 0x4ca330
int __stdcall HAPINET_enumproviders(GUID* guid, char* name, unsigned long major,
                           unsigned long minor, HapinetSession* net)
{
    char local[200];
    HapinetTrace((int)"HAPINET_enumproviders\n");
    net->providerGuids[net->providerCount] = *guid;
    sprintf(local, "%s %d.%d", name, major, minor);
    char* slot = SkipTextLines(net->providerDescLines, net->providerCount);
    strcpy(slot, local);
    net->providerCount++;
    return 1;
}

// HAPINET_getproviders: enumerates the service providers into the GUID array
// at +0x431 and the name buffer at +0x439.
// FUNCTION: 0x4ca400
int __stdcall HAPINET_getproviders(HapinetSession* net, int a, int c)
{
    HapinetTrace((int)"HAPINET_getproviders\n");
    net->providerCount = 0;
    net->providerGuids = (GUID*)a;
    net->providerDescLines = (char*)c;
    DirectPlayEnumerateA(HAPINET_enumproviders, net);
    return 1;
}

// HAPINET_passwordrequired: tests flag 0x400 in the current connection's
// description, or in the local copy when there is no connection.
// FUNCTION: 0x4ca450
unsigned int __stdcall HAPINET_passwordrequired(HapinetSession* net)
{
    HapinetTrace((int)"HAPINET_passwordrequired\n");
    if (net->dplConnection != 0)
        return net->dplConnection->sessionDesc->dwFlags & 0x400;
    return net->sessionDesc.dwFlags & 0x400;
}

// HAPINET_initlobbiedconnection: creates the DirectPlay lobby (DPLAYX ordinal
// 4), queries IID_IDirectPlayLobby2A from it, then copies the connection
// settings into a malloc'd buffer. The GUID at 0x4fce18 is
// IID_IDirectPlayLobby2A; slot 8 (+0x20) of that interface is
// GetConnectionSettings, which first reports DPERR_BUFFERTOOSMALL (0x8877001e)
// with a null buffer and the required size.
// FUNCTION: 0x4ca490
int __stdcall HAPINET_initlobbiedconnection(HapinetSession* net)
{
    HapinetTrace((int)"HAPINET_initlobbiedconnection\n");
    if (net->directPlayLobby2 != 0)
        return 0;
    net->directPlayLobby = 0;
    net->directPlayLobby2 = 0;
    net->dplConnection = 0;
    net->connectionSize = 0;
    int hr = DirectPlayLobbyCreateA(0, &net->directPlayLobby, 0, 0, 0);
    if (hr >= 0) {
        hr = net->directPlayLobby->QueryInterface(&IID_IDirectPlayLobby2A, (void**)&net->directPlayLobby2);
        if (hr >= 0) {
            unsigned long size = 0;
            hr = net->directPlayLobby2->GetConnectionSettings(0, 0, &size);
            if (hr == (int)0x8877001e) {
                net->dplConnection = (DPLCONNECTION*)GameAllocIgnoreTag("DPLAY LOBBY CONNECTION", size);
                if (net->dplConnection != 0) {
                    hr = net->directPlayLobby2->GetConnectionSettings(0, net->dplConnection, &size);
                    if (hr >= 0)
                        net->connectionSize = size;
                } else {
                    hr = (int)0x8007000e;
                }
            }
            HapinetTrace((int)"HAPINET_guaranteepackets\n");
            g_guaranteePackets = 1;
        }
    }
    return hr >= 0;
}

// HAPINET_initconnection: opens the session through slot 38 of the DirectPlay
// interface at +0x4c5.
// FUNCTION: 0x4ca590
int __stdcall HAPINET_initconnection(HapinetSession* net, int* connection)
{
    HapinetTrace((int)"HAPINET_initconnection\n");
    return net->dp->Slot38(*connection, 0) >= 0;
}

// The original calls this out of line from HAPINET_getconnections; in
// this file /Ob2 would inline it into the caller.
#pragma auto_inline(off)
// HAPINET_initmultiplay: creates the DirectPlay object from the service
// provider GUID in sp, stores the application GUID and answers whether the
// queried DirectPlay interface came back.
// FUNCTION: 0x4ca5d0
int __stdcall HAPINET_initmultiplay(HapinetSession* net, GUID* sp, GUID* application)
{
    HapinetTrace((int)"HAPINET_initmultiplay\n");
    g_guaranteePackets = 0;
    net->hwnd = *(int*)(GetDisplay() + 0x40);
    net->application = *application;
    net->directPlayLobby = 0;
    net->directPlayLobby2 = 0;
    net->dplConnection = 0;
    net->connectionSize = 0;
    net->dp1 = 0;
    net->dp = 0;
    net->serviceProviderGuid = *sp;
    int hr = DirectPlayCreate((GUID*)&net->serviceProviderGuid, &net->dp1, 0);
    if (hr != 0)
        return 0;
    hr = net->dp1->QueryInterface(&IID_IDirectPlay3A, (void**)&net->dp);
    return hr == 0 ? 1 : 0;
}
#pragma auto_inline(on)

// FUNCTION: 0x4ca6a0
int __stdcall HAPINET_addplayer(HapinetSession* net, unsigned long* id, char* shortName, char* longName,
                           char* name, short versionMajor, short versionMinor)
{
    HapinetTrace((int)"HAPINET_addplayer\n");
    if (net->dp != 0) {
        DPNAME dpname;
        dpname.dwSize = sizeof(DPNAME);
        dpname.dwFlags = 0;
        dpname.lpszShortNameA = shortName;
        dpname.lpszLongNameA = longName;
        strncpy(net->playerLongName, longName, 16);
        strncpy(net->playerShortName, shortName, 16);
        PlayerData_004ca6a0 data;
        memset(&data, 0, sizeof(data));
        strncpy(data.name, name, 16);
        data.versionMajor = versionMajor;
        data.versionMinor = versionMinor;
        int hr = net->dp->CreatePlayer(id, &dpname, 0, &data, sizeof(data), 0);
        return hr == 0 ? 1 : 0;
    }
    return 0;
}

// FUNCTION: 0x4ca780
int __stdcall HAPINET_removeplayer(HapinetSession* net, unsigned long id)
{
    HapinetTrace((int)"HAPINET_removeplayer\n");
    if (net->dp != 0) {
        int hr = net->dp->DestroyPlayer(id);
        return hr == 0 ? 1 : 0;
    }
    return 0;
}

// HAPINET_getplayername: IDirectPlay2::GetPlayerName (+0x54); returns
// 0x88770140 when there is no DirectPlay interface.
// FUNCTION: 0x4ca7c0
int __stdcall HAPINET_getplayername(HapinetSession* net, unsigned long id, void* data, unsigned long* size)
{
    HapinetTrace((int)"HAPINET_getplayername\n");
    if (net->dp != 0) {
        return net->dp->GetPlayerName(id, data, size);
    }
    return 0x88770140;
}

// HAPINET_setplayername: IDirectPlay2::SetPlayerName (+0x78); returns
// 0x88770140 when there is no DirectPlay interface.
// FUNCTION: 0x4ca800
int __stdcall HAPINET_setplayername(HapinetSession* net, unsigned long id, void* name, unsigned long flags)
{
    HapinetTrace((int)"HAPINET_setplayername\n");
    if (net->dp != 0) {
        return net->dp->SetPlayerName(id, name, flags);
    }
    return 0x88770140;
}

// HAPINET_enumplayers: IDirectPlay2::EnumPlayers (+0x30); returns 0x88770140
// when there is no DirectPlay interface.
// FUNCTION: 0x4ca840
int __stdcall HAPINET_enumplayers(HapinetSession* net, void* session, void* callback, void* context, unsigned long flags)
{
    HapinetTrace((int)"HAPINET_enumplayers\n");
    if (net->dp != 0) {
        return net->dp->EnumPlayers(session, callback, context, flags);
    }
    return 0x88770140;
}

// HAPINET_createcompoundaddress: IDirectPlayLobby2::CreateCompoundAddress
// (+0x38) on the lobby interface at +0x4cd; returns 0x88770140 when there
// is none.
// FUNCTION: 0x4ca880
int __stdcall HAPINET_createcompoundaddress(HapinetSession* net, void* elements, unsigned long count, void* address,
                           unsigned long* size)
{
    HapinetTrace((int)"HAPINET_createcompoundaddress\n");
    if (net->directPlayLobby2 != 0) {
        return net->directPlayLobby2->CreateCompoundAddress(elements, count, address, size);
    }
    return 0x88770140;
}

// HAPINET_enumaddress: IDirectPlayLobby::EnumAddress (+0x14) on the lobby
// interface; returns 0x88770140 when there is none. Same shape as 0x4ca840.
// FUNCTION: 0x4ca8c0
int __stdcall HAPINET_enumaddress(HapinetSession* net, void* callback, void* address, unsigned long size, void* context)
{
    HapinetTrace((int)"HAPINET_enumaddress\n");
    if (net->directPlayLobby2 != 0) {
        return net->directPlayLobby2->EnumAddress(callback, address, size, context);
    }
    return 0x88770140;
}

// HAPINET_createdplayinterface: DirectPlayCreate (DPLAYX ordinal 1), then
// QueryInterface for IDirectPlay3A (the GUID at 0x4fcd78) into the second
// slot.
// FUNCTION: 0x4ca900
HRESULT __stdcall HAPINET_createdplayinterface(GUID* sp, DPlayIfacePair* net)
{
    HapinetTrace((int)"HAPINET_createdplayinterface\n");
    memset(net, 0, sizeof(DPlayIfacePair));
    HRESULT hr = DirectPlayCreate(sp, &net->dp, 0);
    if (hr >= 0) {
        hr = net->dp->QueryInterface(&IID_IDirectPlay3A, (void**)&net->dp3);
    }
    return hr;
}

// Releases the IDirectPlay3 interface, then the DirectPlay object.
// FUNCTION: 0x4ca940
int __stdcall HAPINET_releasedplayinterface(DPlayIfacePair* net)
{
    HapinetTrace((int)"HAPINET_releasedplayinterface\n");
    int result = 0;
    if (net->dp3 != 0) {
        result = net->dp3->Release();
        net->dp3 = 0;
    }
    if (net->dp != 0) {
        net->dp->Release();
        net->dp = 0;
    }
    return result;
}

// DirectX 5's DPERR_UNINITIALIZED, MAKE_DPHRESULT(320); the toolchain's
// DirectX 3 <dplay.h> does not define it.
#define DPERR_UNINITIALIZED_004ca990 0x88770140

// HAPINET_getplayeraddress: IDirectPlay2::GetPlayerAddress (+0x48) on the
// interface at +0x4.
// FUNCTION: 0x4ca990
int __stdcall HAPINET_getplayeraddress(DPlayIfacePair* net, unsigned long player, void* data, unsigned long* size)
{
    HapinetTrace((int)"HAPINET_getplayeraddress\n");
    if (net->dp3 != 0)
        return net->dp3->GetPlayerAddress(player, data, size);
    return DPERR_UNINITIALIZED_004ca990;
}

// Returns the lobby object at +0x4c9.
// FUNCTION: 0x4ca9d0
int __stdcall GetCreatedLobbyInterface(HapinetSession* net)
{
    return (int)net->directPlayLobby;
}

// FUNCTION: 0x4ca9e0
int __stdcall GetSessionGuidInstance(int net)
{
    return net + 0x465;
}

// FUNCTION: 0x4ca9f0
int __stdcall QueryOnlineFlags(int)
{
    return 0;
}

// FUNCTION: 0x4caa00
int __stdcall RIReportGameChat(int arg1, int arg2)
{
    return 1;
}

// FUNCTION: 0x4caa10
int __stdcall RIReport(int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10)
{
    return 1;
}

// FUNCTION: 0x4caa20
void __stdcall RISetCallbacks(int report, int notify)
{
    g_reporterReport = report;
    g_reporterNotify = notify;
}
