// Decompiled by Opus. Names are provisional.
// Stays in its own file: HAPI_ClearShadowFlags (0x4be3b0, now in hpi.cpp)
// would be inlined into the recursive call below, but the original calls it
// out of line.

#include "archive_entry.h"

struct ArchiveDirectory {
    int count;
    ArchiveEntry* entries;          // +0x4
};

struct ArchiveDirectoryNode {
    char unknown_0[0x10];
    ArchiveDirectory* list;         // +0x10
};

struct ArchiveItem {
    char unknown_0[8];
    ArchiveDirectoryNode* node;            // +0x8
};

// The display context GetDisplay returns, the type graphics/display.cpp and
// util/hpi.cpp name DisplayContext: the open archives' items at +0x618 and
// their count at +0x61c. Its own view, not graphics/display_context.h: the
// items at +0x618 are ArchiveItem** here, void** in that header.
struct DisplayContext {
    char unknown_0[0x618];
    ArchiveItem** items;              // +0x618
    int itemCount;                    // +0x61c
};

extern char g_emptyStringLiteral[];

DisplayContext* GetDisplay(void);
void __stdcall HAPI_ClearShadowFlags(ArchiveDirectory* list);
void __stdcall HAPI_MarkShadowedFiles(char* name, int a, int b);

// FUNCTION: 0x4be320
void HAPI_ResolveShadowedFiles(void)
{
    DisplayContext* state = GetDisplay();
    if (state->itemCount > 0) {
        for (int i = 0; i < state->itemCount; i++) {
            ArchiveDirectory* list = state->items[i]->node->list;
            for (int j = list->count - 1; j >= 0; j--) {
                list->entries[j].flags &= ~2;
                if (list->entries[j].flags & 1)
                    HAPI_ClearShadowFlags(list->entries[j].dir);
            }
        }
        HAPI_MarkShadowedFiles(g_emptyStringLiteral, -1, 1);
    }
}
