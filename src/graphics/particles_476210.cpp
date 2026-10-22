// Decompiled by deepseek-v4.1-flash, finished by space-bunny-free, finished by deepseek-v4.1-flash, finished by Claude Opus 5.5. Names are provisional.
// FLAGS: /Gi
// std::vector<SmokeParticle>::insert(iterator, size_type, const _Ty&), stock
// MSVC 5 <vector> on a 32-byte record, emitted out of line through a member
// pointer.
#include <vector>

// The element of SmokeParticles' vector: SmokeParticle in smoke_particle.h,
// 0x20 bytes. This file only needs its size, and particles.cpp casts the vector
// to Class_00476210 to call this insert.
struct SmokeParticle {
    int dwords[8];                     // 0x20 bytes
};

typedef std::vector<SmokeParticle> Vec_00476210;
typedef void (Vec_00476210::*InsertFn_00476210)(
    Vec_00476210::iterator, Vec_00476210::size_type, const SmokeParticle&);

// The caller's growth step; the insert's bytes need the TU to instantiate
// reserve.
void __stdcall Grow_00476210(Vec_00476210* records, int periods)
{
    records->reserve(periods + records->size());
}

// FUNCTION: 0x476210 ?insert@?$vector@USmokeParticle@@V?$allocator@USmokeParticle@@@std@@@std@@QAEXPAUSmokeParticle@@IABU3@@Z
InsertFn_00476210 g_insert_00476210 = &Vec_00476210::insert;
