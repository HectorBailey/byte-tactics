// Decompiled by Opus, Haiku, Sonnet, DeepSeek V4.1 Flash and Space Bunny Free. Names are provisional.
// Stays in its own file: merged with the module's second part its fog-culled
// loop walks from the wrong field (particles.cpp).
// SmokeParticles (vtable 0x4fd618, 0x38 bytes), derived from ParticleSystem
// (the family is listed in particles.cpp): smoke that drifts with the wind.
#include <windows.h>   // only for its symbol ids
#include <stddef.h>
#include <stdlib.h>
#include <vector>

#include "../util/vec3.h"

#include "../util/fixed.h"

// Kept, not map/byte_map.h: the header's helpers move Render (0x475470) to 96.0%.
struct MapSize {
    unsigned int width;                // +0x0
    unsigned int height;               // +0x4
    int Contains(unsigned int tx, unsigned int ty)
    {
        return tx < width && ty < height;
    }
};

struct ByteMap {
    unsigned char* data;               // +0x0
    MapSize size;                      // +0x4
    int Index(int x, int y) { return size.width * y + x; }
    // Get goes through Index: the extra register shapes both arms.
    unsigned char Get(int x, int y) { return data[Index(x, y)]; }
};

#pragma pack(push, 1)
// A player slot with its explored map spelled as a ByteMap: network/player.h
// has the same three members at +0x7c as separate fields, which cannot give
// this fog test the helpers it inlines.
struct FogPlayer {
    char unknown_0[0x7c];
    ByteMap explored;                  // +0x7c
    char unknown_88[0x14b - 0x88];
};
#pragma pack(pop)

#include "../game/game.h"

extern Game* g_game;

int __stdcall GetGafFrameCount(void* ptr);
void* __stdcall GetGafFrame(void* a, int b);
void __stdcall DrawFrameBlended(void* dest, void* src, int x, int y);

static inline int IsExplored(FogPlayer* map, FixedVec3* pos)
{
    int tx = pos->x.whole >> 5;
    int ty = (pos->z.whole - (pos->y.whole >> 1)) >> 5;
    if (map->explored.size.Contains(tx, ty) && map->explored.Get(tx, ty))
        return 1;
    return 0;
}

// The body of the matched 0x408090.
static inline int IsSeen(FogPlayer* map, FixedVec3* pos)
{
    int tx = pos->x.whole >> 5;
    int ty = (pos->z.whole - (pos->y.whole >> 1)) >> 5;
    if (!map->explored.size.Contains(tx, ty)) {
        return 0;
    }
    // Pointer local declared after the Contains test: the extra register
    // makes the width load materialise.
    ByteMap* b = &map->explored;
    return (g_game->visibilityMask[b->Index(tx, ty)] &
            (1 << g_game->playerIndex)) != 0;
}

static inline int IsVisible(FogPlayer* map, FixedVec3* pos)
{
    if ((g_game->mapFlags & 2) == 2)
        return IsExplored(map, pos);
    return IsSeen(map, pos);
}

// The 32-byte record; its unculled draw method is 0x475040.
// One particle, 0x20 bytes.
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
    void DrawParticle(int dest, short px, short py);
    int IsExpired(int unused);
    void DrawIfVisible(void* dest, short px, short py)
    {
        short sx = posw.x.whole - px + 0x80;
        short sy = posw.z.whole - (posw.y.whole >> 1) - py + 0x20;
        if (IsVisible((FogPlayer*)&g_game->players[g_game->playerIndex], &posw))
            DrawFrameBlended(dest, GetGafFrame(data, count), sx, sy);
    }
};

typedef std::vector<SmokeParticle> Vec_00474cd0;

#include "particle_system.h"

class SmokeParticles : public ParticleSystem {
public:
    int time;                                           // +0x8, the next emit tick
    Vec_00474cd0 records;                               // +0xc (_First +0x10)
    int emitPeriod;                                     // +0x1c, the emit period
    int holdPeriod;                                     // +0x20
    int maxFrame;                                       // +0x24, the frame count - 1
    int altAnimation;                                   // +0x28, the other animation
    Vec3 pos;                                           // +0x2c

    SmokeParticles();
    virtual void Update();                              // slot 1, 0x475340
    virtual void Render(int);                           // slot 2, 0x475470
    virtual int IsFinished();                           // slot 3, 0x474f80
    virtual void Emit();                                // slot 4, 0x474df0
    virtual int IsEmitDue();                            // slot 5, 0x475440
    virtual void Init(Vec3* pos, int limit, int a, int b, int c,
                              int alt);                 // slot 6, 0x474d50
};

// FUNCTION: 0x475470
void SmokeParticles::Render(int dest)
{
    for (std::vector<SmokeParticle>::iterator it = records.begin(); it != records.end();
         ++it) {
        it->DrawIfVisible((void*)dest, g_game->scroll_x, g_game->scroll_y);
    }
}
