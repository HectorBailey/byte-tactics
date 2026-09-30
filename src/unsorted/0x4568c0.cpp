// Decompiled by deepseek-v4.1-flash, finished by space-bunny-free, edited by
// deepseek-v4.1, finished by deepseek-v4.1-flash. Names are provisional.
// Partial, 78.9%. A short loop index restores three induction registers.
// Remaining difference, a single 4-byte stack slot. The original allocates
// 0x34 (sub esp,0x34) and keeps only two dword locals below the candidate
// array: esp+0x14 and esp+0x18, with cand[10] at esp+0x1c. esp+0x14 is
// reused three times by MSVC, first for the inlined index loop's byte
// counter, then for `int* out = &g_game->field_29fc` (0x456977 stores it
// there), then for the returning flag (0x456b7b stores the immediate 1,
// 0x456bc1 stores 0, and every epilogue reads [esp+0x14] into eax). res is
// the only other local, at esp+0x18. This file merges the index with the
// flag but gives `out` its own slot at esp+0x18, so res moves down to
// esp+0x1c and cand to esp+0x20, and the frame is 0x38: every [esp+..]
// reference in the body is off by 4. Making out share esp+0x14 needs the
// index variable to be gone from the source (the call site read at
// 0x45691b is the inlined helper's own counter), and inlining the helper
// call into the players[] expression was tried and changed nothing.
// Otherwise the body matches; the remaining misses are the readiness tests
// around 0x456b91 (field_29a4 / field_29d0) and register scheduling.
//
// deepseek-v4.1-flash re-checked the frame problem and confirmed the
// coefficient map from a /Fa listing. i/idx/ret sit at -0x34 and out at
// -0x30 here, while the original has all four at -0x30; res and cand are
// already at the original's absolute offsets. So only the i/idx/ret group
// needs to fold into out's slot and the frame drops from 0x38 to 0x34.
// Nothing tried moved it: inlining FindOccupied into the players[] index
// (vA), declaring out before res and assigning in place (vR), a function
// scope `int* out;` (vB), and a block scoping the idx local (vD) all score
// 78.9%; dropping the out local entirely scores 64.1% (vC) and assigning
// out at the top 76.6% (vS). Reordering the tail so res==0 is the
// fall-through path matches the original's `jne` but still scores 78.4%
// (the frame dominates), and changing the k4 loop counter from
// unsigned short to int (the original compares the pointer offset against
// 0x29f8, cmp bx,0xa here) drops to 77.1%.
#include <stdlib.h>
#include <algorithm>

#pragma pack(push, 1)
struct PlayerInfo_004568c0 {
    char unknown_0[0x97];
    unsigned short flag_97_0 : 1;
    unsigned short rest_97 : 15;
    char unknown_99[0x9b - 0x99];
    unsigned short pad_9b_a : 6;
    unsigned short flag_9b_6 : 1;
    unsigned short pad_9b_b : 7;
    unsigned short flag_9b_14 : 1;
};

class Class_00456030 {
  public:
    int field_0;
    char unknown_4[0x73 - 0x4];
    char field_73;
    int FUN_00456030();
};

class Player_004568c0 {
  public:
    int active;
    int id;
    char unknown_8[0x27 - 0x8];
    PlayerInfo_004568c0* info;
    char unknown_2b[0x73 - 0x2b];
    unsigned char state;
    char unknown_74[0x146 - 0x74];
    unsigned char field_146;
    unsigned char field_147;
    char unknown_148[0x14b - 0x148];
};

struct Game_004568c0 {
    char unknown_0[0x1b63];
    Player_004568c0 players[10];
    char unknown_2851[0x29a4 - 0x2851];
    int field_29a4[10];
    char unknown_29cc[0x29d0 - 0x29cc];
    int field_29d0[10];
    char unknown_29f8[0x29fc - 0x29f8];
    int field_29fc[10];
    char unknown_2a24[0x2a28 - 0x2a24];
    int field_2a28;
    char unknown_2a2c[0x2a42 - 0x2a2c];
    unsigned char localPlayer;
};
#pragma pack(pop)

extern Game_004568c0* g_game;

int __stdcall FUN_00451bc0(int from, int to, void* packet, int size);
int __stdcall FUN_00451df0(int id, void* packet, int size);

static inline unsigned char FindOccupied_004568c0() {
    for (unsigned char i = 0; i < 10; i++) {
        if (g_game->players[i].state != 0 && g_game->players[i].info->flag_97_0)
            return i;
    }
    return 10;
}

