// Decompiled by space-bunny-free, finished by muse-spark-1.3-free, finished by space-bunny-free, edited by deepseek-v4.1, finished by deepseek-v4.1-flash, finished by mimo-v2.6-pro. Names are provisional.
// RETRY mimo-v2.6-pro: 87.7% at 848 bytes (the exact original size). The flag
// encoding that every earlier pass fought over is now byte-identical. The
// original reads the +0x97 flag byte as `xor eax,eax; mov al,[ecx+0x97];
// and eax,1`: a full zero-extension followed by the mask. Every simple
// `flags & 1` spelling folds to `mov dl,[ecx+0x97]; and edx,1` (2 bytes
// shorter), which also forced the loop counter register and destroyed the
// schedule. Isolating candidates in scratch files and disassembling them
// shows the fold is defeated only when the byte value has a second use that
// the optimizer later removes. The shape that reproduces the original:
//
//     int f = p->data->flags;
//     p->data->flags |= 0;      // no-op RMW; its store is dead-stored away
//     int flag = f & 1;
//
// The `|= 0` is a stand-in for whatever second use the original had (the
// store is eliminated, so nothing of it remains in the original either);
// flag it in review. `flags = f` and `flags = f | 0` keep a store, plain
// `int flag = flags & 1` and ~30 other spellings all fold (see
// build/scratch/0x452cc0/flagtest*.cpp). With the widen fixed, the plain
// `for (int i = 0; i < 10; i++)` clearing loop emits the original's
// bottom-tested latch (`add edx,0x14b; cmp edx,0xcee; jl`, no entry guard)
// and the flag preheader/schedule (`and eax,1; xor edx,edx;
// mov [esp+0x10],eax`), and the file is exactly 848 bytes.
//
// STILL DIFFERENT (the whole remaining 12.3%): one cyclic register rotation.
// Original: g_game=ebx, p=esi, slot=edi. Ours: g_game=edi, p=ebx, slot=esi.
// Every hunk of the check diff is that rotation; instruction shapes, sizes,
// reloads and schedules all match. Ruled out on this 87.7 base: hoisting
// `unsigned char slot;` above fi (flat 87.7), a function-scope
// `Game_00452cc0* g = g_game;` used everywhere (52.1%), swapping the two
// arms of the p if/else, and spelling the clearing loop through
// `g_game->players[i]` instead of a `q` pointer. On the older 84.3 base the
// earlier passes ruled out declaration-order sweeps, the register keyword,
// extra g_game uses, N-dead-declaration sweeps, unsigned/byte/bitfield flag
// types and inlined flag helpers (their notes follow).
//
// History of the 84.3 base (two diffs then: the flag encoding and this same
// rotation):
#include <stdio.h>
#include <string.h>

#pragma pack(push, 1)

class Class_00435100 {
public:
    int FUN_00435100();
};

class Class_00463c60 {
public:
    void FUN_00463c60(int param_1);
};

struct PlayerData_00452cc0 {
    char unknown_0[0x94];
    unsigned char field_94;            // +0x94
    char unknown_95[0x97 - 0x95];
    unsigned char flags;               // +0x97
    char unknown_98[0x9d - 0x98];
    unsigned short word_9d;            // +0x9d
};

struct Player_00452cc0 {
    int active;                        // +0x00
    unsigned int id;                   // +0x04
    char unknown_8[0xc - 8];
    int field_c;                       // +0x0c
    char unknown_10[0x27 - 0x10];
    PlayerData_00452cc0* data;         // +0x27
    char unknown_2b[0x73 - 0x2b];
    unsigned char type;                // +0x73
    char unknown_74[0x108 - 0x74];
    unsigned char allies[0xb];         // +0x108
    unsigned char field_113[0xb];      // +0x113
    char unknown_11e[0x146 - 0x11e];
    unsigned char field_146;           // +0x146
    char unknown_147[0x14b - 0x147];
};

struct Game_00452cc0 {
    char unknown_0[0x14];
    char unknown_14[0x1b63 - 0x14];
    Player_00452cc0 players[10];       // +0x1b63
    char unknown_2851[0x2a3c - 0x2851];
    unsigned short field_2a3c;         // +0x2a3c
    char unknown_2a3e[0x2a44 - 0x2a3e];
    union {
        unsigned short value;          // +0x2a44
        struct {
            unsigned short b0 : 1;
            unsigned short b1 : 1;
            unsigned short b2 : 1;
            unsigned short b3 : 1;
            unsigned short rest : 12;
        };
    } flags;
    char unknown_2a45[0x391e9 - 0x2a46];
    Class_00435100* net;               // +0x391e9
};
#pragma pack(pop)

