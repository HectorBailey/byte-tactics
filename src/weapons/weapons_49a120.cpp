// Decompiled by Claude Opus 5.5. Names are provisional.
#include <math.h>
#include <string.h>
#include "../util/vec3.h"
#include "../ai/squad_manager.h"
#include "../network/player.h"

// A 16.16 fixed-point value and its whole part.
union Fixed {
    int raw;
    struct {
        unsigned short frac;
        short whole;
    } part;
};

struct FixedVec3 {
    Fixed x;
    Fixed y;
    Fixed z;
};

#pragma pack(push, 1)

#include "weapon_def.h"
#include "../map/feature.h"

// Not units/unit_def.h: the header takes 0x49a120 out of its symbol-id window.
struct UnitDef_0049a120 {
    char unknown_0[0x15e];
    Vec3 boxMin;                       // +0x15e
    Vec3 boxMax;                       // +0x16a
};

struct Unit_0049a120 {
    char unknown_0[0x6a];
    Vec3 pos;                          // +0x6a
    char unknown_76[0x92 - 0x76];
    UnitDef_0049a120* def;             // +0x92
    Player* player;                    // +0x96
    char unknown_9a[0xff - 0x9a];
    unsigned char playerIndex;         // +0xff
    char unknown_100[0x118 - 0x100];
};

// Thaldren's Projectile (0x6b bytes): one weapon in flight. Its owner is this
// file's unit view, so the uses cast at the owner.
#include "projectile.h"

struct Cell {
    unsigned short unit;               // +0x0
    unsigned short unit2;              // +0x2
    char unknown_4[4];
    unsigned short feature;            // +0x8
    union {
        unsigned short spot;           // +0xa
        struct {
            unsigned char offsetZ;     // +0xa
            unsigned char offsetX;     // +0xb
        } origin;
    };
    unsigned char flags;               // +0xc
};

// Kept as a view: the declarations ingame/feature_spot.h brings move the spot
// address arithmetic in 0x49a120 (docs/c2-regalloc.md).
struct Spot_0049a120 {
    char unknown_0[8];
    Vec3 pos;                          // +0x8
    char unknown_14[0x30 - 0x14];
};

// One view of the 0xe byte projectile detonate packet (type 0xe): the
// receiver at 0x49af90 in the weapons file holds the other.
struct ProjectileDetonatePacket {
    unsigned char type;                // +0x0
    Vec3 aim;                          // +0x1
    unsigned char weaponIndex;         // +0xd
};

struct Game_0049a120 {
    char unknown_0[0x141f3];
    int projectileCount;               // +0x141f3
    Projectile* projectiles;           // +0x141f7
    char unknown_141fb[0x1420b - 0x141fb];
    Spot_0049a120* spots;              // +0x1420b
    char unknown_1420f[0x14233 - 0x1420f];
    int mapWidthTiles;                 // +0x14233
    int mapHeightTiles;                // +0x14237
    char unknown_1423b[0x1426f - 0x1423b];
    Feature* features;                 // +0x1426f
    char unknown_14273[0x14287 - 0x14273];
    Cell* heightMap;                   // +0x14287
    char unknown_1428b[0x14357 - 0x1428b];
    Unit_0049a120* units;              // +0x14357
};
#pragma pack(pop)

extern Game_0049a120* g_game;

// The system headers come after the game's own declarations, and only these
// (lean windows.h, vector): the symbol ids they give cell, feature and g_game
// decide the spot and feature address arithmetic.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>

// What one explosion has already hit, so nothing takes damage twice.
struct Hits_0049a120 {
    int numUnits;
    int numFeatures;
    Unit_0049a120* units[20];
    Cell* features[64];

    // Each returns 0 if the thing was hit already, else records it (while
    // there is room) and returns 1.
    int AddUnit(Unit_0049a120* unit)
    {
        for (int k = 0; k < numUnits; k++) {
            if (units[k] == unit)
                return 0;
        }
        if (numUnits < 20) {
            units[numUnits] = unit;
            numUnits++;
        }
        return 1;
    }
    int AddFeature(Cell* cell)
    {
        for (int k = 0; k < numFeatures; k++) {
            if (features[k] == cell)
                return 0;
        }
        if (numFeatures < 64) {
            features[numFeatures] = cell;
            numFeatures++;
        }
        return 1;
    }
};

