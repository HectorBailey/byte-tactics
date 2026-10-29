// Projectile: one weapon in flight (Thaldren's Projectile, 0x6b bytes), the
// elements of the array at g_game+0x141f7 that the firing, aim, update and
// detonate code walks. The one declaration of the record; the pointer targets
// stay private to their own files.
// The position, trail, velocity and aim words are three ints each, spelled as
// plain members rather than vec3.h's Vec3: weapons/weapons.cpp and
// weapons/weapons_49b720.cpp take a fixed-point view of the same words (y is a
// union of the halves and the view inlines operator+=), so those two keep their
// own record with a reason line. A file that wants a Vec3 casts at the use, as
// the explosion calls already do.
#ifndef PROJECTILE_H
#define PROJECTILE_H

#pragma pack(push, 1)

struct WeaponDef;
struct Unit;

struct Projectile {
    WeaponDef* weapon;                 // +0x00, Thaldren's pWeapon
    int posX;                          // +0x04, Thaldren's nPosX
    int posY;                          // +0x08
    int posZ;                          // +0x0c
    int trailX;                        // +0x10, Thaldren's nTrailX
    int trailY;                        // +0x14
    int trailZ;                        // +0x18
    int velX;                          // +0x1c, Thaldren's nVelX
    int velY;                          // +0x20
    int velZ;                          // +0x24
    int aimX;                          // +0x28, Thaldren's nAimX
    int aimY;                          // +0x2c
    int aimZ;                          // +0x30
    short turnAccum;                   // +0x34, Thaldren's nTurnAccum
    unsigned short pitch;              // +0x36, Thaldren's wPitch
    unsigned short heading;            // +0x38, Thaldren's wHeading
    int speed;                         // +0x3a, Thaldren's nSpeed
    int distance;                      // +0x3e, Thaldren's nDistance
    unsigned int spawnTick;            // +0x42, Thaldren's dwSpawnTick
    unsigned int expireTick;           // +0x46, Thaldren's dwExpireTick
    unsigned int nextSmokeTick;        // +0x4a, Thaldren's dwNextSmokeTick
    Unit* targetUnit;                  // +0x4e, Thaldren's pTargetUnit
    Unit* ownerUnit;                   // +0x52, Thaldren's pOwnerUnit
    Projectile* interceptedProjectile; // +0x56
    short lastFeatureTileX;            // +0x5a
    short lastFeatureTileZ;            // +0x5c
    short groundHeightAvg;             // +0x5e
    unsigned short burstRemaining;     // +0x60, Thaldren's wBurstRemaining
    short weaponPiece;                 // +0x62, Thaldren's nWeaponPiece
    short propellerSpin;               // +0x64, Thaldren's nPropellerSpin
    unsigned char ownerPlayer;         // +0x66, Thaldren's bOwnerPlayer
    short compactIndex;                // +0x67, Thaldren's nCompactIndex
    unsigned short flags;              // +0x69, Thaldren's wFlags
};

#pragma pack(pop)

#endif
