// Decompiled by deepseek-v4.1-flash, finished by space-bunny-free, finished by deepseek-v4.1-flash, finished by deepseek-v4.1-flash, finished by Space Bunny Free, finished by DeepSeek V4.1 Flash, finished by Space Bunny Free, finished by Claude Opus 5.5. Names are provisional.
// FLAGS: /Gi
// std::vector<WakeParticle>::insert(iterator, size_type, const _Ty&),
// stock MSVC 5 <vector>, emitted out of line through a member pointer. The
// caller (0x474880) grows the vector with reserve and fills it with insert.
#include <vector>

// The element of WakeParticles' vector: WakeParticle in particles.cpp, 0x44
// bytes. This file only needs its size.
struct WakeParticle {
    int data[0x11];
};

typedef std::vector<WakeParticle> Vec_00475ef0;
typedef void (Vec_00475ef0::*InsertFn_00475ef0)(
    Vec_00475ef0::iterator, Vec_00475ef0::size_type, WakeParticle const&);

// A use of operator= on this vector type, standing in for the original TU's
// own; the insert's bytes need the TU to instantiate it.
void __stdcall Assign_00475ef0(Vec_00475ef0* to, const Vec_00475ef0* from)
{
    *to = *from;
}

// FUNCTION: 0x475ef0 ?insert@?$vector@UWakeParticle@@V?$allocator@UWakeParticle@@@std@@@std@@QAEXPAUWakeParticle@@IABU3@@Z
InsertFn_00475ef0 g_insert_00475ef0 = &Vec_00475ef0::insert;
