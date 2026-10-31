// SetStateBits, split out of unit.cpp (#7099): it only matches at the symbol
// count unit.cpp's own Unit view gave it, so unit.cpp keeps the header and this
// file keeps the view (docs/c2-regalloc.md).
// Decompiled by space-bunny-free, Haiku, deepseek-v4.1-flash, Claude Sonnet 5.5, GPT-6.1-sol, deepseek-v4.1 and mimo-v2.6-pro. Names are provisional.

// <windows.h> is needed for CanRepair's operand order (see there).
#include <windows.h>
// Only for its symbol ids: SetStateBits matches at one symbol count.
#include <ddraw.h>

#include "cob_script.h"
#include "../weapons/unit_weapon_slot.h"

// Unused here: the symbol ids these declarations take keep SetStateBits (0x48b090)
// matching (docs/c2-regalloc.md).
char IsMemFussy();
char IsGonzo();
char IsMemSet();
int GetMemSetValue();
void SetMemFussyDefault(void);

#include "unit_def.h"

// Unused here: these headers and forward declarations take the symbol ids that keep
// CanLoad (0x489a90) and SetStateBits (0x48b090) matching (docs/c2-regalloc.md).
#include "../util/hapi_bank.h"
#include "../map/mission.h"
#include "../sound/sound.h"

// Unused here: the symbol ids these declarations take keep 0x48b090 matching
// (docs/c2-regalloc.md).
void ResetAIPlayers(void);
void RegisterVtolOrders(void);
void StepAllGafSequences(void);
void ResetNetStats(void);
void FUN_004161f0(void);
void InitCommands(void);
int UpdatePlacementGhostValidity(void);
void RefreshSelectionOrders(void);
void DispatchOrdersPanelPageFlags(void);
void ResetCameraState(void);
void FindLocalCommander(void);
void ClampCameraPosition(void);
void ClampCameraTarget(void);
void UpdateScreenShake(void);
void UpdateCameraFollow(void);
void BeginMouseScroll(void);
void EndMouseScroll(void);
void UpdateMouseScroll(void);
void UpdateEdgeScroll(void);
void CenterCameraOnRadarClick(void);
void CenterCameraOnStartPosition(void);
void RegisterDataArchives(void);
int FUN_0041d8a0(void);
int GetCdPathMismatch(void);
void CreateGameObject(void);
void InitMissionStatus(void);
void SetUpEndMissionScreen(void);
void StartScreenFade(void);
void StepScreenFade(void);
void ScheduleFadeTick(void);
int IsFadeDone(void);
void StepPaletteFade(void);
void FillEndGameStatistics(void);
int AreStatBarsComplete(void);
int ShouldShowNextMission(void);
void OpenEndMissionScreen(void);

// Unused here: the symbol ids these declarations take keep 0x489800, 0x4898b0,
// 0x489960, 0x4899b0, 0x489a70, 0x489a90, 0x48b090 matching (docs/c2-regalloc.md).
void RegisterUnitOrders(void);
void RegisterGroundOrders(void);
void EnableAICommands(void);
void RegisterAICommands(void);
void FUN_00406f40(void);


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
class Unit {
public:
    int motion;                        // +0x0
    UnitWeaponSlot weapons[3];         // +0x4, stride 0x1c
    char unknown_58[0x6e - 0x58];
    union {
        int f6e;                       // +0x6e, 16.16
        struct {
            unsigned short f6e_fraction;
            short f70;                 // +0x70
        };
    };
    char unknown_72[0x86 - 0x72];
    Unit* carrier;                     // +0x86, the unit this one is loaded on
    Unit* cargo;                       // +0x8a, list head of the cargo count
    Unit* cargoNext;                   // +0x8e
    UnitDef* def;                      // +0x92
    Player* player;                    // +0x96
    CobScript* script;                 // +0x9a
    void* state;                       // +0x9e
    PathOrderAttach* head;             // +0xa2
    char unknown_a6[0xa8 - 0xa6];
    unsigned short id;                 // +0xa8
    char unknown_aa[0x104 - 0xaa];
    float buildLeft;                   // +0x104
    short health;                      // +0x108
    char unknown_10a[0x10e - 0x10a];
    unsigned char activateFlags;       // +0x10e
    char cobStateFlags;
    int flags;                         // +0x110