extern Game_00452cc0* g_game;

int __stdcall FUN_0044ffd0(unsigned char index);
unsigned char __stdcall FUN_0044fe40(int id);
void __stdcall FUN_00486f10(unsigned char player);
void __stdcall FUN_0046c620(int msg);
int __stdcall FUN_004ca780(void* net, int id);

static inline unsigned char FindIndex_00452cc0(int id)
{
    for (unsigned char i = 0; i < 10; i++) {
        if (FUN_0044ffd0(i) == id)
            return i;
    }
    return 10;
}

static inline int PlayerId_00452cc0(unsigned char i)
{
    if (i != 10 && g_game->players[i].type != 0)
        return g_game->players[i].id;
    return -1;
}

static inline unsigned char FindPlayerIndex_00452cc0(int id)
{
    if (id != -1) {
        for (unsigned char i = 0; i < 10; i++) {
            if (PlayerId_00452cc0(i) == id)
                return i;
        }
    }
    return 10;
}

static inline int IsActive12_00452cc0(Player_00452cc0* p)
{
    if (p->active == 0)
        return 0;
    if (p->type == 1 || p->type == 2)
        return 1;
    return 0;
}

static inline int IsType1_00452cc0(Player_00452cc0* p)
{
    if (p->active == 0)
        return 0;
    if (p->type == 1)
        return 1;
    return 0;
}

static inline int IsType3_00452cc0(Player_00452cc0* p)
{
    if (p->active == 0)
        return 0;
    if (p->type == 3)
        return 1;
    return 0;
}

// FUNCTION: 0x452cc0
void __stdcall FUN_00452cc0(int id)
{
    unsigned char fi;
    if (id == -1)
        fi = 10;
    else
        fi = FindIndex_00452cc0(id);

    Player_00452cc0* p;
    if (fi == 10)
        p = 0;
    else
        p = &g_game->players[FUN_0044fe40(id)];

    if (p == 0)
        return;
    if (p->active == 0)
        return;
    if (p->type != 1 && p->type != 2 && p->type != 3)
        return;
    if (p->field_146 == 10)
        return;

    unsigned char slot = p->field_146;
    int f = p->data->flags;
    p->data->flags |= 0;
    int flag = f & 1;

    for (int i = 0; i < 10; i++) {
        Player_00452cc0* q = &g_game->players[i];
        if (q->active != 0 && (q->type == 1 || q->type == 2)) {
            q->field_113[slot] = 0;
            q->allies[slot] = 0;
        }
    }

    FUN_00486f10(FindPlayerIndex_00452cc0(id));

    if (g_game->flags.b2) {
        if (p->active == 0)
            goto do_remove;
        if (p->type == 1)
            goto after_remove;
        if (p->type == 2)
            goto after_remove;
        goto do_remove;
    }
    if (p->active != 0 && (p->type == 1 || p->type == 2))
        FUN_004ca780((char*)g_game + 0x14, p->id);

do_remove:
    ((Class_00463c60*)p)->FUN_00463c60(0);
    p->active = 0;
    p->id = -1;
    p->field_c = 0;

after_remove:
    g_game->field_2a3c--;
    p->data->word_9d &= 0xfffb;
    memset(&p->allies, 0, 11);

    if (g_game->net->FUN_00435100() == 3)
        FUN_0046c620(3);

    if ((g_game->flags.value & 4) && flag != 0) {
        unsigned int best = 0;
        Player_00452cc0* q = g_game->players;
        int n = 10;
        do {
            if ((q->active != 0 && q->type == 3)
                || (q->active != 0 && q->type == 1)) {
                if (q->id > best)
                    best = q->id;
            }
            q++;
        } while (--n);
        Player_00452cc0* r;
        if (FindPlayerIndex_00452cc0(best) == 10)
            r = 0;
        else
            r = &g_game->players[FindPlayerIndex_00452cc0(best)];
        if (r != 0)
            r->data->flags |= 1;
    }
}