static inline int PlayerId_004568c0(unsigned char pi) {
    int id = -1;
    if (pi != 10 && g_game->players[pi].state != 0)
        id = g_game->players[pi].id;
    return id;
}

// FUNCTION: 0x4568c0
int FUN_004568c0() {
    unsigned char idx = FindOccupied_004568c0();
    int res = ((Class_00456030*)&g_game->players[idx])->FUN_00456030();
    if (res != 0 && g_game->field_2a28 == 0) {
        int* out = g_game->field_29fc;
        if (g_game->players[g_game->localPlayer].info->flag_9b_14) {
            int n = 0;
            for (int k0 = 0; k0 < 10; k0++) {
                Player_004568c0* q = &g_game->players[k0];
                if (q->active != 0 && (q->state == 1 || q->state == 2 || q->state == 3) &&
                    q->field_146 != 10 && (q->info->flag_9b_6) == 0)
                    out[k0] = n++;
                else
                    out[k0] = -1;
            }
        } else {
            int cand[10];
            for (int z = 0; z < 10; z++)
                cand[z] = -1;
            int n = 0;
            unsigned char* q = &g_game->players[0].state;
            int cnt = 10;
            do {
                Player_004568c0* p = (Player_004568c0*)(q - 0x73);
                if (p->active != 0 && (p->state == 1 || p->state == 2 || p->state == 3) &&
                    p->field_146 != 10 && (p->info->flag_9b_6) == 0) {
                    cand[n] = n;
                    n++;
                }
                q += 0x14b;
            } while (--cnt);
            if (n > 2 || (int)((__int64)rand() * 2 / 0x8000) != 0)
                std::random_shuffle(cand, cand + n);
            int* cp = cand;
            for (int k2 = 0; k2 < 10; k2++) {
                Player_004568c0* q2 = &g_game->players[k2];
                if (q2->active != 0 && (q2->state == 1 || q2->state == 2 || q2->state == 3) &&
                    q2->field_146 != 10) {
                    if (g_game->players[k2].active != 0 && (q2->info->flag_9b_6))
                        out[k2] = -1;
                    else
                        out[k2] = *cp++;
                } else {
                    out[k2] = -1;
                }
            }
        }
        g_game->field_2a28 = 1;
    }
    int ret = 1;
    for (int k3 = 0; k3 < 10; k3++) {
        Player_004568c0* q = &g_game->players[k3];
        if (q->active != 0 && q->state == 3) {
            if (res != 0) {
                if (g_game->field_29a4[k3] == 0 || g_game->field_29d0[k3] == 0) {
                    ret = 0;
                    break;
                }
            } else {
                if (g_game->field_29a4[k3] == 0) {
                    ret = 0;
                    break;
                }
            }
        }
    }
    if (res != 0) {
        for (unsigned short k4 = 0; k4 < 10; k4++) {
            if (g_game->field_29d0[k4] == 0) {
                unsigned char packet[2];
                packet[0] = 0x1e;
                packet[1] = (unsigned char)g_game->field_29fc[k4];
                if (g_game->players[k4].active != 0) {
                    if (g_game->players[k4].state == 3) {
                        int to = PlayerId_004568c0(k4);
                        int from = -1;
                        for (int j = 0; j < 10; j++) {
                            if (g_game->players[j].state == 1) {
                                from = g_game->players[j].id;
                                break;
                            }
                        }
                        FUN_00451bc0(from, to, packet, 2);
                    } else if (g_game->players[k4].active != 0 &&
                               (g_game->players[k4].state == 1 || g_game->players[k4].state == 2)) {
                        g_game->players[k4].field_147 = packet[1];
                        g_game->field_29d0[k4] = 1;
                    }
                }
            }
        }
    }
    unsigned char pkt = 0x15;
    if (res != 0) {
        if (ret != 0) {
            for (int k5 = 0; k5 < 10; k5++) {
                Player_004568c0* q = &g_game->players[k5];
                if (q->active != 0 && (q->state == 1 || q->state == 2))
                    FUN_00451df0(PlayerId_004568c0(k5), &pkt, 1);
            }
        }
        return ret;
    }
    for (int k6 = 0; k6 < 10; k6++) {
        Player_004568c0* q = &g_game->players[k6];
        if (q->active != 0 && (q->state == 1 || q->state == 2))
            FUN_00451df0(PlayerId_004568c0(k6), &pkt, 1);
    }
    return ret;
}