    unsigned char GetState() { return activateFlags; }

    void ReleaseWeapons(unsigned char index);
    void ClaimWeapons(unsigned char index);
    int CanReclaim(void *param_1);
    int CanRepair(Unit* other);
    int CountCargo();
    int CanLoad(Unit* other);
    void SetStateBits(int mask, int set);
};
#pragma pack(pop)

void __stdcall QueueUnitSpeech(Unit* unit, int kind, char* text);
void __stdcall MarkSelectionOrdersDirty(Unit* unit);
int __stdcall BroadcastPacket(int player, void* data, int size);

// The twin of ClaimWeapons (0x4898b0) with bit 4 the other way round: it
// requires bit 4 clear and then sets it.
// Sets or clears bits of the unit's state byte at +0x10e and reacts to the three
// bits that mean active (1), building (8) and working (4). The gained and the
// lost bits are tested separately: each gained bit plays its script event and
// its message, and losing the working bit (4) tells every object linked to this
// unit (the list head at +0xa2) to update, then a network packet (0x11) tells
// the owner when the owner is a real player (1 or 2).
static inline int HasBit(unsigned char bits) { return 1 & bits; }

static inline int LostBits(unsigned char was, int is) { return (unsigned char)was & ~is; }

static inline unsigned char GainedBits(unsigned char was, int is) { return (unsigned char)(~was & (int)is); }

static inline unsigned char AsByte(unsigned char bits) { return (unsigned char)bits; }

// Unused here: their symbol ids are what SetStateBits (0x48b090) matches at
// (docs/c2-regalloc.md). They stand where the removed Point_004898b0,
// Flags_004898b0, Entry_004898b0 and Node_00489a90 views stood.
void FreeCobScript(int);
int CheckMapCrc();
int FindUnusedLogo();
int LoadUnitInfo();
void AddDownloadBuildOptions();
void BuildAllPassMaps(void);
void CheckDownloadableFlags();
void FreeDownloadMenus();
void FreeMovementClasses(void);
void FreeUnitCategories();
void FreeUnitInfo();
void FreeUnitMemory(void);
void FreeUnitTypes();
void InitUnitCategories();
void KillAllUnits(void);

// FUNCTION: 0x48b090
void Unit::SetStateBits(int mask, int set)
{
    // Old state read through the in-class accessor: it fixes the set arm's load order.
    unsigned char lost, gained, old = GetState();
    int isOne, active;
    PathOrderAttach* link;
    int now;
    if (set)
        now = old | (unsigned char)mask;
    else
        now = old & ~(mask & 0xff);
    activateFlags = (unsigned char)now;
    {
        if ((unsigned char)now != old) {
            // LostBits returns int and narrows its first operand, and lost then
            // goes round AsByte: that pair is what makes MSVC 5 compute `lost`
            // into cl, the original's register, instead of into al. isOne and
            // active are unused, and so are HasBit and GainedBits above; all
            // four are dead weight the bytes need.
            lost = LostBits(old, now);
            unsigned char newLost = AsByte(lost);
            gained = ~old & now;
            lost = (unsigned char)newLost;
            if (gained & 1) {
                script->StartScript("Activate", 0, 0);
                QueueUnitSpeech(this, 3, 0);
            }
            if (lost & 1) {
                script->StartScript("Deactivate", 0, 0);
                QueueUnitSpeech(this, 4, 0);
            }
            if (gained & 8)
                script->StartScript("StartBuilding", 0, 0);
            if (lost & 8)
                script->StartScript("StopBuilding", 0, 0);
            if (gained & 4) {
                QueueUnitSpeech(this, 0xe, 0);
                for (link = head; link; link = link->next) {
                    if (link->value)
                        link->value->OrStatusFlags(0x10000);
                }
            }
            if (lost & 4)
                QueueUnitSpeech(this, 0xf, 0);
            MarkSelectionOrdersDirty(this);
            if (player->active != 0) {
                if (player->type == 1 || player->type == 2) {
                    ActivateFlagsPacket packet;
                    packet.type = 0x11;
                    packet.unitId = id;
                    packet.activateFlags = activateFlags;
                    BroadcastPacket(player->id, &packet, 4);
                }
            }
        }
    }
}
