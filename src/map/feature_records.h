// The feature records that features.cpp, features_421f20.cpp and
// features_4224b0.cpp each declared for themselves: the GAF sequence header and
// the animation reference that go with it, the feature unit, the animation slot,
// the name slot, the name list, and the manager that owns the animation and name
// pools.
//
// The pack(1) is load-bearing for three of them. FeatureUnit's int def sits at
// +0x92, which is not 4-byte aligned, so the record is only 0x118 bytes at 1-byte
// packing; AnimSrc is 3 bytes rather than 4, and Anim 12 rather than 16. All three
// files already had these inside a pack(1) region, so the pragma moves here with
// them. AnimEntry, NameEntry, List and AnimManager are the same size at either
// packing: their char arrays need no padding and every pointer and int in
// AnimManager already lands on a 4-byte boundary.
#ifndef FEATURE_RECORDS_H
#define FEATURE_RECORDS_H

#pragma pack(push, 1)

struct AnimSrc {                       // a GAF sequence header
    unsigned short count;              // +0x0
    unsigned char kind;                // +0x2
};

struct Anim {                          // an animation reference (0xc bytes)
    unsigned short index;              // +0x0
    unsigned short value;              // +0x2
    unsigned char kind;                // +0x4
    char unknown_5[3];
    AnimSrc* src;                      // +0x8
};

struct AnimEntry {                     // one slot of the feature animation pool
    short next;                        // +0x0
    short prev;                        // +0x2
    char unknown_4[0x2c];
};

struct FeatureUnit {                   // a feature unit (0x118 bytes)
    char unknown_0[0x92];
    int def;                           // +0x92
    char unknown_96[0xa6 - 0x96];
    unsigned short unitDefIndex;       // +0xa6
    char unknown_a8[0x110 - 0xa8];
    int flags_110;                     // +0x110
    int flags_114;                     // +0x114
};

struct NameEntry {                     // one feature name (0x84 bytes)
    char unknown_0[4];
    char name[0x80];                   // +0x4
};

struct List {                          // the feature name list
    char unknown_0[0x1c];
    int count;                         // +0x1c
    NameEntry* entries;                // +0x20
};

struct AnimManager {                   // the feature animation and name manager
    char unknown_0[0x10];
    AnimEntry* anim;                   // +0x10
    FeatureUnit* featureUnit;          // +0x14
    int activeHead;                    // +0x18
    int idleHead;                      // +0x1c
    int freeHead;                      // +0x20
    char unknown_24[0x58 - 0x24];
    int nameCount;                     // +0x58
    char unknown_5c[0x74 - 0x5c];
    char (*names)[0x100];              // +0x74
};

#pragma pack(pop)

#endif  // FEATURE_RECORDS_H
