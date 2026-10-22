// Decompiled by deepseek-v4.1-flash, finished by space-bunny-free, edited by deepseek-v4.1, finished by deepseek-v4.1-flash, finished by deepseek-v4.1-flash, finished by Space Bunny Free, finished by DeepSeek V4.1 Flash, finished by Space Bunny Free, finished by Claude Opus 5.5. Names are provisional.
// FLAGS: /Gi
// std::vector<TimedSubParticle>::insert(iterator, size_type, const _Ty&), stock
// MSVC 5 <vector> on a 32-byte record, emitted out of line through a member
// pointer. The caller (0x4751c0) grows the vector with reserve and fills it
// with insert.
//
// This function and 0x476210 are the same insert on two identically laid out
// records.
#include <vector>

// The element of TimedSubParticles' vector: TimedSubParticle in
// particles_475700.cpp, 0x20 bytes. This file only needs its size, and
// TimedSubParticles::Emit (0x4751c0) casts its records to Vec_00476490 to call
// this insert.
struct TimedSubParticle { int dwords[8]; };

typedef std::vector<TimedSubParticle> Vec_00476490;
typedef void (Vec_00476490::*InsertFn_00476490)(
    Vec_00476490::iterator, Vec_00476490::size_type, const TimedSubParticle&);

// A use of operator= on this vector type, standing in for the original TU's
// own; the insert's bytes need the TU to instantiate it.
void __stdcall Assign_00476490(Vec_00476490* to, const Vec_00476490* from)
{
    *to = *from;
}

// FUNCTION: 0x476490 ?insert@?$vector@UTimedSubParticle@@V?$allocator@UTimedSubParticle@@@std@@@std@@QAEXPAUTimedSubParticle@@IABU3@@Z
InsertFn_00476490 g_insert_00476490 = &Vec_00476490::insert;
