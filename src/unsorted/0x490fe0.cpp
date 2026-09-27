// Decompiled by deepseek-v4.1-flash. Names are provisional.
#include <string.h>
#include <windows.h>

class Class_004ce450 {
public:
    int FUN_004ce450();
};

class Class_004ce7e0 {
public:
    unsigned char FUN_004ce7e0(int param_1);
};

class Class_004cd9c0 {
public:
    int FUN_004cd9c0();
};

class Class_004cdb40 {
public:
    void FUN_004cdb40();
};

class Class_004ced40 {
public:
    int FUN_004ced40();
};

class Class_004cedc0 {
public:
    void FUN_004cedc0(int on);
};

class Class_004ce3e0 {
public:
    void FUN_004ce3e0(const void* src);
};

class CD {
public:
    char unknown_0[0x200];
    int size;                          // +0x200
    char unknown_204[0x278 - 0x204];
    int field_278;                     // +0x278

    int FUN_004ce450();
    int FUN_004ce460();
    int FUN_004ce680();
    int FUN_004ce690(int);
    int FUN_004ce7a0(int);
    int FUN_004cd9c0();
    void FUN_004cdb40();
    int FUN_004ced40();
    void FUN_004cedc0(int);
    void FUN_004ce3e0(const void*);
};

struct Game_490fe0 {
    char unknown_0[0x10];
    CD* cd;                            // +0x10
    char unknown_14[0x2a44 - 0x14];
    unsigned char flags_2a44;          // +0x2a44
    char unknown_2a45[0x37f14 - 0x2a45];
    unsigned char field_37f14;         // +0x37f14
    unsigned char unknown_37f15;
    unsigned char field_37f16;         // +0x37f16
    char unknown_37f17[0x391f1 - 0x37f17];
    int mode;                          // +0x391f1
};

extern Game_490fe0* g_game;

struct CdLists_490fe0 {
    char unknown_0[0xaa0];
};

extern CdLists_490fe0 DAT_0051e828;
extern int DAT_0051e848;
extern int DAT_0051e84c;
extern int DAT_0051e850;
extern int DAT_0051e854;
extern int DAT_0051e858;
extern int DAT_0051f2e8;

// FUNCTION: 0x490fe0
void FUN_00490fe0()
{
    char tracks[16] = {1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    char buf[0x88];

    int saved = g_game->cd->FUN_004ce680();
    mciSendStringA("stop cdaudio", 0, 0, 0);
    mciSendStringA("close cdaudio", 0, 0, 0);
    mciSendStringA("open cdaudio", 0, 0, 0);
    g_game->cd->FUN_004cedc0(g_game->field_37f14 & 1);
    g_game->cd->FUN_004ce7a0(g_game->field_37f16);
    g_game->cd->FUN_004ce690(saved);

    int id = g_game->cd->FUN_004cd9c0();
    int index = 0;
    int* slot = &DAT_0051e848;
    while (*slot != id) {
        slot = (int*)((char*)slot + 0x88);
        index++;
        if (slot >= &DAT_0051f2e8)
            goto newdisc;
    }
    {
        memcpy(buf, (char*)&DAT_0051e828 + index * 0x88, 0x88);
        for (int j = index; j != 0; j--)
            memcpy((char*)&DAT_0051e828 + j * 0x88,
                   (char*)&DAT_0051e828 + (j - 1) * 0x88, 0x88);
        memcpy(&DAT_0051e828, buf, 0x88);
        g_game->cd->FUN_004ce3e0(&DAT_0051e84c);
    }
newdisc:
    if (index == 0x14) {
        if (g_game->cd->FUN_004ce450() == 0x10) {
            if (g_game->cd->FUN_004ce460() != 0) {
                g_game->cd->FUN_004ce3e0(tracks);
            }
        }
        for (int k = 0xaa0; k > 0; k -= 0x88)
            memcpy((char*)&DAT_0051e828 + k,
                   (char*)&DAT_0051e828 + k - 0x88, 0x88);
        DAT_0051e84c = *(int*)&tracks[0];
        DAT_0051e850 = *(int*)&tracks[4];
        DAT_0051e854 = *(int*)&tracks[8];
        DAT_0051e858 = *(int*)&tracks[12];
        DAT_0051e848 = id;
    }
    if ((g_game->flags_2a44 & 4) != 0 && g_game->mode == 6)
        g_game->cd->FUN_004cdb40();
    else
        g_game->cd->FUN_004ced40();
}
