// UnitSyncEntry: one entry of the shared unit list's std::map, 0x10 bytes: the
// cell a unit occupies and the limit on it. The one declaration for the five
// unit_sync files that model the map by hand. unit_sync.h only forward-declares
// it: its own methods take a pointer, and including that header would cost
// these files the ids of every UnitSync method.
#ifndef UNIT_SYNC_ENTRY_H
#define UNIT_SYNC_ENTRY_H

struct UnitSyncEntry {
    int x;                             // +0x0
    int y;                             // +0x4
    short w;                           // +0x8
    short h;                           // +0xa
    int limit;                         // +0xc
};

#endif