Cell* __stdcall GetMapCell(int x, int y);
int __stdcall ApplyWeaponDamage(Projectile* weapon, Unit_0049a120* target, float scale);
void __stdcall DetonateProjectile(Projectile* weapon, Unit_0049a120* unit);
int __stdcall VectorLength(Vec3* v);
Vec3 __stdcall GetFootprintCentre(Point16* cell, Feature* def);
void __stdcall DamageFeature(Cell* cell, int x, int z, WeaponDef* def);
int __stdcall BroadcastPacket(int id, void* data, int size);

static inline int Length(Vec3* v)
{
    double x = v->x;
    double y = v->y;
    double z = v->z;
    return (int)sqrt(x * x + y * y + z * z);
}

static inline Unit_0049a120* UnitFromId(unsigned short id)
{
    if (id == 0)
        return 0;
    return &g_game->units[id];
}

static inline Vec3 Sub(const Vec3& a, const Vec3& b)
{
    Vec3 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

// Splash damage: every unit and feature within the weapon's radius of `pos`
// takes damage that falls off from the centre, and weapons in flight close
// enough are detonated too.
// Stays in its own file: it is a gap region (one of data/functions.csv's 29
// gap rows), and a file holding both a gap function and game functions is
// classified gap by tools/sources.py, so the game build would leave the
// other functions out.
// FUNCTION: 0x49a120
void __stdcall ApplyAreaDamage(Projectile* weapon, Vec3* pos)
{
    Hits_0049a120 hits;
    memset(&hits, 0, 8);
    int radius = weapon->weapon->areaOfEffect >> 1;
    int r = radius / 16 + 1;
    int cx = ((FixedVec3*)pos)->x.part.whole;
    int x0 = cx / 16 - r;
    int x1 = cx / 16 + r;
    int cz = ((FixedVec3*)pos)->z.part.whole;
    int z0 = cz / 16 - r;
    int z1 = cz / 16 + r;
    if (x0 < 0)
        x0 = 0;
    if (x1 > g_game->mapWidthTiles)
        x1 = g_game->mapWidthTiles;
    if (z0 < 0)
        z0 = 0;
    if (z1 > g_game->mapHeightTiles)
        z1 = g_game->mapHeightTiles;
    int enemyDamage = 0;
    int friendlyDamage = 0;

    for (int z = z0; z < z1; z++) {
        Cell* cell = GetMapCell(x0, z);
        for (int x = x0; x < x1; x++, cell++) {
            if (!cell)
                continue;
            for (int i = 0; i <= 1; i++) {
                Unit_0049a120* unit = i ? UnitFromId(cell->unit2) : UnitFromId(cell->unit);
                if (!unit || unit == (Unit_0049a120*)weapon->ownerUnit)
                    continue;
                if (!hits.AddUnit(unit))
                    continue;
                UnitDef_0049a120* def = unit->def;
                Vec3 d;
                {
                    // The upper-bound sum is written in the test and again in the
                    // block local `hi`, used once: the two sums become one temporary.
                    int p = pos->x;
                    if (p < unit->pos.x + def->boxMin.x) {
                        int lo = unit->pos.x + def->boxMin.x;
                        d.x = lo - p;
                    } else if (p > unit->pos.x + def->boxMax.x) {
                        int hi = unit->pos.x + def->boxMax.x;
                        d.x = p - hi;
                    } else {
                        d.x = 0;
                    }
                }
                {
                    int p = pos->y;
                    if (p < unit->pos.y + def->boxMin.y) {
                        int lo = unit->pos.y + def->boxMin.y;
                        d.y = lo - p;
                    } else if (p > unit->pos.y + def->boxMax.y) {
                        int hi = unit->pos.y + def->boxMax.y;
                        d.y = p - hi;
                    } else {
                        d.y = 0;
                    }
                }
                {
                    int p = pos->z;
                    if (p < unit->pos.z + def->boxMin.z) {
                        int lo = unit->pos.z + def->boxMin.z;
                        d.z = lo - p;
                    } else if (p > unit->pos.z + def->boxMax.z) {
                        int hi = unit->pos.z + def->boxMax.z;
                        d.z = p - hi;
                    } else {
                        d.z = 0;
                    }
                }
                Fixed dist;
                dist.raw = Length(&d);
                int distance = dist.part.whole;
                if (distance < radius) {
                    float edge = weapon->weapon->edgeDamage;
                    float scale;
                    if (distance) {
                        float t = (float)distance / radius - 1.0f;
                        // Square t in place and assign the whole falloff back to t
                        // before `scale = t`: sets the x87 stack lifetimes.
                        t *= t;
                        t = (1.0f - edge) * t + edge;
                        scale = t;
                    } else {
                        scale = 1.0f;
                    }
                    int damage = ApplyWeaponDamage(weapon, unit, scale);
                    if (weapon->ownerPlayer == unit->playerIndex)
                        friendlyDamage += damage;
                    else
                        enemyDamage += damage;
                }
            }
            // A mask test (bit 14, unitsOnly): the bit-field form changes the code.
            if (weapon->weapon->flags.value & 0x4000)
                continue;
            Cell* origin = cell;
            int fx = x;
            int fz = z;
            if (cell->feature == 0xfffe) {
                fx -= cell->origin.offsetX;
                fz -= cell->origin.offsetZ;
                origin = GetMapCell(fx, fz);
            }
            unsigned short feature = origin->feature;
            if (feature >= 0xfffb)
                continue;
            int index = cell - g_game->heightMap;
            Point16 cp;
            cp.x = index % g_game->mapWidthTiles;
            cp.y = index / g_game->mapWidthTiles;
            Point16 at = cp;
            Spot_0049a120* spot = &g_game->spots[cell->spot];
            int distance;
            if ((cell->flags & 1) && spot) {
                Vec3 v = Sub(*pos, spot->pos);
                Fixed dist;
                dist.raw = VectorLength(&v);
                distance = dist.part.whole;
            } else {
                Vec3 p = GetFootprintCentre(&at, &g_game->features[feature]);
                Vec3 v = Sub(*pos, p);
                Fixed dist;
                dist.raw = (int)sqrt((double)v.x * v.x + (double)v.y * v.y + (double)v.z * v.z);
                distance = dist.part.whole;
            }
            if (distance >= radius)
                continue;
            if (!hits.AddFeature(origin))
                continue;
            DamageFeature(origin, fx, fz, weapon->weapon);
        }
    }

    if (weapon->weapon->flags.b.interceptor) {
        // Every field is read through `other` (no projectiles[j]): the derived
        // pointer is stepped after it.
        Projectile* other = g_game->projectiles;
        for (int j = 0; j < g_game->projectileCount; j++, other++) {
            if ((other->flags & 2) || other == weapon)
                continue;
            Vec3* p = (Vec3*)&other->posX;
            int dx = weapon->posX - p->x;
            int dy = weapon->posY - p->y;
            int dz = weapon->posZ - p->z;
            int reach = weapon->weapon->areaOfEffect;
            if ((int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dy * dy) >> 32) + (int)(((__int64)dz * dz) >> 32)
                < reach * reach) {
                DetonateProjectile(other, 0);
                ProjectileDetonatePacket packet;
                packet.type = 0xe;
                packet.aim = *(Vec3*)&other->aimX;
                packet.weaponIndex = other->weapon->index;
                BroadcastPacket(((Unit_0049a120*)weapon->ownerUnit)->player->id, &packet, sizeof(packet));
                packet.type = 0xe;
                packet.aim = *(Vec3*)&weapon->aimX;
                packet.weaponIndex = weapon->weapon->index;
                BroadcastPacket(((Unit_0049a120*)weapon->ownerUnit)->player->id, &packet, sizeof(packet));
            }
        }
    }

    if (weapon->ownerUnit)
        // The damage split's Obj_00406f50 parameter is this weapon object.
        ((Unit_0049a120*)weapon->ownerUnit)->player->ai->MarkOwnerNetDirtyFromDamageSplit((Obj_00406f50*)weapon, enemyDamage, friendlyDamage);
}
