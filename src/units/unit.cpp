// Decompiled by space-bunny-free, Haiku, deepseek-v4.1-flash, Claude Sonnet 5.5, GPT-6.1-sol, deepseek-v4.1 and mimo-v2.6-pro. Names are provisional.

// <windows.h> is needed for CanRepair's operand order (see there).
#include <windows.h>
// Only for its symbol ids: the six methods here match at one symbol count.
#include <ddraw.h>

#include "cob_script.h"
#include "../weapons/unit_weapon_slot.h"

// Unused here: the symbol ids these declarations take keep the six methods
// matching (docs/c2-regalloc.md).
char IsMemFussy();
char IsGonzo();
char IsMemSet();
int GetMemSetValue();
void SetMemFussyDefault(void);

#include "unit_def.h"

// Unused here: these headers and forward declarations take the symbol ids that
// keep CanLoad (0x489a90) matching (docs/c2-regalloc.md).
#include "../util/hapi_bank.h"
#include "../map/mission.h"
#include "../sound/sound.h"

// Unused here: the symbol ids these declarations take keep 0x489800, 0x4898b0,
// 0x489960, 0x4899b0, 0x489a70 and 0x489a90 matching (docs/c2-regalloc.md).
void RegisterUnitOrders(void);
void RegisterGroundOrders(void);
void EnableAICommands(void);
void RegisterAICommands(void);
void FUN_00406f40(void);

// The unit is units/unit.h's. SetStateBits keeps its own file: it only matches
// at the symbol count its pre-#7097 view gave it (docs/c2-regalloc.md).
#include "unit.h"

#pragma pack(push, 1)
// Bit 19 is a one-bit bitfield (MSVC shifts it down to test it), while bit 8
// of the same word is tested as a plain mask. Kept as a view: unit_def.h keeps
// flags2 as a plain int, and the shift is what 0x489a90 emits.
union Flags_00489a90 {
    struct {
        unsigned int unknown_0 : 19;
        unsigned int flag19 : 1;       // bit 19, tested with shr eax, 0x13
        unsigned int unknown_1 : 12;
    } bits;
    int all;
};

struct Game {
    char unknown_0[0x1427f];
    unsigned char seaLevel;            // +0x1427f
};

#include "../network/player.h"

struct ActivateFlagsPacket {
    unsigned char type;                // +0x0
    short unitId;                      // +0x1, the unit id
    unsigned char activateFlags;       // +0x3, the new state
};
#pragma pack(pop)

extern Game* g_game;

// One virtual slot, called on the object a link belongs to.
class OrderBase {
public:
    virtual void OrStatusFlags(unsigned int value);
};

// The links of the owner's list; the head of a unit's list is at +0xa2.
class PathOrderAttach {
public:
    void* vptr;                        // +0x0
    void* owner;                       // +0x4
    PathOrderAttach* next;             // +0x8
    OrderBase* value;                  // +0xc
};

#pragma pack(push, 1)
#pragma pack(pop)

void __stdcall QueueUnitSpeech(Unit* unit, int kind, char* text);
void __stdcall MarkSelectionOrdersDirty(Unit* unit);
int __stdcall BroadcastPacket(int player, void* data, int size);

// The twin of ClaimWeapons (0x4898b0) with bit 4 the other way round: it
// requires bit 4 clear and then sets it.
// FUNCTION: 0x489800
void Unit::ReleaseWeapons(unsigned char index)
{
    if (index == 3) {
        this->ReleaseWeapons(0);
        this->ReleaseWeapons(1);
        index = 2;
    }
    UnitWeaponSlot* e = &weapons[index];
    if ((e->flags & WEAPON_SLOT_HAS_WEAPON) != 0 && (e->flags & WEAPON_SLOT_ACTIVE) == 0) {
        e->flags |= WEAPON_SLOT_ACTIVE;
        int i = index;
        UnitWeaponSlot* p = &weapons[i];
        if (p->aimTargetXOrUnitId != 0 || p->aimTargetZOrUnitSentinel != (short)0x8000) {
            p->aimTargetXOrUnitId = 0;
            p->aimTargetZOrUnitSentinel = (short)0x8000;
            script->FindScript("StartBuilding");
            script->StartScriptWithArgs("TargetCleared", 0, 0, 1, i, 0, 0, 0);
        }
    }
}

