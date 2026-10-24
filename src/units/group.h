// Group: one of a player's ten squads, the 0x20-byte elements of Player::groups
// (player.h). The one declaration of the record, for the two ai_player files
// that order squads and for squads.cpp, which builds and frees them.
#ifndef GROUP_H
#define GROUP_H

#include <vector>

struct Player;
struct Unit;
struct Vec3;

#pragma pack(push, 1)
struct Group {
    Player* player;                    // +0x0
    int id;                            // +0x4
    int field_8;                       // +0x8
    int field_c;                       // +0xc
    std::vector<Unit*> units;          // +0x10

    Group(Player* a, int b)
        : player(a), id(b), field_8(0), field_c(0)
    {
    }

    void Send(unsigned char mode, int remove, Unit* target, Vec3* pos, int flags, int extra);
};
#pragma pack(pop)

#endif
