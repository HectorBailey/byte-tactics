// Decompiled by deepseek-v4.1-flash, finished by GPT-6.1-sol, finished by GPT-6, finished by deepseek-v4.1-flash, finished by deepseek-v4.1-flash, finished by deepseek-v4.1-flash, finished by claude-opus-5-5. Names are provisional.
//
// The projectile update loop. One if/else-if chain over the type's flag
// bits (selfProp guided, lineOfSight straight, ballistic timed drift, dropped
// drift, meteor tumbling), with
// a burst-fire branch for projectiles whose counter is not 0.
// Remove() is the inlined "deselect and mark dead" block that 0x49b090 also
// writes out.

// Needed: without <stdio.h> the b0 arm loads g_game first and gets a 5-byte mov.
#include <stdio.h>

#pragma pack(push, 1)

// Kept: y is a union of the fixed-point parts and the view has operator+=.
struct Vec3_0049b720 {
    int x;
    union {
        int y;
        struct {
            unsigned short lo;
            short hi;
        } yw;
    };
    int z;
    Vec3_0049b720& operator+=(const Vec3_0049b720& o)
    {
        x += o.x;
        y += o.y;
        z += o.z;
        return *this;
    }
};

#include "weapon_def.h"
#include "unit_weapon_slot.h"

#include "../units/unit.h"

// Thaldren's wFlags bits: the projectile's flags word at +0x69, one field per
// bit the update loop tests.
struct ProjectileFlags {
    unsigned short b0 : 1;
    unsigned short dead : 1;
    unsigned short b2 : 2;
    unsigned short state : 2;
    unsigned short rest : 10;
};

// Thaldren's Projectile (0x6b bytes): the record the update loop below walks.
// Kept local: its position, start and velocity are the fixed-point Vec3 below
// (y is a union of the halves and the view has operator+=), which the header's
// plain ints cannot spell without moving the loop's arithmetic.
struct Projectile {
    WeaponDef* type;                   // +0x0
    Vec3_0049b720 pos;                 // +0x4
    Vec3_0049b720 start;               // +0x10
    Vec3_0049b720 vel;                 // +0x1c
    char unknown_28[0x34 - 0x28];
    short roll;                        // +0x34
    short heading;                     // +0x36
    short pitch;                       // +0x38
    unsigned int speed;                // +0x3a
    unsigned int distance;             // +0x3e, Thaldren's nDistance
    unsigned int spawnTick;            // +0x42, Thaldren's dwSpawnTick
    unsigned int expireTick;           // +0x46, Thaldren's dwExpireTick
    unsigned int nextSmokeTick;        // +0x4a, Thaldren's dwNextSmokeTick
    int f4e;                           // +0x4e
    Unit* ownerUnit;                   // +0x52, Thaldren's pOwnerUnit
    void* f56;                         // +0x56
    char unknown_5a[0x60 - 0x5a];
    unsigned short burstRemaining;     // +0x60, Thaldren's wBurstRemaining
    unsigned short weaponPiece;        // +0x62, Thaldren's nWeaponPiece
    short propellerSpin;               // +0x64, Thaldren's nPropellerSpin
    char unknown_66[0x69 - 0x66];
    ProjectileFlags flags;             // +0x69, Thaldren's wFlags
};

struct Net_0049b720 {
    char unknown_0[0xd48];
    int noSeaLevelTrigger;
};

struct Game {
    char unknown_0[0x141f3];
    int projectileCount;               // +0x141f3
    Projectile* projectiles;           // +0x141f7
    char unknown_141fb[0x14263 - 0x141fb];
    int gravity;                       // +0x14263
    char unknown_14267[0x1427f - 0x14267];
    unsigned char seaLevel;            // +0x1427f
    char unknown_14280[0x142f7 - 0x14280];
    Projectile* cameraFollowTrackObj;     // +0x142f7
    char unknown_142fb[0x1433f - 0x142fb];
    Vec3_0049b720 cameraSnapPos;       // +0x1433f
    unsigned short cameraSnapTicks;    // +0x1434b
    char unknown_1434d[0x37ecc - 0x1434d];
    Vec3_0049b720 wind;                // +0x37ecc
    char unknown_37ed8[0x38a47 - 0x37ed8];
    unsigned int gameTick;             // +0x38a47
    char unknown_38a4b[0x391e9 - 0x38a4b];
    Net_0049b720* mapInfo;             // +0x391e9
};

