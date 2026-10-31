// Unit: one game unit (Thaldren's UnitInstance), 0x118 bytes. The one
// declaration of the struct for every file that reads or writes a unit.
// Pointer members are forward declarations; the by-value blocks (the three
// weapon slots and the resource account) come from their own headers.
#ifndef UNIT_H
#define UNIT_H

#include "../game/unit_resources.h"
#include "../util/angles.h"
#include "../util/vec3.h"
#include "../weapons/unit_weapon_slot.h"
#include "unit_def.h"

struct Player;
class CobScript;
struct ObjectState;
class PathOrderAttach;
class SpatialBucket;
class UnitMotion;
struct Order;

#pragma pack(push, 1)

struct Unit {
    UnitMotion* motion;                // +0x0
    UnitWeaponSlot weapons[3];         // +0x4, stride 0x1c
    float extraction;                  // +0x58
    Order* list;                       // +0x5c
    Order* list2;                      // +0x60
    // Not a union with the three angles: the flag word's union below takes the
    // 14 symbol ids that one did, so every includer keeps its count (docs/c2-regalloc.md).
    Angles16 angles;                   // +0x64, bank, heading and pitch
    Vec3 pos;                          // +0x6a
    Point16 cell;                      // +0x76
    short losCacheCellX;               // +0x7a
    short losCacheCellZ;               // +0x7c
    Point16 footprint;                 // +0x7e
    SpatialBucket* spatialBucket;      // +0x82
    Unit* carrier;                     // +0x86
    Unit* cargo;                       // +0x8a
    Unit* cargoNext;                   // +0x8e
    UnitDef* def;                      // +0x92
    Player* player;                    // +0x96
    CobScript* script;                 // +0x9a
    ObjectState* state;                // +0x9e
    PathOrderAttach* head;             // +0xa2
    unsigned short unitDefIndex;       // +0xa6
    unsigned short id;                 // +0xa8
    unsigned short hoverBobPhase;      // +0xaa
    int group;                         // +0xac
    int workTime;                      // +0xb0
    char unknown_b4[4];                // +0xb4, no view names it
    unsigned short killCount;          // +0xb8
    // The net pending mask, one bit each: SetUnitValue sets `dirty` with a
    // single or, which a byte member would not give.
    unsigned short unknown_ba : 2;
    unsigned short dirty : 1;          // +0xba bit 2
    unsigned short unknown_ba_3 : 13;
    UnitResources resourceSlot;        // +0xbc, 0x34 bytes
    Unit* attacker;                    // +0xf0
    unsigned char lastAttackerSlot;    // +0xf4
    unsigned char lastDamageType;      // +0xf5
    unsigned char healthPercent;       // +0xf6
    unsigned char prevHealthPercent;   // +0xf7
    unsigned char losSightFrameIdx;    // +0xf8
    unsigned char transportPiece;      // +0xf9
    unsigned char recentlyDamagedTimer; // +0xfa
    int postTransferHoldoff;           // +0xfb
    unsigned char playerIndex;         // +0xff
    int unfinishedInitZero;            // +0x100
    float buildLeft;                   // +0x104
    short health;                      // +0x108
    int sfxOccupy;                     // +0x10a, Thaldren's nSfxOccupy
    // The script's activation bits: bit 0 on, bit 1 on2, bit 2 the cloak range
    // ring. One byte, as the files that pass it whole read it.
    unsigned char activateFlags;       // +0x10e
    unsigned char bit0 : 1;            // +0x10f bit 0
    unsigned char bit1 : 1;
    unsigned char bit2 : 1;
    unsigned char bit3 : 1;
    unsigned char unknown_10f : 4;
    // The state word, as one integer or as its named bits (Thaldren's dwFlags,
    // whose note on UnitInstance+0x110 gives the bits named here).
    union {
        unsigned int flags;            // +0x110
        struct {
            unsigned int occupyLayer : 2;   // bits 0-1, the map layer the unit occupies
            unsigned int moveRate : 2;      // bits 2-3
            unsigned int selected : 1;      // bit 4
            unsigned int selectable : 1;    // bit 5, Thaldren's MakeSelectable
            unsigned int unknown_110_6 : 2;
            unsigned int radar : 1;         // bit 8
            unsigned int sonar : 1;         // bit 9
            unsigned int jammed : 1;        // bit 10
            unsigned int cloak : 1;         // bit 11, cloaking wanted
            unsigned int unknown_110_12 : 1;
            unsigned int hpDirty : 1;       // bit 13, build progress or health changed
            unsigned int dying : 1;         // bit 14
            unsigned int unknown_110_15 : 1;
            unsigned int moved : 1;         // bit 16, moved this tick
            unsigned int hideCargo : 1;     // bit 17, carried with no piece (0xff)
            unsigned int moveOrder : 2;     // bits 18-19, the standing move order
            unsigned int fireOrder : 2;     // bits 20-21, the standing fire order
            unsigned int buildMenuOpen : 1; // bit 22
            unsigned int buildMenuPage : 3; // bits 23-25
            unsigned int unknown_110_26 : 2;
            unsigned int alive : 1;         // bit 28
            unsigned int building : 1;      // bit 29
            unsigned int isAirBase : 1;     // bit 30
            unsigned int hasWeapons : 1;    // bit 31
        };
    };
    unsigned int zBufferFlag;          // +0x114

    unsigned char ChooseWeapon();
    void ReleaseWeapons(unsigned char index);
    void ClaimWeapons(unsigned char index);
    int CanReclaim(void* target);
    int CanRepair(Unit* other);
    int CountCargo();
    int CanLoad(Unit* other);
    // The callers pass the mask as a byte, with no zero extension, so it is an
    // unsigned char here; unit.cpp's definition reads it as an int.
    void SetStateBits(unsigned char mask, int set);
};

#pragma pack(pop)

#endif
