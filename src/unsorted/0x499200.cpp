// Decompiled by deepseek-v4.1-flash, finished by space-bunny-free, finished by GPT-6.1-sol. Names are provisional.
// Header sweeps (128 C and 768 C/C++ combinations) found no improvement.
// PARTIAL, 94.7% (1656 of 1655 bytes, instruction-text score). Every branch,
// field offset, call target, stack slot and basic-block boundary from
// 0x499200 to 0x499876 now agrees with the original, and the instruction
// COUNT of the last block matches exactly. What is left is register ROLES in
// the last block, and our block is one byte longer because of them.
// SOLVED this session (91.7% -> 94.7%), the thing two earlier sessions gave up
// on: the constant 4 that lives in edi from 0x499603 to 0x49986b.
// - A local of any scalar type holding 4 is folded by MSVC 5 into an immediate
//   at every use (`push 4`, `or word [m],4`). Every spelling tried failed
//   (int/unsigned/short/char/const/register/static/enum/4.0f/`four = four`,
//   `four += 0`, dead extra uses, a dead __inline helper, an __inline helper
//   taking the literal as a parameter, ...).
// - What works: give it to a 4-byte AGGREGATE WITH A CONSTRUCTOR, pass the
//   aggregate BY VALUE to FUN_004ce690, and read the field for the `|=`.
//       struct Four_00499200 { int v; Four_00499200() : v(4) {} };
//       Four_00499200 four;
//       g_game->field_10->FUN_004ce690(four);      // -> push edi
//       g_game->field_2a44.value |= four.v;        // -> or word [m], di
//   That is the guide's item 29 (a constructor materialises a constant where a
//   field assignment cannot) plus the guide's "a constant loaded into a
//   register and pushed" entry (a 4-byte struct passed by value is not
//   `push 4`). A union with the same constructor, a 1-argument constructor, a
//   4-argument constructor, a ctor whose body assigns instead of an init list,
//   two 16-bit members, an __inline accessor and a struct member holding the
//   aggregate all give the identical 94.7%, so the shape of the aggregate does
//   not matter, only that it is an aggregate built by a constructor.
// - The `|= four.v` statement is written TWICE: once inside the inner
//   `if (net->FUN_00435c00(a) != 0)` in the first arm, once at the end of the
//   else arm. MSVC then tail-merges the two copies into the single
//   `mov eax,[g]; or word [m],di` at 0x49982a that both arms reach, which is
//   the original's shape. Written once after the inner if/else instead, MSVC
//   puts the `|=` in the first arm only and jumps the else arm over it
//   (93.9%, and semantically wrong). Written once after the OUTER if/else it
//   also runs on the inner-if-false path, which the original does not.
// Still differs (all register roles in the 0x4997a0..0x49986b block, each one
// step round the eax -> ecx -> edx cycle from the original):
// - the original builds g_game in edx where we use eax (the 6-byte form versus
//   the 5-byte `mov eax,[m]`; that is the single extra byte), builds the
//   FUN_004a9660 field_519 argument in eax where we use ecx, the FUN_004ab400
//   second argument in ecx where we use edx, and the FUN_00435a20 argument in
//   edx (with `this` in ecx loaded first) where we use ecx (loaded after it).
// - the tail uses eax/ecx/edx for field_391f1, field_391f5 and field_10 where
//   we use edx/eax/ecx. Same one-step rotation, consistently.
// - This is a scratch-register ROTATION (the guide's "rotated by one across a
//   loop" entry), not a shape problem: the block's instruction count and every
//   branch target already agree. Not moved by naming a copy of four.v, by
//   naming g_game in the else arm (that costs 15 points, the named pointer
//   stays live), or by any of the aggregate shapes above.
// Earlier sessions' notes, kept because they record what does NOT work:
// - +0x2cc6 is NOT involved, but +0x2a44 and +0x3923b are bitfield unions now.
//   +0x2a44: `|= 8` is a byte RMW and `|= <const>` is a WORD RMW, so the field
//   is `union { unsigned short value; struct { ...b3:1... } bits; }` and the
//   two statements are `bits.b3 = 1;` and `value |= four;`.
//   +0x3923b: the original is `mov ax,word[m]; test al,0x14`, a 16-bit load
//   followed by an 8-bit test, which is what a `||` of two bitfields in a
//   16-bit storage unit gives: `bits.b2 || bits.b4`. A plain `unsigned short`
//   local folds to `test byte ptr [m],0x14` and loses the load. This was the
//   only scoring change of the session (+0.5).
// What still differs, and everything tried (the edi range is now solved, see
// above; the list is what earlier sessions proved does NOT work):
// - a literal-4 local of any type is folded by MSVC 5 into an immediate at
//   every use. Already tried, all identical at 91.7%: int/unsigned/short/
//   unsigned short/char/const/register/`four = four`, `four += 0`, `four *= 1`,
//   `four ^= 0`, `four2 = four`, `if (four == 4)`, a dead extra `|=`/`&=` use,
//   `static int four = 4`, an enum constant, `4.0f` and `4.0` initialisers, a
//   bitfield union local, `int four_tab[1] = {4}`, `int four; four = 4;`,
//   `unsigned short four = 4;`, the declaration at the top of the body, a dead
//   __inline helper to change the /Ob2 budget, and two __inline helpers
//   (`Push(n)` / `Or(n)` and one `Mode(n)`) called with the literal 4 so the
//   constant would arrive through a parameter. Do not retry these.
// - the register-role swaps listed above were called knock-on effects of the
//   missing edi live range. They were, in part: fixing the edi range moved the
//   block from 91.7% to 94.7% and made its instruction count exact, but the
//   rotation is still one step out.
// Main-loop frame handler. Copies the 24-byte view/input block off g_game,
// feeds it to the camera update, then runs the order/selection state machine
// off the flags byte at +0x2cc6 and the mouse message stored in the block.
// Advances the frame queues and, on the network/skirmish paths, flips the
// end-of-frame hooks.
#include <stdlib.h>

