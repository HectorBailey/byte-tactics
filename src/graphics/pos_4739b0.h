// Pos_004739b0: the integer halves of the 16.16 position a particle is drawn
// at, 12 bytes with the fraction halves left as padding. The one declaration of
// the struct, for particles.cpp, particles_472630.cpp and particles_473a00.cpp.
#ifndef POS_4739B0_H
#define POS_4739B0_H

struct Pos_004739b0 {
    char unknown_0[0x2];
    short x;                           // +0x2
    char unknown_4[0x2];
    short height;                      // +0x6
    char unknown_8[0x2];
    short y;                           // +0xa
};

#endif
