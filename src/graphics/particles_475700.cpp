// Decompiled by Opus, Haiku and Sonnet. Names are provisional.
// Stays in its own file: merged with the module's second part the inlined draw
// rotates its temporaries (particles.cpp).
// TimedSubParticles (vtable 0x4fd638, 0x34 bytes), derived from ParticleSystem
// (the family is listed in particles.cpp): the same shape as SmokeParticles,
// without the fog culling.
// Needed: without <windows.h> slot 2 subtracts the scroll y before the half height.
#include <windows.h>
#include <stddef.h>
#include <stdlib.h>
#include <vector>

// g_game before the shared header: its symbol id decides which register holds
// the draw arguments and their order in Render.
struct Game;
extern Game* g_game;

// The TimedSubParticles class.
#include "timed_sub_particles.h"

// A 16.16 fixed-point value: the whole part in the high half.
struct Fixed {
    unsigned short frac;
    short whole;
};

struct FixedVec3 {
    Fixed x;
    Fixed y;
    Fixed z;
};

#include "../game/game.h"

// Declared returning int: the original uses the full eax without masking it.
int __stdcall GetGafFrameCount(void* ptr);
void* __stdcall GetGafFrame(void* a, int b);
void __stdcall DrawFrameBlended(void* dest, void* src, int x, int y);

// One particle, 0x20 bytes.
struct TimedSubParticle {
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
    void DrawParticle(int dest, short px, short py);
    int IsExpired(int unused);
    void Draw(void* dest, short px, short py)
    {
        short sy = posw.z.whole - (posw.y.whole >> 1) - py + 0x20;
        short sx = posw.x.whole - px + 0x80;
        DrawFrameBlended(dest, GetGafFrame(data, count), sx, sy);
    }
};

#include "particle_system.h"

// FUNCTION: 0x475700
void TimedSubParticles::Render(int dest)
{
    for (std::vector<TimedSubParticle>::iterator it = records.begin(); it != records.end(); ++it) {
        it->Draw((void*)dest, g_game->scroll_x, g_game->scroll_y);
    }
}