#include "../map/cell.h"

#pragma pack(pop)

// Unused here: these headers' symbol ids keep 0x49b720 matching (docs/c2-regalloc.md).
#include "../map/mission.h"
#include "../sound/sound.h"

extern Game* g_game;

void __stdcall CompactProjectiles();
void __stdcall CheckProjectileCollision(WeaponDef* type, Projectile* p);
Vec3_0049b720* __stdcall GetProjectileAimPoint(Projectile* p);
int __stdcall TurnUnitTowardsPoint(Projectile* p, Vec3_0049b720* target);
void __stdcall DetonateProjectile(Projectile* p, void* unit);
void __stdcall GetWeaponPiecePosition(Unit* unit, Vec3_0049b720* out, unsigned char weapon, int piece);
int __stdcall PlaySoundAt(int sound, Vec3_0049b720* pos, int flag);
void __stdcall EmitWhiteSmoke(Vec3_0049b720* pos, short kind);
Cell* __stdcall GetMapCellAtPosition(Vec3_0049b720* pos);
void __stdcall AddExplosionEffect(Vec3_0049b720* pos, void* src, int index, int flag);
int __stdcall RandomInt(int range);
int __cdecl FUN_004b70ef(short angle, int scale);
int __cdecl FUN_004b7123(short angle, int scale);

// A copy of AllocProjectile (matched in its own file), the function just before
// this one in the original source; defined here so that /Ob2 inlines it.
Projectile* AllocProjectile()
{
    Projectile* p = 0;
    if (g_game->projectileCount < 300) {
        p = &g_game->projectiles[g_game->projectileCount++];
        p->flags.dead = 0;
        p->f4e = 0;
    }
    return p;
}

// Gravity goes through this pointer store, not `p->vel.y -= ...`: it makes
// p->type get reloaded, so `type` is not propagated.
static inline void Fall(Vec3_0049b720* v)
{
    v->y -= g_game->gravity;
}

static inline void Remove(Projectile* p)
{
    if (p == g_game->cameraFollowTrackObj) {
        g_game->cameraSnapPos = g_game->cameraFollowTrackObj->pos;
        g_game->cameraSnapTicks = p->type->holdTime;
        g_game->cameraFollowTrackObj = 0;
    }
    p->flags.dead = 1;
}

