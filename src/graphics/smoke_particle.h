// SmokeParticle: one smoke puff (the element of SmokeParticles' vector), 0x20
// bytes. The one declaration of the record, for the files that step, expire or
// draw one; its 16.16 position comes with it, since the record holds it by
// value. The files 0x474b80 and 0x475470 keep their own view: their fog
// test needs a different 16.16 position (0x474b80) or an inlined draw helper
// (0x475470).
#ifndef SMOKE_PARTICLE_H
#define SMOKE_PARTICLE_H

#include "../util/vec3.h"

#include "../util/fixed.h"

// One smoke puff (the element of SmokeParticles' vector), 0x20 bytes.
struct SmokeParticle {
    void* data;                        // +0x00, the animation
    union {
        Vec3 pos;                      // +0x04
        FixedVec3 posw;
    };
    int limit;                         // +0x10, the rounds it lives
    int count;                         // +0x14, the rounds so far (the frame)
    int period;                        // +0x18
    int timer;                         // +0x1c

    void Step();
    void DrawParticle(void* dest, short px, short py);
    int IsExpired(int unused);
};

#endif
