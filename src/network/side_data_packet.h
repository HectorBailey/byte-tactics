// SideDataPacket: the player-info broadcast packet (type 0x20), 0xba bytes: a
// type byte then the sender's PlayerInfo (Thaldren's SideData), whose id sits
// at +0x91. net_game.cpp broadcasts it and net_game_453d40.cpp reads it. The
// one declaration for the files that share it.
#ifndef SIDE_DATA_PACKET_H
#define SIDE_DATA_PACKET_H

#include "player_info.h"

#pragma pack(push, 1)
struct SideDataPacket {
    unsigned char type;                // +0x0, 0x20
    PlayerInfo info;                   // +0x1
};
#pragma pack(pop)

#endif
