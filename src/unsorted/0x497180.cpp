// Decompiled by deepseek-v4.1-flash, finished by space-bunny-free. Names are provisional.
//
// Start-of-battle setup. Seeds the RNG from the clock, then switches the game
// state on the campaign mode (1 demo, 2 campaign, 3 skirmish or network),
// handing each mode its start positions, team assignment and view centre, and
// finally builds the "MAIN2.GUI" gadget and releases the between-mission
// object.
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

union Fixed_497180 {
    int i;                                  // 16.16
    struct {
        short frac;
        short whole;
    } h;
};

struct FixedPos_497180 {
    Fixed_497180 x;
    Fixed_497180 y;
    Fixed_497180 z;
};

struct Gadget_497180 {
    char unknown_0[8];
    void (__stdcall* handler)(Gadget_497180*);  // +0x8
    char* owner;                                // +0xc
};

struct Sub_497180 {
    char unknown_0[0x10];
};

class Class_00435100 {
public:
    int FUN_00435100();
    void FUN_00435a20(void* player);
    int FUN_00437320(FixedPos_497180* pos, int id);
};

class Class_004618a0 {
public:
    void FUN_004618a0(int a);
};

class Class_004b4560 {
public:
    void FUN_004b4560(const char* name);
};

class Class_004b48f0 {
public:
    int FUN_004b48f0(const char* name);
};

class Class_004b3630 {
public:
    void FUN_004b3630();
};

#pragma pack(push, 1)
struct Player_497180 {                   // pointed at by a record's +0x27
    char unknown_0[0x8f];
    unsigned char ready;                  // +0x8f
    char unknown_90[0x95 - 0x90];
    unsigned char nameIndex;              // +0x95
    unsigned char index2;                 // +0x96
    char unknown_97[0x9b - 0x97];
    struct {
        unsigned short lo_b0 : 1;          // +0x9b bit 0
        unsigned short lo_b1 : 1;
        unsigned short lo_b2 : 1;
        unsigned short lo_b3 : 1;
        unsigned short lo_b4 : 1;
        unsigned short lo_b5 : 1;
        unsigned short lo_b6 : 1;          // +0x9b bit 6
        unsigned short lo_b7 : 1;
        unsigned short hi_b0 : 1;          // +0x9c bit 0
        unsigned short hi_b1 : 1;
        unsigned short hi_b2 : 1;
        unsigned short hi_rest : 5;
    } bits9b;                              // +0x9b, two bytes
    unsigned char unknown_9d[0xa1 - 0x9d];
    unsigned short size1;                 // +0xa1
    unsigned short size2;                 // +0xa3
    unsigned short flags_a5;              // +0xa5
    char unknown_a7[0x14b - 0xa7];
};

struct PlayerRec_497180 {                // 0x14b bytes, array at g_game+0x1b63
    int active;                           // +0x00
    unsigned char bits21;                 // +0x21
    char unknown_22[0x27 - 0x22];
    Player_497180* player;                // +0x27
    char unknown_2b[0x73 - 0x2b];
    unsigned char type;                   // +0x73
    char unknown_74[0xdc - 0x74];
    float size1;                          // +0xdc
    float size2;                          // +0xe0
    char unknown_e4[0x146 - 0xe4];
    unsigned char team;                   // +0x146
    unsigned char which;                  // +0x147
    unsigned short bits148 : 8;           // +0x148
    unsigned short started : 1;           // +0x149
    char unknown_14a[0x14b - 0x14a];
};

struct TeamDef_497180 {                  // 0x18-byte records
    int mode;                             // +0x00
    unsigned char nameIndex;              // +0x04
    unsigned char unknown_5[0x8 - 0x5];
    unsigned char flags9c;                // +0x08
    unsigned char unknown_9[0xc - 0x9];
    int size1;                            // +0x0c
    int size2;                            // +0x10
    unsigned char index2;                 // +0x14
    unsigned char unknown_15[0x18 - 0x15];
};

struct StartDef_497180 {                // the record at campaigns+0x108
    int startMode;                        // +0x00
    unsigned char nameIndex;              // +0x04
    unsigned char unknown_5[0x8 - 0x5];
    unsigned char flags9c;                // +0x08
    unsigned char unknown_9[0xc - 0x9];
    unsigned char flags10c;               // +0x0c
    char unknown_d[0x18 - 0xd];
};

struct Campaign_497180 {                 // pointed at by g_game+0x29a0
    TeamDef_497180 teams[10];             // +0x00
    char unknown_f0[0x108 - 0xf0];
    StartDef_497180 def2;                 // +0x108
    char unknown_114[0x118 - 0x114];
    int useOrder;                         // +0x118
};

