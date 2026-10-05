// Decompiled by Opus. Names are provisional.
#include <string.h>

#pragma pack(push, 1)
struct Player_00406e40 {
    int active;                        // +0x00
    char unknown_4[0x73 - 0x4];
    unsigned char type;                // +0x73
    char unknown_74[0x14b - 0x74];
};

struct Game {
    char unknown_0[0x1b63];
    Player_00406e40 players[10];       // +0x1b63
};
#pragma pack(pop)

extern Game* g_game;
extern int DAT_00501774;
extern char DAT_005119b8[];

class Class_004b73c0 {
public:
    char* GetArg(int index, char* fallback);
};

class CommandArgs {
public:
    int GetIntArg(int index, int fallback);
};

// 0x40-byte set (512 bits).
class UnitTypeSet {
public:
    int bits[16];
    void AddTypeOrCategory(char* text, int* out);
};

void __stdcall FUN_00409e90(int player, UnitTypeSet* set, int value, int param_4);

// FUNCTION: 0x406e40
void __stdcall FUN_00406e40(CommandArgs* args)
{
    if (DAT_00501774 != 0) {
        int count;
        UnitTypeSet set;
        memset(&set, 0, sizeof(set));
        set.AddTypeOrCategory(((Class_004b73c0*)args)->GetArg(1, DAT_005119b8), &count);
        int value = args->GetIntArg(2, 0);
        // A narrow index: MSVC then counts the loop down in a separate
        // register instead of testing the player offset.
        for (char i = 0; i < 10; i++) {
            if (g_game->players[i].active != 0 && g_game->players[i].type == 2) {
                FUN_00409e90(i, &set, value, count);
            }
        }
    }
}