#pragma pack(push, 1)

struct View_00499200 {
    int x;                             // +0x0
    int y;                             // +0x4
    int field_8;                       // +0x8
    int field_c;                       // +0xc
    int msg;                           // +0x10
    int field_14;                      // +0x14
};

struct Struct_00499200_531 {
    int unknown_0;
    int value;                         // +0x4
};

struct BitFlags16_00499200 {
    unsigned short b0:1;
    unsigned short b1:1;
    unsigned short b2:1;
    unsigned short b3:1;
    unsigned short b4:1;
    unsigned short b5:1;
    unsigned short b6:1;
    unsigned short b7:1;
    unsigned short b8:1;
    unsigned short b9:1;
    unsigned short b10:1;
    unsigned short b11:1;
    unsigned short b12:1;
    unsigned short b13:1;
    unsigned short b14:1;
    unsigned short b15:1;
};

union Flags16_00499200 {
    unsigned short value;
    BitFlags16_00499200 bits;
};

class Class_00435100 {
public:
    int FUN_00435100();
};

class Class_00435c50 {
public:
    int FUN_00435c50();
};

class Class_004352b0 {
public:
    char* FUN_004352b0();
};

class Class_00435110 {
public:
    void FUN_00435110(void* p);
};

class Class_00435c00 {
public:
    int FUN_00435c00(int a);
};

class Class_00435a20 {
public:
    void FUN_00435a20(void* p);
};

// A 4-byte aggregate, so MSVC materialises the constant 4 into a register
// (edi) instead of folding it into each use as an immediate.
struct Four_00499200 {
    int v;
    Four_00499200() : v(4) {}
};

class Class_004ce690 {
public:
    void FUN_004ce690(Four_00499200 a);
};

struct Game_00499200 {
    char unknown_0[0x10];
    Class_004ce690* field_10;          // +0x10
    char unknown_14[0x519 - 0x14];
    char field_519[0x18];              // +0x519
    Struct_00499200_531* field_531;    // +0x531
    char unknown_535[0x29a0 - 0x535];
    char* field_29a0;                  // +0x29a0
    char unknown_29a4[0x2a3c - 0x29a4];
    unsigned short field_2a3c;         // +0x2a3c
    char unknown_2a3e[0x2a44 - 0x2a3e];
    Flags16_00499200 field_2a44;       // +0x2a44
    char unknown_2a46[0x2c76 - 0x2a46];
    View_00499200 view;                // +0x2c76
    char unknown_2c8e[0x2c92 - 0x2c8e];
    int field_2c92;                    // +0x2c92
    int field_2c96;                    // +0x2c96
    int field_2c9a;                    // +0x2c9a
    int field_2c9e;                    // +0x2c9e
    int field_2ca2;                    // +0x2ca2
    int field_2ca6;                    // +0x2ca6
    char unknown_2caa[0x2cac - 0x2caa];
    short field_2cac;                  // +0x2cac
    char unknown_2cae[0x2cb0 - 0x2cae];
    short field_2cb0;                  // +0x2cb0
    char unknown_2cb2[0x2cb4 - 0x2cb2];
    short field_2cb4;                  // +0x2cb4
    int field_2cb6;                    // +0x2cb6
    unsigned short field_2cba;         // +0x2cba
    char unknown_2cbc[0x2cbe - 0x2cbc];
    signed char selected;              // +0x2cbe
    char unknown_2cbf[0x2cc3 - 0x2cbf];
    unsigned char orderMode;           // +0x2cc3
    char unknown_2cc4[0x2cc6 - 0x2cc4];
    unsigned char flags_2cc6;          // +0x2cc6
    char unknown_2cc7[0x2cdf - 0x2cc7];
    int field_2cdf;                    // +0x2cdf
    char unknown_2ce3[0x14357 - 0x2ce3];
    char* units;                       // +0x14357
    char unknown_1435b[0x1487f - 0x1435b];
    void* table[21];                   // +0x1487f
    char unknown_148d3[0x37e9c - 0x148d3];
    unsigned short field_37e9c;        // +0x37e9c
    char unknown_37e9e[0x37efa - 0x37e9e];
    int field_37efa;                   // +0x37efa
    char unknown_37efe[0x391e9 - 0x37efe];
    Class_00435100* net;               // +0x391e9
    char unknown_391ed[0x391f1 - 0x391ed];
    int field_391f1;                   // +0x391f1
    void (*field_391f5)(void);         // +0x391f5
    char unknown_391f9[0x3923b - 0x391f9];
    Flags16_00499200 field_3923b;      // +0x3923b
    char unknown_3923d[0x39249 - 0x3923d];
    int field_39249;                   // +0x39249
};
#pragma pack(pop)

