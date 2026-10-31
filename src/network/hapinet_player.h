// HapinetPlayerData: the 0x15-byte player data HAPINET_addplayer (0x4ca6a0,
// hapinet.cpp) hands IDirectPlay::CreatePlayer for a new player, and the host
// reads back from the create-player message (net_game_453d40.cpp): the game
// password and two version words. The host rejects a joiner whose data is not
// 0x15 bytes or whose version words are not 0 and 0x50. The one declaration for
// the files that share it. Nothing is included.
#ifndef HAPINET_PLAYER_H
#define HAPINET_PLAYER_H

#pragma pack(push, 1)
struct HapinetPlayerData {             // 0x15 bytes
    char password[17];                 // +0x00, Thaldren's pszPassword
    short versionMajor;                // +0x11, the host rejects a joiner unless 0
    short versionMinor;                // +0x13, the host rejects a joiner unless 0x50
};
#pragma pack(pop)

#endif
