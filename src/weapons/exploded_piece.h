// ExplodedPiece: the 0x30-byte record a unit's script builds and hands to
// StartExplodePiece, which copies it to the front of a pool slot. The one
// declaration of the record, for explosions.cpp, which owns the pool and the
// piece it is copied from, and unit_script.cpp, whose ExplodePiece builds one
// on the stack.
#ifndef EXPLODED_PIECE_H
#define EXPLODED_PIECE_H

struct Unit;
struct PieceRec;

#pragma pack(push, 1)
struct ExplodedPiece {
    Unit* unit;                        // +0x00
    int index;                         // +0x04
    int angVelZ;                       // +0x08
    int angVelX;                       // +0x0c
    int angVelY;                       // +0x10
    int vel_x;                         // +0x14
    int vel_y;                         // +0x18
    int vel_z;                         // +0x1c
    int ticks;                         // +0x20
    int scale;                         // +0x24
    // Two forms of the flag word, because its two writers need them: the
    // script writes the whole word (a 1-bit field would truncate
    // `(b & 2) << 4`) and the pool reads single bits.
    union {
        unsigned int bits;             // +0x28
        struct {
            unsigned int b0 : 1;
            unsigned int b1 : 1;
            unsigned int b2 : 1;
            unsigned int b3 : 1;
            unsigned int b4 : 1;
            unsigned int b5 : 1;
            unsigned int rest : 26;
        };
    };
    PieceRec* rec;                     // +0x2c
};
#pragma pack(pop)

#endif
