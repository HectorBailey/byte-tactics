// Decompiled by Claude Opus 5.5. Names are provisional.
// <stdio.h> (or another header, see tools/headers.py) is needed only for
// compiler state: without it the energy + metal sum loads metal first.
#include <stdio.h>

#include "box.h"

struct Unit;

#include "unit_motion.h"

// Unused here: forward declarations of real functions; their symbol ids keep the
// allocation of the functions after the merged views (docs/c2-regalloc.md).
void UpdateMouseScroll();
void UpdateEdgeScroll();

#include "../units/unit.h"

// Unused here: the symbol ids these declarations take keep 0x414770 matching
// (docs/c2-regalloc.md).
void RegisterUnitOrders(void);

#pragma pack(push, 1)
#include "../map/feature.h"
// Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
int __stdcall FindLandingPad(Unit*, int);
unsigned short __stdcall ChooseBuildOption(unsigned int, Unit*);
void __stdcall ClearWeaponTarget(Unit*, int);
void __stdcall DetonateUnitWeapon(Unit*, int);
void __stdcall DrawUnit(void*, Unit*);

#include "order.h"

#include "../game/game.h"

// The order object the vtol orders build: PathOrder, the class order_targets.cpp
// defines. The constructor keeps that definition's argument types so the call
// reaches it by name.
class PathOrder {
public:
    char unknown_0[0x36];
    PathOrder(Order* order, const Vec3& pos);
    void SetAltitude(int);
};

#pragma pack(pop)

extern Game* g_game;

unsigned short __stdcall FindFeatureAtPos(Vec3* pos, Point16* cell, Point16* size);
void __stdcall QueueUnitSpeech(Unit* unit, int kind, char* text);
int __stdcall GetGroundHeight(Vec3* pos);
void __stdcall GetNanoPiecePosition(Unit* unit, Vec3* out);
void __stdcall EmitReverseNanoParticles(Box* from, Vec3* to, int count);
void __stdcall ReclaimFeature(Unit* unit, Vec3* pos);
void __stdcall AttachUnitToPiece(Unit* unit, Unit* target, char p3, char p4);

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
        order->SetAttachedFx((OrderFx*)obj);
        order->flags |= flags | 0xe0;
    }
}

// Order handler "Reclaiming" (second variant, driven by a PathOrder
// move object rather than the turn/approach states of 0x404ad0).
// Stays in a file of its own: it matches only in this file's symbol context.
// FUNCTION: 0x414770
int __stdcall VtolReclaimOrder(Unit* unit, Order* order, int flags)
{
    Point16 cell;
    Point16 size;
    unsigned short index = FindFeatureAtPos(&order->pos, &cell, &size);
    if (index == 0xffff) {
        QueueUnitSpeech(unit, 7, "Reclamation failed");
        return 8;
    }
    Feature* f = &g_game->features[index];
    if (!(f->flags & 0x80))
        return 8;
    switch (order->state) {
    case 0:
        if (unit->motion && (unit->def->flags1 & 0x800) && (unit->def->flags2 & 0x400)) {
            order->AnnounceStatusIfFlagged("Reclaiming");
            PrepVtolClimb(unit, order, 0);
            return 1;
        }
        break;
    case 1: {
        PathOrder* obj = new PathOrder(order, order->pos);
        order->SetAttachedFx((OrderFx*)obj);
        order->flags = 0xe0;
        return 1;
    }
    case 2:
        if (flags & 0x40)
            return 8;
        order->time = (int)(30.0f - (f->metal + f->energy) * -0.5f);
        QueueUnitSpeech(unit, 11, 0);
        return 1;
    case 3:
        order->SetDeadlineTicks(2);
        order->time -= 2;
        if (order->time <= 0)
            return 1;
        unit->workTime = g_game->gameTick + 300;
        if (order->time > 30) {
            Vec3 nano;
            GetNanoPiecePosition(unit, &nano);
            Box box;
            box.lo.x = cell.x << 20;
            box.lo.z = cell.y << 20;
            box.lo.y = GetGroundHeight(&box.lo) << 16;
            box.hi = box.lo;
            box.hi.x += f->footprintX << 20;
            box.hi.z += f->footprintZ << 20;
            box.hi.y += f->height << 16;
            EmitReverseNanoParticles(&box, &nano, 6);
            EmitReverseNanoParticles(&box, &nano, 6);
        }
        return 2;
    case 4:
        ReclaimFeature(unit, &order->pos);
        return 5;
    }
    return 7;
}