struct MissionDef_497180 {               // at g_game+0x39219
    StartDef_497180 def;                  // +0x00
};

struct PlayerName_497180 {               // 0x232 bytes, array at g_game+0x37f5f
    char name[0x232];
};

struct NetModeFlags_497180 {             // at g_game+0x14281
    unsigned short b0 : 1;
    unsigned short b1 : 1;
    unsigned short b2 : 1;
    unsigned short rest : 13;
};

struct Game_497180 {
    char unknown_0[0x519];
    Sub_497180 sub;                       // +0x519
    char unknown_529[0x1b63 - 0x529];
    PlayerRec_497180 players[10];         // +0x1b63
    char unknown_2851[0x29a0 - 0x2851];
    Campaign_497180* campaign;            // +0x29a0
    char unknown_29a4[0x2a42 - 0x29a4];
    unsigned char localPlayer;            // +0x2a42
    unsigned char viewPlayer;             // +0x2a43
    char unknown_2a44[0x14223 - 0x2a44];
    int mapW;                             // +0x14223
    int mapH;                             // +0x14227
    char unknown_1422b[0x14281 - 0x1422b];
    NetModeFlags_497180 flags;            // +0x14281
    char unknown_14283[0x37e37 - 0x14283];
    int viewWidth;                        // +0x37e37
    int viewHeight;                       // +0x37e3b
    char unknown_37e3f[0x37ea0 - 0x37e3f];
    char guiName[0x37ee6 - 0x37ea0];
    unsigned short viewTeam;              // +0x37ee6
    char unknown_37ee8[0x37eec - 0x37ee8];
    unsigned short camStart;              // +0x37eec
    char unknown_37eee[0x37ef6 - 0x37eee];
    int startMode;                        // +0x37ef6
    char unknown_37efa[0x37f5b - 0x37efa];
    char namePad[4];                      // +0x37f5b
    PlayerName_497180 names[4];           // +0x37f5f
    char unknown_38827[0x38a47 - 0x38827];
    int lastCount;                        // +0x38a47
    char unknown_38a4b[0x38a51 - 0x38a4b];
    unsigned short bits38a51 : 1;         // +0x38a51
    unsigned short rest38a51 : 15;
    char unknown_38a53[0x38d6b - 0x38a53];
    void* mission;                        // +0x38d6b
    char unknown_38d6f[0x38d75 - 0x38d6f];
    volatile unsigned short netflags;     // +0x38d75
    char unknown_38d77[0x38d81 - 0x38d77];
    int playerCount;                      // +0x38d81
    char unknown_38d85[0x391e9 - 0x38d85];
    Class_00435100* net;                  // +0x391e9
    char unknown_391ed[0x39219 - 0x391ed];
    MissionDef_497180 missionDef;         // +0x39219
};
#pragma pack(pop)

extern Game_497180* g_game;
extern int DAT_005091cc;
extern int DAT_00506dbc;
extern Class_004618a0 DAT_00513000;

void __stdcall FUN_004b6ca0(int x);
void __stdcall FUN_004b6b50(int x);
int __stdcall FUN_004b6c30(int x);
unsigned char FUN_00456850();
void FUN_00431740();
void FUN_00453d40();
void __stdcall FUN_00465fb0(void* mission);
void FUN_0047a760();
void FUN_004917d0();
void FUN_00465e30();
void __stdcall FUN_004816a0(int x);
void __stdcall FUN_00432610(void* mission);
void FUN_00488310();
void FUN_0041d1f0();
void __stdcall FUN_004288d0(int a, int b, int c, int d);
void FUN_00450f90();
void FUN_00451180();
void FUN_00464f80();
void FUN_0046c620(int x);
void FUN_004649d0();
void __stdcall FUN_0041c4c0(int x, int y, int z);
unsigned short __stdcall FUN_00488b10(const char* name);
void __stdcall FUN_00496ee0(int team, int startpos);
void __stdcall FUN_00485f50(int team, unsigned short id, FixedPos_497180 pos, int a, int b,
    int c);
Gadget_497180* __stdcall FUN_004aa8f0(Sub_497180* sub, const char* name, int flags);
void __stdcall FUN_00494890(Gadget_497180* gadget);
void operator delete(void* p);

