// Decompiled by Claude Opus 5.5, finished by deepseek-v4.1-flash and GPT-6, edited by deepseek-v4.1, finished by GPT-6.1-sol, edited by Claude Opus 5.5, finished by Claude Opus 5.5. re-verified by GPT-6. Names are provisional.
// "Attacking" order handler of aircraft (VTOL). Interrupts hand over to a
// "VTOL_SEEKATTACK" order; the order follows its target unit and gives up
// outside its range. State 0 prepares the order (PrepVtolClimb), states 1 and
// 2 make attack runs past the target, state 4 turns around after a pause,
// state 5 aims at the target again and state 6 flies on and, when the unit is
// below three quarters of its health, sends it to a random repair pad
// ("VTOL_LANDING").
#include <list>
#include <windows.h>
#include <math.h>
#include <vector>

#include "../util/vec3.h"

// A free operator, not a member: util/vec3.h declares Vec3 without methods.
static inline Vec3 operator+(const Vec3& a, const Vec3& b)
{
    Vec3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}
#include "../util/fixed.h"
// Unused here: the symbol ids this header takes keep 0x411f50 matching on
// util/fixed.h (docs/c2-regalloc.md).
#include "../util/hapi_bank.h"

struct Unit;
class UnitMotion {
public:
    char unknown_0[0x22];
    short field_22;                    // +0x22
    char unknown_24[0x2e - 0x24];
    unsigned char flags;               // +0x2e
    void SetFlightMode(Unit* unit, int state);
};
#include "path_order_attach.h"
class MissionType { public: unsigned char index; MissionType(const char*); };

// Unused here: forward declarations of real functions; their symbol ids keep the
// allocation of the functions after the merged views (docs/c2-regalloc.md).
void UpdateMouseScroll();
void UpdateEdgeScroll();
void CenterCameraOnRadarClick();
void CenterCameraOnStartPosition();

#include "../units/unit_def.h"

#pragma pack(push, 1)
struct Struct_Unit96 {
    char pad0[0x146]; unsigned char slotIndex;
};
struct Unit {
    UnitMotion* motion;
    char pad4[0x66 - 4]; short heading;
    char pad68[2];
    union {
        Vec3 pos;                      // +0x6a
        FixedVec3 fixedPos;
    };
    char pad76[0x86 - 0x76]; int carrier;
    char pad8a[8]; UnitDef* def;
    Struct_Unit96* player;
    char pad9a[0x108 - 0x9a]; short health;
    char pad10a[0x110 - 0x10a]; unsigned int flags;
    void ReleaseWeapons(int);
    void ClaimWeapons(int);
    void SetStateBits(int, int);
};
// Local Order view: the file's own Vec3 conflicts with order.h.
struct Order {
    char pad0[5]; unsigned char state; unsigned int flags;
    char padA[0x12 - 0xa]; PathOrderAttach target;
    Vec3 pos;
    Point16 start;                     // +0x2e
    char pad32[0x3e - 0x32]; int range;
    unsigned int flags_42;
    char pad46[0x4a - 0x46]; int next;
    void AnnounceStatusIfFlagged(const char*);
    void SetAttachedFx(int);
    void SetDeadlineTicks(int);
    Order(MissionType type, Unit* target, Vec3* pos, int c, int d, int e);
    char unknown_4e[0x8];
    // Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
    void ReattachFxToUnit();
    void MergeFlagsFromTable(int k);
    void AttachRingApproachGoal(Vec3* pos, int radius1, int radius2);
    Order(Unit* unit, void* file, char* name);
    void OrStatusFlags(unsigned int flags);
    void AttachApproachRadiusGoal(Vec3* pos, int radius);
    void AttachBuildFootprintMarker(Point16 cell, Point16 size);
    Unit* Target();
    void Wait();
    Vec3* Position();
};
struct Struct_Game391e9 {
    char pad0[0xd3c]; int gravity;
};
struct Game {
    char pad0[0x391e9]; Struct_Game391e9* mapInfo;
};
// The order object the vtol orders build: PathOrder, the class order_targets.cpp
// defines. The constructor keeps that definition's argument types so the call
// reaches it by name.
class PathOrder {
public:
    char unknown_0[0x36];
    PathOrder(Order* order, const Vec3& pos);
    void SetAltitude(int);
    void SetApproachRadius(short);
};
class Class_0044e190 {
public:
    char unknown_0[0x36];
    Class_0044e190(Order* order, Unit* unit);
};
#pragma pack(pop)

extern Game* g_game;

int __stdcall RandomInt(int);
int __cdecl FUN_004b70ef(short, int);
int __cdecl FUN_004b7123(short, int);
int __stdcall GetHeadingBetween(Vec3*, Vec3*);
void __stdcall SetWeaponTargetPos(Unit*, Vec3*, int);
void __stdcall ClearWeaponTarget(Unit*, int);
void __stdcall AttachUnitToPiece(Unit* unit, Unit* target, char p3, char p4);
void __stdcall AppendOrder(Unit*, Order*);
void __stdcall AppendOrderToTail(Unit*, Order*);
void __stdcall GetFactoriesInRadius(int player, Vec3* pos, int range, std::vector<Unit*>* out);

static inline Vec3 Offset(short angle, int distance)
{
    Vec3 v;
    v.x = -FUN_004b70ef(angle, distance);
    v.y = 0;
    v.z = -FUN_004b7123(angle, distance);
    return v;
}

// Declared here, defined after AirStrikeOrder: the definition's local object
// is numbered before the function otherwise, which moves its register
// allocation (docs/c2-regalloc.md).
static inline void AttachMove(Order* order, const Vec3& dest, short radius);