// Clears the unit's target entry `index` (the same reset as ClearWeaponTarget) and
// tells the unit's script "StartBuilding" and "TargetCleared", but only when
// the entry's flag byte at +0x1b has bit 1 and bit 4 both set, and the entry
// is not already clear. Note that bit 4 is *cleared* again on entry, so a set
// bit 4 makes the test pass and then gets turned off: see the note at the end.
// Index 3 is not an entry of its own: it recurses into 0 and 1 and then works
// on entry 2, so the byte parameter is a plain unsigned char.
// FUNCTION: 0x4898b0
void Unit::ClaimWeapons(unsigned char index)
{
    if (index == 3) {
        this->ClaimWeapons(0);
        this->ClaimWeapons(1);
        index = 2;
    }
    UnitWeaponSlot* e = &weapons[index];
    if ((e->flags & WEAPON_SLOT_HAS_WEAPON) != 0 && (e->flags & WEAPON_SLOT_ACTIVE) != 0) {
        e->flags &= ~WEAPON_SLOT_ACTIVE;
        int i = index;
        UnitWeaponSlot* p = &weapons[i];
        if (p->aimTargetXOrUnitId != 0 || p->aimTargetZOrUnitSentinel != (short)0x8000) {
            p->aimTargetXOrUnitId = 0;
            p->aimTargetZOrUnitSentinel = (short)0x8000;
            script->FindScript("StartBuilding");
            script->StartScriptWithArgs("TargetCleared", 0, 0, 1, i, 0, 0, 0);
        }
    }
    // The guard only fires when bit 4 is *set* and then clears it, so this
    // entry clears its target exactly once per set of bit 4 and never sets
    // it. The twin at 0x489800 is the other way round (it requires bit 4 to be
    // clear and then sets it), so this looks like the two halves of one
    // "toggle the target flag" that was written twice instead of once.
}

// FUNCTION: 0x489960
int Unit::CanReclaim(void *param_1)
{
    Unit* other = (Unit*)param_1;
    unsigned int val1 = def->flags2;

    if ((val1 & 0x400) != 0) {
        unsigned int val2 = other->flags;

        if ((val2 & 3) != 2) {
            unsigned int val3 = other->def->flags2;

            if ((val3 & 0x1000) == 0) {
                return 1;
            }
        }
    }

    return 0;
}

// Same class as 0x489a90: a `[ecx+0x92]` def pointer, the def flags word at
// +0x245 and the def word at +0x241, and the g_game byte at +0x1427f.
//
// A yes/no test between two units of that class.
// FUNCTION: 0x4899b0
int Unit::CanRepair(Unit* other)
{
    if (other
        && (def->flags2 & 0x200)
        && (other->health != other->def->maxHealth)
        && ((other->flags & 3) != 2)) {
        if (def->flags1 & 0x800) {
            if (!(def->flags1 & 0x200000) && ((short*)&other->pos.y)[1] + other->def->modelMaxYHigh < g_game->seaLevel)
                goto fail;
        }
        // Tests the same condition again, not an else.
        if (!(def->flags1 & 0x800)) {
            if (((short*)&other->pos.y)[1] + other->def->modelMaxYHigh < g_game->seaLevel - def->maxwaterdepth)
                goto fail;
        }
        return 1;
    }
    // Failures are a goto to this shared return 0, placed after the return 1.
fail:
    return 0;
}

// FUNCTION: 0x489a70
int Unit::CountCargo()
{
    int count = 0;
    Unit* node = cargo;
    while (node) {
        if (node->carrier == this)
            count++;
        node = node->cargoNext;
    }
    return count;
}

// A yes/no test between two units of that class: it returns 1 only when every
// check below passes, and 0 from eight separate early returns.
// FUNCTION: 0x489a90
int Unit::CanLoad(Unit* other)
{
    UnitDef* theirDef = other->def;
    if (((Flags_00489a90*)&theirDef->flags2)->bits.flag19)
        return 0;
    UnitDef* ourDef = def;
    if (!(ourDef->flags2 & 0x100))
        return 0;
    if (CountCargo() >= (unsigned char)ourDef->transportcapacity)
        return 0;
    if (!other->motion)
        return 0;
    if (theirDef->footprint.x > ourDef->capacity)
        return 0;
    if ((other->flags & 3) == 2)
        return 0;
    if (!(ourDef->flags1 & 0x800) && theirDef->minwaterdepth >= 0)
        return 0;
    if (((int*)&other->pos.y)[0] + theirDef->modelMaxY <= (g_game->seaLevel << 16))
        return 0;
    if (other->buildLeft != 0.0f)           // the fcomp tests equality, not a range
        return 0;
    return 1;
}