// FUNCTION: 0x497180
void FUN_00497180(void)
{
    LARGE_INTEGER perfCount;
    FixedPos_497180 pos;
    FixedPos_497180 start;
    int order[10];

    QueryPerformanceCounter(&perfCount);
    FUN_004b6ca0(perfCount.LowPart + perfCount.HighPart);
    srand((unsigned)time(NULL));
    g_game->lastCount = 0;

    switch (g_game->net->FUN_00435100()) {
    case 1: {
        StartDef_497180* def = &g_game->missionDef.def;
        DAT_005091cc = 0;
        g_game->startMode = def->startMode;
        g_game->flags.b2 = def->flags10c;
        g_game->flags.b0 = def->nameIndex ^ g_game->flags.b0;
        g_game->flags.b1 = def->flags9c;
        FUN_00431740();
        break;
    }
    case 2: {
        StartDef_497180* def = &g_game->campaign->def2;
        g_game->viewTeam = g_game->camStart;
        DAT_005091cc = 1;
        g_game->startMode = def->startMode;
        g_game->flags.b2 = def->flags10c;
        g_game->flags.b0 = def->nameIndex ^ g_game->flags.b0;
        g_game->flags.b1 = def->flags9c;
        break;
    }
    case 3: {
        int sel;
        g_game->viewTeam = g_game->camStart;
        DAT_005091cc = 1;
        g_game->bits38a51 = 0;
        sel = FUN_00456850();
        if (g_game->players[g_game->localPlayer].type & 2) {
            Player_497180* p;
            do {
                p = g_game->players[g_game->localPlayer].player;
                if (DAT_00506dbc)
                    DAT_00513000.FUN_004618a0(1);
                FUN_00453d40();
                sel = FUN_00456850();
                FUN_004b6b50(0x32);
            } while (sel == 10 || p->index2 == -1 || !p->ready);
            FUN_004b6b50(0x32);
        }
        g_game->net->FUN_00435a20(g_game->players[sel].player);
        if (FUN_00456850() == 10)
            break;
        Player_497180* pl = g_game->players[FUN_00456850()].player;
        DAT_005091cc = ((unsigned short)(((unsigned char*)pl)[0] | (((unsigned char*)pl)[1] << 8)) >> 0xd) & 1;
        g_game->startMode = ((unsigned short)(((unsigned char*)pl)[0] | (((unsigned char*)pl)[1] << 8)) >> 0xb) & 3;
        g_game->flags.b1 = pl->bits9b.hi_b0;
        g_game->flags.b2 = pl->bits9b.hi_b0;
        g_game->flags.b0 = pl->bits9b.hi_b0;
        g_game->viewTeam = pl->flags_a5;
        break;
    }
    default:
        break;
    }

    if (g_game->mission != 0) {
        ((Class_004b4560*)g_game->mission)->FUN_004b4560("summary");
        if (((Class_004b48f0*)g_game->mission)->FUN_004b48f0("BetweenMissions") == 0) {
            FUN_00465fb0(g_game->mission);
            if (g_game->net->FUN_00435100() == 2) {
                TeamDef_497180* def = g_game->campaign->teams;
                int count = 0;
                for (int i = 0; i < 10; i++) {
                    int mode = def->mode;
                    if (mode == 1 || mode == 2)
                        count = i + 1;
                    def++;
                }
                int cur = g_game->playerCount;
                if (count > cur)
                    cur = count;
                g_game->playerCount = cur;
                FUN_0047a760();
            }
        }
    }

    FUN_004917d0();

    if (g_game->net->FUN_00435100() != 1) {
        if (g_game->net->FUN_00435100() == 3) {
            Player_497180* selPl;
            g_game->netflags |= 4;
            while ((g_game->netflags & 8) == 0)
                FUN_004b6b50(0x32);
            selPl = g_game->players[FUN_00456850()].player;
            g_game->flags.b0 = selPl->bits9b.hi_b0;
            g_game->flags.b1 = selPl->bits9b.hi_b0;
            g_game->flags.b2 = selPl->bits9b.hi_b0;
            g_game->startMode = ((unsigned short)(((unsigned char*)selPl)[0] | (((unsigned char*)selPl)[1] << 8)) >> 0xb) & 3;
            for (int i = 0; i < 10; i++) {
                if (g_game->players[i].active == 0)
                    continue;
                PlayerRec_497180* rec = &g_game->players[i];
                if (rec->type == 1 || rec->type == 2) {
                    Player_497180* pl2;
                    int n;
                    pos.y.i = 0;
                    pos.x.i = (FUN_004b6c30(g_game->mapW - 0xa0) + 0x50) << 16;
                    pos.z.i = (FUN_004b6c30(g_game->mapH - 0xa0) + 0x50) << 16;
                    if (rec->active != 0 && (rec->player->flags9b.b.lo & 0x40))
                        continue;
                    pl2 = rec->player;
                    n = pl2->nameIndex;
                    g_game->net->FUN_00437320(&pos, rec->which);
                    if (rec->active != 0 && rec->type == 1) {
                        start.x = pos.x;
                        start.y = pos.y;
                        start.z = pos.z;
                    }
                    unsigned short id = FUN_00488b10(g_game->names[n].name);
                    int s1 = selPl->size1 * 100;
                    int s2 = selPl->size2 * 100;
                    FUN_00485f50(rec->team, id, pos, 1, 1, 0);
                    rec->started = 1;
                    rec->size1 = (float)(s1 >= 200 ? s1 : 200);
                    rec->size2 = (float)(s2 >= 200 ? s2 : 200);
                }
            }
            {
                Player_497180* lp = g_game->players[g_game->localPlayer].player;
                unsigned char f = lp->bits9b.lo_b6;
                int hw = g_game->viewWidth / 2;
                int hh = g_game->viewHeight / 2;
                if (((f >> 6) & 1) != 0) {
                    g_game->flags.b0 = 0;
                    g_game->flags.b1 = 0;
                    FUN_0041c4c0(hw, hh, 0);
                } else {
                    FUN_0041c4c0(start.x.h.whole - hw, start.z.h.whole - hh, 0);
                }
            }
            FUN_0046c620(6);
        } else if (g_game->net->FUN_00435100() == 2 && g_game->mission == 0) {
            if (g_game->campaign->useOrder != 0) {
                for (int i = 0; i < 10; i++) {
                    if ((unsigned char)i < 10) {
                        PlayerRec_497180* rec = &g_game->players[i];
                        if (rec->active != 0
                            && (rec->type == 1 || rec->type == 2 || rec->type == 3)
                            && rec->team != 10)
                            FUN_00496ee0(i, i);
                    }
                }
            } else {
                int* slot = order;
                int n = 0;
                for (int k = 0; k < 10; k++)
                    order[k] = -1;
                for (int j = 0; j < 10; j++) {
                    if ((unsigned char)j < 10) {
                        PlayerRec_497180* rec = &g_game->players[j];
                        if (rec->active != 0
                            && (rec->type == 1 || rec->type == 2 || rec->type == 3)
                            && rec->team != 10) {
                            *slot++ = j;
                            n++;
                        }
                    }
                }
                if (n > 2 || (int)(((__int64)rand() * 2) / 0x8000) != 0) {
                    for (unsigned int m = 1; m < (unsigned int)n; m++) {
                        int r = rand() % m;
                        int t = order[m];
                        order[m] = order[r];
                        order[r] = t;
                    }
                }
                slot = order;
                for (int l = 0; l < 10; l++) {
                    if ((unsigned char)l < 10) {
                        PlayerRec_497180* rec = &g_game->players[l];
                        if (rec->active != 0
                            && (rec->type == 1 || rec->type == 2 || rec->type == 3)
                            && rec->team != 0x0a) {
                            int s = *slot++;
                            FUN_00496ee0(l, s);
                        }
                    }
                }
            }
            FUN_00465e30();
        }
    }

    FUN_004816a0(1);

    if (g_game->mission) {
        ((Class_004b4560*)g_game->mission)->FUN_004b4560("summary");
        if (((Class_004b48f0*)g_game->mission)->FUN_004b48f0("BetweenMissions") == 0) {
            FUN_00432610(g_game->mission);
        } else {
            FUN_00488310();
            FUN_0041d1f0();
        }
    } else if (g_game->net->FUN_00435100() == 1) {
        FUN_00488310();
        FUN_0041d1f0();
    }

    sprintf(g_game->guiName, "%sMAIN2.GUI",
        g_game->namePad
            + 0x232 * g_game->players[g_game->viewPlayer].player->nameIndex);
    FUN_004288d0(0, g_game->viewPlayer, 0, 0);

    Gadget_497180* gadget =
        FUN_004aa8f0(&g_game->sub, g_game->guiName, 0x20);
    gadget->handler = FUN_00494890;
    gadget->owner = (char*)g_game;

    g_game->players[g_game->localPlayer].player->flags9b.b.lo |= 0x10;
    FUN_00450f90();
    FUN_00451180();
    FUN_00464f80();
    FUN_00465e30();

    if (g_game->mission != 0) {
        ((Class_004b3630*)g_game->mission)->FUN_004b3630();
        operator delete(g_game->mission);
        g_game->mission = 0;
    }
    FUN_004649d0();

    g_game->netflags |= 2;
}