// Stays in its own file: the b0 arm's register tie follows this file's symbol
// ids, which the joined weapons.cpp moves.
// FUNCTION: 0x49b720
void UpdateProjectiles()
{
    int n = g_game->projectileCount;
    for (int i = 0; i < n; i++) {
        Projectile* p = &g_game->projectiles[i];
        // type is read before oldY: orders the two spill stores.
        WeaponDef* type = p->type;
        int oldY = p->pos.yw.hi;

        if (p->burstRemaining != 0) {
            if (g_game->gameTick >= p->spawnTick + type->burstRate) {
                if (type->burstRate >= 5 || (p->burstRemaining & 1)) {
                    unsigned char w;
                    for (w = 0; w < 3; w++) {
                        if (p->ownerUnit->weapons[w].weapon == type)
                            break;
                    }
                    GetWeaponPiecePosition(p->ownerUnit, &p->pos, w, p->weaponPiece);
                }
                p->burstRemaining--;
                p->spawnTick += type->burstRate;
                Projectile* q = AllocProjectile();
                if (q) {
                    *q = *p;
                    q->spawnTick = g_game->gameTick;
                    if (type->flags.b.soundTrigger)
                        PlaySoundAt(type->sound, &p->pos, 0);
                    if (type->lifetime != 0)
                        q->expireTick = g_game->gameTick + type->lifetime;
                    else
                        q->expireTick = (p->distance + 0x100000) / p->speed + g_game->gameTick;
                    if (type->lifeRand != 0)
                        q->expireTick += RandomInt(type->lifeRand) - (type->lifeRand >> 1);
                    q->burstRemaining = 0;
                    if (type->spread != 0) {
                        short ang = RandomInt(type->spread) + (short)(p->heading - (type->spread >> 1));
                        int t = FUN_004b7123(p->pitch, type->speed);
                        p->vel.x = -FUN_004b70ef(ang, t);
                        p->vel.z = -FUN_004b7123(ang, t);
                    }
                }
                if (p->burstRemaining == 0)
                    Remove(p);
            }
            continue;
        }

        if (type->flags.b.propeller)
            p->propellerSpin += 0x400;

        if (type->flags.b.selfProp) {
            if (p->expireTick > g_game->gameTick) {
                if (type->flags.b.waterWeapon && p->pos.yw.hi >= g_game->seaLevel) {
                    Fall(&p->vel);
                    p->pitch = 0;
                } else {
                    int seek = 0;
                    if (p->speed < type->speed) {
                        p->speed += type->acceleration;
                        if (p->speed > type->speed)
                            p->speed = type->speed;
                    }
                    if (type->flags.b.twoPhase) {
                        if (p->flags.state > 0)
                            seek = 1;
                    } else if (type->flags.b.guidance) {
                        seek = 1;
                    }
                    if (seek) {
                        Vec3_0049b720* target = GetProjectileAimPoint(p);
                        if (TurnUnitTowardsPoint(p, target) == 0)
                            DetonateProjectile(p, 0);
                    }
                    p->vel.y = FUN_004b70ef(p->pitch, p->speed);
                    int t = FUN_004b7123(p->pitch, p->speed);
                    p->vel.x = -FUN_004b70ef(p->heading, t);
                    p->vel.z = -FUN_004b7123(p->heading, t);
                }
            } else if (type->flags.b.burnBlow) {
                DetonateProjectile(p, 0);
            } else {
                Fall(&p->vel);
                if (type->flags.b.twoPhase) {
                    if (p->flags.state == 0) {
                        p->expireTick = g_game->gameTick + p->type->flightTime;
                        p->flags.state++;
                        if (!p->type->flags.b.tracks) {
                            p->f56 = 0;
                            p->f4e = 0;
                        }
                    }
                }
            }
            p->pos += p->vel;
            CheckProjectileCollision(type, p);
        } else if (type->flags.b.lineOfSight) {
            if (p->expireTick > g_game->gameTick) {
                p->pos += p->vel;
                if (type->flags.b.beamWeapon) {
                    if (p->flags.b0)
                        p->start += p->vel;
                    else if (g_game->gameTick > p->spawnTick + type->duration)
                        p->flags.b0 = 1;
                }
                CheckProjectileCollision(type, p);
            } else {
                Remove(p);
            }
        } else if (type->flags.b.ballistic) {
            if (type->lifetime != 0) {
                if (p->expireTick > g_game->gameTick) {
                    p->pos += p->vel;
                    p->pos += g_game->wind;
                    p->vel.y -= g_game->gravity;
                    CheckProjectileCollision(type, p);
                } else if (type->flags.b.burnBlow) {
                    DetonateProjectile(p, 0);
                } else {
                    EmitWhiteSmoke(&p->pos, 9);
                    Remove(p);
                }
            } else {
                p->pos += p->vel;
                p->pos += g_game->wind;
                p->vel.y -= g_game->gravity;
                CheckProjectileCollision(type, p);
            }
        } else if (type->flags.b.dropped) {
            p->pos += p->vel;
            p->pos += g_game->wind;
            p->vel.y -= g_game->gravity;
            CheckProjectileCollision(type, p);
        } else if (type->flags.b.meteor) {
            p->pos += p->vel;
            p->roll += ((short*)&p->vel.x)[1] << 8;
            p->pitch += ((short*)&p->vel.z)[1] << 8;
            CheckProjectileCollision(type, p);
        }

        if (!p->flags.dead) {
            if (type->flags.b.smokeTrail && p->expireTick > g_game->gameTick && p->nextSmokeTick < g_game->gameTick) {
                EmitWhiteSmoke(&p->pos, 9);
                p->nextSmokeTick += type->smokeRate;
            }
            if (oldY > g_game->seaLevel && p->pos.yw.hi <= g_game->seaLevel) {
                Cell* cell = GetMapCellAtPosition(&p->pos);
                if (cell && cell->high < g_game->seaLevel && g_game->mapInfo->noSeaLevelTrigger == 0)
                    AddExplosionEffect(&p->pos, type->splash, 0, 1);
            }
        }
    }
    CompactProjectiles();
}
