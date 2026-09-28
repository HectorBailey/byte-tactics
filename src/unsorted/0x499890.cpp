// Decompiled by deepseek-v4.1-flash. Names are provisional.
// Per-frame input handler: peeks the next queued event and reacts to a few
// commands (0x7e play/pause, 0xe3 cancel, 0xd6 screenshot, which makes a
// "%s\\screenshots" directory, takes a shot named SHOT and stores its handle
// at +0x38a37). It then peeks two more events, pops one of them into the view
// rect at +0x2c76 when the third dword matches (or copies the first/second
// event there for codes 0x205/0x202 and otherwise), ticks the timers and calls
// the current state handler unless the active object is paused.
//
// The bit at +0x37f2f is an unsigned short bitfield at an odd (packed) offset:
// only a standalone `if (bitfield)` emits the "mov cl; shr cl, 1; test cl, 1"
// form, inside an `&&` chain MSVC folds it to `test byte ptr [m], 2`.
#include <stdio.h>

struct Event_00499890 {
    int data[6];
};

struct Sub_00499890 {
    char unknown_0[0xf1];
    unsigned char flags;               // +0xf1
};

class Class_00435100 {
public:
    int FUN_00435100();
};

#pragma pack(push, 1)
struct Game_00499890 {
    char unknown_0[0xc];
    Sub_00499890* field_c;             // +0xc
    char unknown_10[0x519 - 0x10];
    char queue_519[0x2c76 - 0x519];    // +0x519
    Event_00499890 view_2c76;          // +0x2c76
    char unknown_2c8e[0x37ebe - 0x2c8e];
    unsigned short flags_37ebe;        // +0x37ebe
    char unknown_37ec0[0x37f2f - 0x37ec0];
    unsigned short bit0_37f2f : 1;     // +0x37f2f
    unsigned short flag_37f2f : 1;
    unsigned short rest_37f2f : 14;
    char unknown_37f31[0x38a37 - 0x37f31];
    int field_38a37;                   // +0x38a37
    char unknown_38a3b[0x38a51 - 0x38a3b];
    unsigned short field_38a51;        // +0x38a51
    char path_38a53[260];              // +0x38a53
    char unknown_38b57[0x391e9 - 0x38b57];
    Class_00435100* field_391e9;       // +0x391e9
    char unknown_391ed[0x391f5 - 0x391ed];
    void (*handler)();                 // +0x391f5
};
#pragma pack(pop)

extern Game_00499890* g_game;

int __cdecl FUN_004c1b00();
int FUN_004c1ab0();
void FUN_004b5910();
int __stdcall FUN_00491d70(int force);
int __stdcall FUN_004bcf00(char* path);
int __stdcall FUN_004cb170(char* param_1, char* param_2);
unsigned int FUN_004b6340();
int __stdcall FUN_004c2de0(Event_00499890* out);
void __stdcall FUN_004a9fd0(void* param);
void FUN_00494e70();
void FUN_0047f680();
int __stdcall FUN_004c2d60(Event_00499890* out);
void FUN_004b6370();

// FUNCTION: 0x499890
void FUN_00499890()
{
    int event = FUN_004c1b00();
    if (event == 0x7e) {
        if (g_game->flag_37f2f) {
            FUN_004c1ab0();
            FUN_004b5910();
        }
    }
    if ((g_game->flags_37ebe & 1) && event == 0xe3) {
        unsigned short clear = 0xfffe;
        FUN_004c1ab0();
        FUN_00491d70(0);
        g_game->flags_37ebe &= clear;
        if (g_game->field_391e9->FUN_00435100() != 3) {
            g_game->field_38a51 &= clear;
        }
    }
    if (event == 0xd6) {
        char buf[256];
        FUN_004c1ab0();
        FUN_004bcf00(g_game->path_38a53);
        sprintf(buf, "%s\\screenshots", g_game->path_38a53);
        FUN_004bcf00(buf);
        FUN_004cb170(buf, "SHOT");
        g_game->field_38a37 = FUN_004b6340();
    }
    Event_00499890 e1;
    FUN_004c2de0(&e1);
    FUN_004a9fd0(g_game->queue_519);
    Event_00499890 e2;
    FUN_004c2de0(&e2);
    FUN_00494e70();
    FUN_0047f680();
    if (e1.data[4] == e2.data[4]) {
        FUN_004c2d60(&g_game->view_2c76);
    } else if (e1.data[4] == 0x205 || e1.data[4] == 0x202) {
        g_game->view_2c76 = e1;
    } else {
        g_game->view_2c76 = e2;
    }
    FUN_004b6370();
    if (!(g_game->field_c->flags & 8)) {
        g_game->handler();
    }
}
