// LobbyDataSyncPacket: the lobby data sync request (type 0x21, 10 bytes) that
// SendLobbySyncRequests (0x450530, net_game_450530.cpp; Thaldren's
// Lobby_SendLobbyDataSyncRequests) sends for a slot whose lobby data has not
// synced, and the packet pump (0x453d40) answers: the flag, the player asked
// about and the caller. The one declaration for the files that share it.
// Nothing is included.
#ifndef LOBBY_DATA_SYNC_PACKET_H
#define LOBBY_DATA_SYNC_PACKET_H

#pragma pack(push, 1)
struct LobbyDataSyncPacket {
    unsigned char type;                // +0x0, 0x21
    unsigned char flag;                // +0x1
    int id;                            // +0x2, the player asked about
    int arg;                           // +0x6, the caller
};
#pragma pack(pop)

#endif
