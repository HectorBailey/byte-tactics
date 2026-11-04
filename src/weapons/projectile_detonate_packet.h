// ProjectileDetonatePacket: the 0xe byte projectile detonate packet (type
// 0xe) the network code sends and applies. The one declaration of the record:
// the sender is 0x49a120 in weapons/weapons_49a120.cpp and the receiver
// 0x49af90 in weapons/weapons.cpp. The includer must have util/vec3.h's Vec3
// already.
#ifndef PROJECTILE_DETONATE_PACKET_H
#define PROJECTILE_DETONATE_PACKET_H

#pragma pack(push, 1)

struct ProjectileDetonatePacket {
    unsigned char type;                // +0x0
    Vec3 aim;                          // +0x1
    unsigned char weaponIndex;         // +0xd
};

#pragma pack(pop)

#endif
