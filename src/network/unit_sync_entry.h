// UnitSyncEntry: one entry of the shared unit list's std::map, 0x10 bytes
// (Thaldren's SyncChecksumEntry): a unit type's FBI and script checksums,
// whether it is enabled here and whether the peers' checksums match, and its
// unit limit. The one declaration for the five unit_sync files that model the
// map by hand. unit_sync.h only forward-declares it: its own methods take a
// pointer, and including that header would cost these files the ids of every
// UnitSync method.
#ifndef UNIT_SYNC_ENTRY_H
#define UNIT_SYNC_ENTRY_H

struct UnitSyncEntry {
    int fbiChecksum;                   // +0x0, UnitDef::fbiChecksum, the map's key
    int scriptChecksum;                // +0x4
    short unitEnabled;                 // +0x8
    short checksumMatch;               // +0xa
    int maxThisUnit;                   // +0xc
};

#endif
