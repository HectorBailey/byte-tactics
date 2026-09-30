// Decompiled by space-bunny-free, finished by deepseek-v4.1-flash and GPT-6.1-sol, edited by deepseek-v4.1. Names are provisional.
// MATCH (402/402 bytes) by deepseek-v4.1. The last 2 bytes came from moving
// `dst++` out of the loop body into the for-increment clause
// (`j++, mapX += halfWidth, src++, dst++`), which makes MSVC5 emit
// `mov [edx],cl / ... / inc edx / mov [esp+0x14],edx`; as a body statement it
// instead sank a `mov eax,edx / inc eax` copy of the store pointer.

#pragma pack(push, 1)
struct Fx_00466c20 {
    char unknown_0[0xcc];
    unsigned char* colorMap;         // +0xcc
};

struct Team_00466c20 {
    char unknown_0[0x7c];
    unsigned char* seenMap;          // +0x7c
    char unknown_80[0x14b - 0x80];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Game {
    char unknown_0[0xc];
    Fx_00466c20* fx;                 // +0xc
    char unknown_10[0xdcb - 0x10];
    unsigned char fogColor;          // +0xdcb
    char unknown_dcc[0x1b63 - 0xdcc];
    Team_00466c20 players[1];        // +0x1b63, 0x14b bytes each
    char unknown_1cae[0x2a43 - 0x1cae];
    unsigned char viewTeam;          // +0x2a43
    char unknown_2a44[0x1422b - 0x2a44];
    int mapWidth;                    // +0x1422b
    int mapHeight;                   // +0x1422f
    int rowWidth;                    // +0x14233
    int mapHeight2;                  // +0x14237
    char unknown_1423b[0x14273 - 0x1423b];
    unsigned short* visibilityMask;  // +0x14273
    char unknown_14277[0x142df - 0x14277];
    void* mappedSurface;             // +0x142df
    void* pictureSurface;            // +0x142e3
    short posX;                      // +0x142e7
    short posY;                      // +0x142e9
    short width;                     // +0x142eb
    short height;                    // +0x142ed
    short blinkTimer;                // +0x142ef
    unsigned short blinkOn : 1;      // +0x142f1, bit 0
    unsigned short pending : 1;
    unsigned short mapChanged : 1;
    unsigned short rest : 13;
};
#pragma pack(pop)

extern Game* g_game;

// FUNCTION: 0x466c20
void FUN_00466c20()
{
    if (g_game->mapChanged) {
        g_game->mapChanged = 0;
        unsigned char fog = g_game->fogColor;
        Team_00466c20* t = &g_game->players[g_game->viewTeam];
        unsigned int mask = 1 << g_game->viewTeam;
        unsigned char* dst = *(unsigned char**)((char*)g_game->mappedSurface + 0xc);
        unsigned char* src = *(unsigned char**)((char*)g_game->pictureSurface + 0xc);
        int halfWidth = g_game->rowWidth / 2;
        int halfHeight = g_game->mapHeight2 / 2;
        for (int i = 0; i < g_game->height; i++) {
            int mapY = i * halfHeight;
            int mapX = 0;
            for (int j = 0; j < g_game->width; j++, mapX += halfWidth, src++, dst++) {
                int index = (mapY / g_game->height) * halfWidth + mapX / g_game->width;
                if (!(unsigned short)(g_game->visibilityMask[index] & mask)) {
                    *dst = fog;
                } else if (t->seenMap[index]) {
                    *dst = *src;
                } else {
                    *dst = g_game->fx->colorMap[*src];
                }
            }
        }
        g_game->pending = 1;
    }
}
