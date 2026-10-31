// Decompiled by Claude Opus 5.5. Names are provisional.
// Stays in its own file: in unit_orders.cpp the feature pointer is added up in the other operand order and lstrcpynA is called directly instead of through the import.
#include <windows.h>

#include "box.h"

#include "../util/angles.h"

struct Unit;

#include "mission_type.h"

#include "path_order_attach.h"

#include "../map/mission.h"

#include "../units/unit_def.h"

#pragma pack(push, 1)
#include "../map/feature.h"

struct FeatureSpot {
    char unknown_0[0x20];
    Angles16 rot;                         // +0x20
    char unknown_26[0x30 - 0x26];
};

struct Cell {
    char unknown_0[8];
    unsigned short feature;            // +0x8
    unsigned short spot;               // +0xa
    char unknown_c;
};

#include "../network/player.h"

#include "../units/unit.h"

#include "order.h"

// g_game is declared before game.h: with the header's symbols ahead of it, its
// id moves 0x404db0 (docs/c2-regalloc.md).
struct Game;
extern Game* g_game;
#include "../game/game.h"

struct FeatureDamagePacket {
    unsigned char type;
    unsigned char sub;
    short x;
    short z;
};
#pragma pack(pop)

unsigned short __stdcall FindFeatureAtPos(Vec3* pos, Point16* cell, Point16* size);
void __stdcall QueueUnitSpeech(Unit* unit, int kind, char* text);
int __stdcall RandomInt(int range);
int __stdcall GetGroundHeight(Vec3* pos);
unsigned short __stdcall GetHeadingBetween(Vec3* from, Vec3* to);
void __stdcall StartBuildingScript(Unit* unit, Order* order, short turn);
int __stdcall WaitIfNotInBuildStance(Unit* unit, Order* order, int flags);
unsigned short __stdcall FindUnitTypeId(char* name);
void __stdcall GetNanoPiecePosition(Unit* unit, Vec3* out);
void __stdcall EmitNanoParticles(Vec3* from, Box* to, int count);
Unit* __stdcall CreateUnit(unsigned char player, unsigned short type, Vec3 pos, int a, int b, int c);
Cell* __stdcall GetOriginCellAtPosition(Vec3* pos);
unsigned short __stdcall GetCellFeature(Cell* cell);
Cell* __stdcall GetMapCellAtPosition(Vec3* pos);
void __stdcall RemoveFeature(void* target, int flag);
int __stdcall BroadcastPacket(int player, void* data, int size);
void __stdcall MarkSelectionOrdersDirty(Unit* unit);
MissionType __stdcall GetOrderType(unsigned char mode, Unit* unit, Unit* target, int flags);
void __stdcall AppendOrder(Unit* owner, Order* node);

// Order handler "Resurrecting": raises the unit a wreck (feature) came from.
// FUNCTION: 0x404db0
int __stdcall ResurrectOrder(Unit* unit, Order* order, int flags)
{
    // Point16, not a short or int: a plain local changes how x is moved.
    Point16 cell;
    Point16 size;
    // Set before the state test although no path reads it: the original
    // computes it there.
    Feature* f = &g_game->features[0xffff];
    if (order->state <= 5) {
        unsigned short index = FindFeatureAtPos(&order->pos, &cell, &size);
        if (index == 0xffff) {
            QueueUnitSpeech(unit, 7, "Resurrection failed");
            return 8;
        }
        f = &g_game->features[index];
        if (!(f->flags & 0x80))
            return 8;
    }
    switch (order->state) {
    case 0:
        if (unit->motion && (unit->def->flags2 & 0x800)) {
            order->AttachBuildFootprintMarker(cell, size);
            order->flags = 0xe0;
            return 1;
        }
        return 7;
    case 1: {
        if (flags & 0x40)
            return 8;
        Vec3 pos;
        pos.x = (size.x + cell.x * 2) << 19;
        pos.z = (size.y + cell.y * 2) << 19;
        pos.y = (RandomInt(f->height) + GetGroundHeight(&pos)) << 16;
        StartBuildingScript(unit, order, GetHeadingBetween(&unit->pos, &pos) - unit->angles.heading);
        return 1;
    }
    case 2:
        return WaitIfNotInBuildStance(unit, order, 0);
    case 3: {
        char name[0x40];
        lstrcpynA(name, f->name, 0x40);
        for (int i = 0; i < 0x40; i++) {
            if (name[i] == '_') {
                name[i] = 0;
                break;
            }
        }
        order->unitType = FindUnitTypeId(name);
        if (!order->unitType) {
            QueueUnitSpeech(unit, 7, "Ressurection failed");
            return 8;
        }
        order->countdown = (int)(g_game->unitDefs[order->unitType].buildtime * 0.3 / (unit->def->buildRate / 30));
        QueueUnitSpeech(unit, 11, 0);
        return 1;
    }
    case 4:
        if (order->countdown--) {
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
            EmitNanoParticles(&nano, &box, 6);
            unit->workTime = g_game->gameTick + 300;
            order->SetDeadlineTicks(1);
            return 2;
        }
        break;
    case 5: {
        // The attach link starts at +0x12 (the order's unknown_12 slot);
        // its owner is Order::target at +0x16.
        ((PathOrderAttach*)order->unknown_12)->SetUnit(CreateUnit(unit->playerIndex, order->unitType, order->pos, 0, 1, 0));
        if (!order->target) {
            QueueUnitSpeech(unit, 7, "Unable to create any more units");
            order->SetDeadlineTicks(300);
            return 2;
        }
        Cell* c = GetOriginCellAtPosition(&order->pos);
        if (GetCellFeature(c) >= 0xfffb)
            return 8;
        FeatureSpot* spot = &g_game->spots[c->spot];
        if (spot)
            order->target->angles = spot->rot;
        RemoveFeature(GetMapCellAtPosition(&order->pos), 0);
        if (g_game->mapInfo->GetGameType() == 3) {
            FeatureDamagePacket packet;
            int n = c - g_game->heightMap;
            int w = g_game->mapWidthTiles;
            packet.type = 0x0f;
            packet.sub = 0xff;
            Point16 xz;
            xz.x = n % w;
            xz.y = n / w;
            packet.x = xz.x;
            packet.z = xz.y;
            BroadcastPacket(unit->player->id, &packet, 6);
        }
        order->target->buildLeft = 0;
        order->target->health = 1;
        MarkSelectionOrdersDirty(unit);
        break;
    }
    case 6: {
        QueueUnitSpeech(unit, 8, "Resurrection complete");
        MissionType kind = GetOrderType(8, unit, order->target, 0);
        if (kind.index)
            AppendOrder(unit, new Order(kind, order->target, 0, 0, 0, 0));
        return 5;
    }
    default:
        return 7;
    }
    return 1;
}
