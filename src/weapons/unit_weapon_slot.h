// UnitWeaponSlot: one of a unit's three weapon slots (Thaldren's
// UnitWeaponSlot, 0x1c bytes), the array at +0x4 of the unit. The one
// declaration of the class, for weapons.cpp, which initialises the slots and
// fires from them, and every file that reads them; the types behind the
// pointers stay private to their own files.
#ifndef UNIT_WEAPON_SLOT_H
#define UNIT_WEAPON_SLOT_H

#include "../units/weapon_aim_cob_cb.h"

struct WeaponDef;

struct UnitWeaponSlot {
    short aimTargetXOrUnitId;          // +0x0
    short aimTargetZOrUnitSentinel;    // +0x2
    WeaponAimCobCb aimCob;             // +0x4
    WeaponDef* weapon;                 // +0xc
    int muzzleAimFromDeltaZ;           // +0x10
    unsigned short reloadTimer;        // +0x14
    short fireHeading;                 // +0x16
    short firePitch;                   // +0x18
    unsigned char stockpile;           // +0x1a
    unsigned char flags;               // +0x1b, the WEAPON_SLOT_ bits below
};

// The pair at +0x0 as one word, which is how unit_save.cpp copies it: 0x487080
// and 0x4876c0 each move it with a single dword, and two short assignments
// there come out as two 2-byte moves and stop matching. The halves are the
// members, the word is this macro, for the same reason as the masks below: an
// anonymous union here costs 14 symbol ids in every file that includes this
// header (#7375).
#define WEAPON_SLOT_AIM_TARGET(slot) (*(int*)&(slot)->aimTargetXOrUnitId)

// The bits of `flags`, as masks: macros take no symbol ids, so every includer
// keeps its count, and a mask test compiles as it did. InitUnitWeaponSlots
// (0x49e070) writes the slot index and HAS_WEAPON, and sets ACTIVE.
#define WEAPON_SLOT_AIMING      0x01  // the Aim script has been started (weapons.cpp)
#define WEAPON_SLOT_HAS_WEAPON  0x02  // the slot's weapon has a nonzero index
#define WEAPON_SLOT_INDEX       0x0c  // bits 2-3, which of the three slots this is
#define WEAPON_SLOT_INDEX_SHIFT 2
#define WEAPON_SLOT_ACTIVE      0x10  // Thaldren's Unit_ActivateWeaponSlot sets it

#endif