extern Game_00499200* g_game;

void FUN_004197d0();
void FUN_0041c180();
void FUN_0041cd50();
void FUN_0041d0f0();
void __stdcall FUN_0041d9f0(int a);
void FUN_004257a0();
int FUN_004572a0();
void FUN_00463c80();
void FUN_0047a760();
void FUN_0048bd00();
int __stdcall FUN_0048c390(void* p);
unsigned short __stdcall FUN_0048cd80();
int __stdcall FUN_0048d220(char mode);
void FUN_00491b60();
void __stdcall FUN_00491d70(int a);
void FUN_00496790();
void __stdcall FUN_00498da0(View_00499200* p);
void __stdcall FUN_00498f70(View_00499200* p);
void __stdcall FUN_00499100(View_00499200* p);
int __stdcall FUN_0049fe60(int value, char* name);
void __stdcall FUN_004a6a40(void* obj, int index);
void __stdcall FUN_004a9660(void* a);
void __stdcall FUN_004ab400(void* a, void* b);
void __stdcall FUN_004b4fd0(void* a, int b);
int FUN_004b6340();
void FUN_004c1a40();
int __stdcall FUN_004c1b80(int a);
void FUN_00499880();
void FUN_00496bb0();
void FUN_004578f0();

// FUNCTION: 0x499200
void FUN_00499200(void)
{
    View_00499200 view = g_game->view;
    FUN_00498da0(&view);

    unsigned char flags = g_game->flags_2cc6;
    if ((flags & 2) != 0 && g_game->orderMode == 0xe) {
        FUN_004197d0();
    } else if ((flags & 2) == 0 && (flags & 1) == 0) {
        if (g_game->selected != 0x13) {
            g_game->selected = 0x13;
            FUN_004ab400(g_game->field_519, g_game->table[0x13]);
        }
    } else {
        g_game->field_2cba = FUN_0048cd80();
        int idx = FUN_0048d220(g_game->orderMode);
        if (g_game->selected != idx) {
            g_game->selected = (signed char)idx;
            FUN_004ab400(g_game->field_519, g_game->table[idx]);
        }
    }

    FUN_0041c180();
    if ((g_game->flags_2cc6 & 0x20) != 0) {
        if (FUN_004c1b80(0xf9) == 0) {
            g_game->orderMode = 1;
            g_game->flags_2cc6 &= 0xdf;
            int index = FUN_0049fe60(g_game->field_531->value, "STOP");
            if (index != -1) {
                FUN_004a6a40(g_game->field_519, index);
            }
        }
    }

    flags = g_game->flags_2cc6;
    if ((flags & 0x10) != 0) {
        if (g_game->field_37efa == 0) {
            if (view.msg == 0x205) {
                g_game->flags_2cc6 = flags & 0xef;
            } else {
                FUN_0041d0f0();
            }
        } else {
            if (view.msg == 0x202) {
                g_game->flags_2cc6 = flags & 0xef;
            } else {
                FUN_0041d0f0();
            }
        }
    } else if (g_game->field_2cdf != 0) {
        FUN_0041cd50();
    } else if (view.msg == 0x204) {
        FUN_00499100(&view);
    } else if (g_game->orderMode != 1) {
        if (view.msg == 0x201) {
            FUN_00498f70(&view);
        }
    } else if ((flags & 8) != 0) {
        if (view.msg == 0x202) {
            g_game->flags_2cc6 = flags & 0xf7;
            int dx = g_game->field_2c92 - g_game->field_2c9e;
            int dz = g_game->field_2c9a - g_game->field_2ca6;
            dx = abs(dx);
            dz = abs(dz);
            int now = FUN_004b6340();
            if (g_game->field_2cb6 + 0x19 > now && dx < 0x20 && dz < 0x20) {
                FUN_00498f70(&view);
            } else if (FUN_0048c390(&view) == 0) {
                FUN_0048bd00();
                FUN_00491d70(1);
            }
        } else {
            g_game->field_2c9e = g_game->field_2cac;
            g_game->field_2ca2 = g_game->field_2cb0;
            g_game->field_2ca6 = g_game->field_2cb4;
        }
    } else if (view.msg == 0x201) {
        if ((flags & 2) != 0) {
            g_game->flags_2cc6 = flags | 8;
            g_game->field_2cb6 = FUN_004b6340();
            g_game->field_2c92 = g_game->field_2cac;
            g_game->field_2c96 = g_game->field_2cb0;
            g_game->field_2c9a = g_game->field_2cb4;
            g_game->field_2c9e = g_game->field_2cac;
            g_game->field_2ca2 = g_game->field_2cb0;
            g_game->field_2ca6 = g_game->field_2cb4;
            if (g_game->selected != 0x13) {
                g_game->selected = 0x13;
                FUN_004ab400(g_game->field_519, g_game->table[0x13]);
            }
        } else if (g_game->field_37efa == 1) {
            if ((flags & 1) != 0) {
                g_game->flags_2cc6 = flags | 0x10;
                if (g_game->selected != 0x13) {
                    g_game->selected = 0x13;
                    FUN_004ab400(g_game->field_519, g_game->table[0x13]);
                }
            }
        } else if ((flags & 1) != 0) {
            FUN_00498f70(&view);
        }
    }

    FUN_00496790();
    {
        unsigned short unit = g_game->field_37e9c;
        if (unit != 0 && *(short*)(g_game->units + unit * 0x118 + 0xa6) == 0) {
            FUN_00491d70(0);
        }
    }

    Four_00499200 four;
    if (g_game->field_3923b.bits.b2 || g_game->field_3923b.bits.b4) {
        if (g_game->net->FUN_00435100() != 3 ||
            (((Class_00435100*)g_game->net)->FUN_00435100() == 3 &&
             FUN_004572a0() != 0)) {
            if (g_game->selected != 0x13) {
                g_game->selected = 0x13;
                FUN_004ab400(g_game->field_519, g_game->table[0x13]);
            }
            FUN_00491d70(1);
            FUN_004a9660(g_game->field_519);
            if (g_game->net->FUN_00435100() == 3) {
                FUN_00463c80();
                FUN_00496790();
            }
            FUN_00491b60();
            FUN_004c1a40();
            g_game->field_10->FUN_004ce690(four);
            g_game->field_391f1 = 7;
            g_game->field_391f5 = FUN_00499880;
            FUN_004b4fd0(FUN_004578f0, 0);
            FUN_0041d9f0(0);
        }
    }

    if (g_game->field_39249 != 0) {
        if (g_game->net->FUN_00435100() == 1) {
            FUN_00491b60();
            FUN_00491d70(1);
            FUN_004a9660(g_game->field_519);
            FUN_004257a0();
            int a = ((Class_00435c50*)g_game->net)->FUN_00435c50();
            char* b = ((Class_004352b0*)g_game->net)->FUN_004352b0();
            ((Class_00435110*)g_game->net)->FUN_00435110(b);
            if (((Class_00435c00*)g_game->net)->FUN_00435c00(a) != 0) {
                g_game->field_2a44.bits.b3 = 1;
                g_game->field_2a44.value |= four.v;
            }
        } else {
            unsigned int saved = g_game->field_2a3c;
            FUN_00491b60();
            FUN_00491d70(1);
            FUN_004a9660(g_game->field_519);
            FUN_004257a0();
            if (g_game->selected != 0x14) {
                g_game->selected = 0x14;
                FUN_004ab400(g_game->field_519, g_game->table[0x14]);
            }
            g_game->field_2a3c = saved;
            ((Class_00435a20*)g_game->net)->FUN_00435a20(g_game->field_29a0 + 0x11c);
            FUN_0047a760();
            g_game->field_2a44.value |= four.v;
        }
        g_game->field_391f1 = 2;
        g_game->field_391f5 = FUN_00496bb0;
        FUN_004b4fd0(FUN_004578f0, 0);
        g_game->field_10->FUN_004ce690(four);
    }
}
