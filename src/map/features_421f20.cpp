// Decompiled by DeepSeek V4.1 Flash. Names are provisional.
// Stays in its own file: in features.cpp the loop address's base and index
// swap (lea [edx+ebx+4] against the original's [ebx+edx+4]; docs/c2-regalloc.md,
// "Symbol ids").
#include <string.h>
#include "feature_records.h"

#pragma pack(push, 1)
struct Game {
    char unknown_0[0x141fb];
    AnimManager anim;         // +0x141fb
    char unknown_14273[0x1439b - 0x14273];
    int unitDefs;                      // +0x1439b
};
#pragma pack(pop)

extern Game* g_game;

void* __cdecl GameAllocIgnoreTag(char* name, unsigned int size);
unsigned short __stdcall LoadFeatureType(char* name);

// FUNCTION: 0x421f20
void __stdcall InitFeatureAnimPool(List* list)
{
    AnimManager* m = &g_game->anim;

    m->anim = (AnimEntry*)GameAllocIgnoreTag("FEATURE ANIM DATA", 0x18000);
    memset(m->anim, 0, 0x18000);
    m->activeHead = -1;
    m->idleHead = -1;
    m->freeHead = 0;
    for (int i = 0; i < 0x800; i++) {
        m->anim[i].next = i + 1;
        m->anim[i].prev = i - 1;
    }
    m->anim[0].prev = -1;
    m->anim[0x7ff].next = -1;

    m->featureUnit = (FeatureUnit*)GameAllocIgnoreTag("Feature Unit", 0x118);
    memset(m->featureUnit, 0, 0x118);
    m->featureUnit->flags_110 |= 0x200;
    m->featureUnit->flags_110 |= 0x20000000;
    m->featureUnit->unitDefIndex = 0;
    m->featureUnit->flags_114 |= 1;
    m->featureUnit->def = g_game->unitDefs;
    m->names = 0;
    m->nameCount = 0;

    // A reference to the field, not a local copy: every use re-reads
    // list->entries, and that is what makes MSVC base the address on the loop
    // offset (lea [ebx+edx+4]) instead of on the loaded pointer.
    NameEntry*& entries = list->entries;

    for (int j = 0; j < list->count; j++) {
        LoadFeatureType(entries[j].name);
    }
}
