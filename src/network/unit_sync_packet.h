// UnitSyncPacket: the 14-byte command packet of type 0x1a that UnitSync sends
// and PacketSequencer queues: a sub-type, a sequence number (0 when it is sent
// without one), the key of the unit it is about, and one payload word or the
// unit's permission bytes. The one declaration of the type for the six
// unit_sync files that hold packets in a std::vector; unit_sync.h only
// forward-declares it, since its own methods take a pointer.
#ifndef UNIT_SYNC_PACKET_H
#define UNIT_SYNC_PACKET_H

#pragma pack(push, 1)

struct UnitSyncPacket {
    unsigned char type;                // +0x0
    unsigned char arg;                 // +0x1
    unsigned int id;                   // +0x2
    int key;                           // +0x6
    union {
        int value;                     // +0xa
        struct {
            unsigned char enabled;     // +0xa
            unsigned char match;       // +0xb
            short limit;               // +0xc
        };
    };
};

#pragma pack(pop)

#endif
