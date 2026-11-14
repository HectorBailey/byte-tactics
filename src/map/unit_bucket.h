// UnitBucket: one cell of the map's spatial grid, 10 bytes. first is the head of
// the list of units standing in the cell, and they link through their own
// cargoNext (Unit +0x8e). Thaldren has the same block as SpatialBucket in
// data/thaldren/structs.csv, 0xa bytes: bMaxHeight at +0x0, bMaxHeightScan at
// +0x1 and dwEdgeFlags at +0x2 fill the first six bytes and pUnitHead at +0x6 is
// the list head; nothing in src/ reads the first six.
//
// Both files include this inside their own pack(1) region, so the record is 10
// bytes in each, which is what their bucket grids are indexed by.
#ifndef UNIT_BUCKET_H
#define UNIT_BUCKET_H

struct Unit;

struct UnitBucket {
    char unknown_0[6];
    Unit* first;                        // +0x6
};

#endif  // UNIT_BUCKET_H
