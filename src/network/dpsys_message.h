// DpSysMessage: the DirectPlay system message HandleNetPackets (0x453d40)
// reads when the sender is 0. +0x00 is its DPSYS_ type (3 create, 5 destroy,
// 0x102 set data, 0x103 set name, 0x104 set session description). The fields
// are named for the create-player layout and the set-data layout, which
// overlay: a create has its data at createData/dataSize, a set-data has it at
// data, a set-name has its short name at +0x14 and the long name at longName,
// and a set-session description starts at +0x04. The one declaration for the
// files that share it.
#ifndef DPSYS_MESSAGE_H
#define DPSYS_MESSAGE_H

#include "hapinet_player.h"

#pragma pack(push, 1)
struct DpSysMessage {
    unsigned int type;                 // +0x00
    int playerType;                    // +0x04, 1 is a player
    int id;                            // +0x08
    char* data;                        // +0x0c
    HapinetPlayerData* createData;     // +0x10, a joining player's data
    int dataSize;                      // +0x14
    char* longName;                    // +0x18
};
#pragma pack(pop)

#endif
