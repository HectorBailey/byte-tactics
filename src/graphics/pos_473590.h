// Pos_00473590: the integer halves of the 16.16 position a particle is drawn
// at, 12 bytes. The one declaration of the struct, for particles.cpp and
// particles_472630.cpp. Every member is a short with its padding spelled out, so
// the layout is the same inside and outside the pack(1) block one file keeps it
// in.
#ifndef POS_473590_H
#define POS_473590_H

struct Pos_00473590 {
    short x;                       // +0x00 (record +0x06)
    char unknown_2[0x4 - 0x2];
    short h;                       // +0x04 (record +0x0a)
    char unknown_6[0x8 - 0x6];
    short y;                       // +0x08 (record +0x0e)
};

#endif
