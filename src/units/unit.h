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
    union {
        struct {
            short bank;                // +0x64
            unsigned short heading;    // +0x66
            short pitch;               // +0x68
        };
        Angles16 angles;               // +0x64
    };
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
    unsigned short netDirtyFlags;      // +0xba
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
    char unknown_10a[4];               // +0x10a
    unsigned char activateFlags;       // +0x10e
    unsigned char cobStateFlags;       // +0x10f
    unsigned int flags;                // +0x110
    unsigned int zBufferFlag;          // +0x114

    unsigned char ChooseWeapon();
    void ReleaseWeapons(unsigned char index);
    void ClaimWeapons(unsigned char index);
    int CanReclaim(void* target);
    int CanRepair(Unit* other);
    int CountCargo();
    int CanLoad(Unit* other);
    void SetStateBits(int mask, int set);
};

#pragma pack(pop)

#endif
