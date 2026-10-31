// Fixed: a 16.16 fixed-point value, the game's coordinate format (Vec3's x, y
// and z, the unit's position at +0x6a), read whole or as its two halves; and
// FixedVec3, a position of three of them, laid out as util/vec3.h's Vec3. The
// one declaration of each for the files that read a coordinate's halves; a
// file that only needs the whole half of a Vec3 can read it as
// ((short*)&pos.x)[1] instead.
#ifndef FIXED_H
#define FIXED_H

union Fixed {
    int value;
    struct {
        unsigned short frac;           // the low half
        short whole;                   // the high half, the whole part
    };
};

struct FixedVec3 {
    Fixed x;
    Fixed y;
    Fixed z;
};

#endif