// 0x40f200, matched in 0x40f200.cpp; inlined into the state 0 case below.
void __stdcall PrepVtolClimb(Unit* unit, Order* order, unsigned int flags)
{
    unit->ClaimWeapons(3);
    if (unit->carrier)
        AttachUnitToPiece(unit, 0, -1, 2);
    unit->SetStateBits(1, 1);
    if ((unit->motion->flags & 3) == 1) {
        unit->motion->SetFlightMode(unit, 2);
        PathOrder* obj = new PathOrder(order, unit->pos);
        obj->SetAltitude(unit->def->altitude / 2);
        order->SetAttachedFx((int)obj);
        order->flags |= flags | 0xe0;
    }
}

// Stays in a file of its own: it matches only in this file's symbol context.
// FUNCTION: 0x411f50
int __stdcall AirStrikeOrder(Unit* unit, Order* order, unsigned int flags)
{
    if (flags & 0x1000a) {
        if (!order->next && (unit->flags & 0x300000))
            AppendOrderToTail(unit, new Order("VTOL_SEEKATTACK", order->target.owner, &order->pos, 0, 0, 0));
        return 5;
    }
    Unit* target = order->target.owner;
    if (!target && (order->flags_42 & 0x200)) {
        if (!order->next)
            AppendOrderToTail(unit, new Order("VTOL_SEEKATTACK", 0, &unit->pos, 0, 0, 0));
        return 5;
    }
    if (target)
        order->pos = target->pos;
    if (order->range && (int)_hypot(unit->fixedPos.x.whole - order->start.x, unit->fixedPos.z.whole - order->start.y) >= order->range)
        return 5;
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0:
        if (unit->motion && (unit->def->flags1 & 0x800)) {
            order->AnnounceStatusIfFlagged("Attacking");
            PrepVtolClimb(unit, order, 0);
            return 1;
        }
        break;
    case 1: {
        unit->ReleaseWeapons(3);
        unit->ClaimWeapons(0);
        // Pointer locals declared bare, then &order->pos assigned first: orders the two lea.
        Vec3* op;
        Vec3* up;
        op = &order->pos;
        up = &unit->pos;
        if ((int)_hypot(order->pos.x - unit->pos.x, order->pos.z - unit->pos.z) < 0x1e00000) {
            int angle = GetHeadingBetween(up, op);
            Vec3 dest = unit->pos + Offset(angle, 0x8c00000);
            AttachMove(order, dest, 0x3c0);
            order->flags |= 0xe2;
            return 1;
        }
        return 1;
    }
    case 2: {
        // Declared bare and assigned in this order, as in case 1.
        Vec3* op2;
        Vec3* up2;
        op2 = &order->pos;
        up2 = &unit->pos;
        int dist = (int)_hypot(order->pos.x - unit->pos.x, order->pos.z - unit->pos.z);
        int angle = GetHeadingBetween(up2, op2);
        int radius = dist / 2;
        Vec3 dest = unit->pos + Offset(RandomInt(0x4000) + angle - 0x2000, radius);
        AttachMove(order, dest, 0x1e0);
        order->flags = 0x100e8;
        return 1;
    }
    case 3:
        return 1;
    case 4: {
        if (flags & 0xe0)
            return 1;
        UnitDef* def = unit->def;
        // size and rate are loaded before the !rate test.
        int size = def->altitude;
        int rate = g_game->mapInfo->gravity;
        if (!rate)
            break;
        // Float local x: keeps the constant multiply from moving outermost.
        float x = (float)sqrt(size * 2.0 / rate) * 30.0f;
        // attackrunlength is added in a second statement: one sum changes the x87 order.
        int time = (int)(x * unit->motion->field_22) + 1;
        time += def->attackrunlength;
        PathOrder* obj;
        if (order->target.owner)
            obj = (PathOrder*)new Class_0044e190(order, order->target.owner);
        else
            obj = new PathOrder(order, order->pos);
        obj->SetApproachRadius(time);
        order->SetAttachedFx((int)obj);
        order->SetDeadlineTicks(1);
        order->flags |= 0x100e8;
        return 2;
    }
    case 5: {
        unit->ClaimWeapons(0);
        SetWeaponTargetPos(unit, &order->pos, 0);
        int angle = GetHeadingBetween(&unit->pos, &order->pos);
        Vec3 dest = unit->pos + Offset(angle, (unit->def->attackrunlength + 0x3c0) << 16);
        AttachMove(order, dest, 0x3c0);
        order->flags = 0xe2;
        return 1;
    }
    case 6: {
        ClearWeaponTarget(unit, 0);
        Vec3 dest = unit->pos + Offset(unit->heading, 0x5a00000);
        AttachMove(order, dest, 0x80);
        order->flags = 0xe2;
        if (unit->health < unit->def->maxHealth / 4 * 3) {
            std::vector<Unit*> pads;
            GetFactoriesInRadius(unit->player->slotIndex, &unit->pos, 0xf00, &pads);
            if (!pads.empty()) {
                order->SetAttachedFx(0);
                Unit* pad = pads[RandomInt(pads.size())];
                AppendOrder(unit, new Order("VTOL_LANDING", pad, 0, 0, 0, 0));
                order->flags = 0;
                return 0;
            }
            return 0;
        }
        order->state = 3;
        return 2;
    }
    }
    return 7;
}

static inline void AttachMove(Order* order, const Vec3& dest, short radius)
{
    PathOrder* obj = new PathOrder(order, dest);
    obj->SetApproachRadius(radius);
    order->SetAttachedFx((int)obj);
}
