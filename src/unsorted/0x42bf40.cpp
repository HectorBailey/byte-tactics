// Decompiled by deepseek-v4.1-flash, finished by GPT-6, edited by deepseek-v4.1. Names are provisional.
// Still 62.2% (4792 bytes against 4772) after deepseek-v4.1's pass.
// The dominant difference is the shared zero default argument. The whole
// original function contains ZERO `push 0` and 83 `push esi`: it sets
// `xor esi,esi` once at 0x42c0f0 after the objectname strcpy test and pushes
// esi for every default (the FUN_004c46c0 group, FUN_004c4760's two defaults,
// FUN_004c58a0's char* default). The FUN_004c4800 group instead rematerialises
// its default in eax (`xor eax,eax; push eax`, 7 times) alongside
// `mov eax,0x10000` for the 0x10000 defaults.
// Tried (all compile, all byte-identical at 62.2%):
//   - one `int def = 0;` at the top of the function used at all 81 zero-default
//     sites; MSVC5 constant-folds it and emits `push 0` at all 90 sites.
//   - the same local declared at point of use (after the objectname test, where
//     the original's xor esi,esi sits) instead of at the top.
//   - a second `unsigned int value = 0;` for the 7 FUN_004c4800 defaults
//     (original rematerialises those in eax): also folded to `push 0`.
// The original's 58a0 name/description defaults are literal `push 0` (the
// xor esi,esi comes after them), so those two sites are right either way.
// A single initialised local is NOT the shape that produces the hoisted esi
// zero; the next attempt should look for the non-constant source of that zero
// (or for the instruction that makes esi live across the funnels, e.g. our
// extra `mov [ebp+0x166],esi` / `mov [ebp+0x176],edi` stores near 0x42d11x,
// which the original does not have: it keeps those values in registers, stores
// them through edi at 0x42d0fb-0x42d107 and stores 0x166 earlier at 0x42d0a5).
// Other confirmed differences:
//  1. The Class_00438760 conversion temp does not sit in its own slot: the
//     original builds it at [esp+0x23] with its char member at [esp+0x2b]
//     (this+8), overlapping the head of the `char buf[100]` slot (which starts
//     at [esp+0x24]). Ours places it at [esp+0x20] with the byte at [esp+0x28].
//  2. The bail-out jumps differ (0x42d154 vs 0x42d167) only because our body is
//     20 bytes longer; fix the instruction differences first, the offsets then
//     follow.
// Remaining: local-byte placement, flag-operation scheduling, yard-map control
// flow and branch differences, plus the tail /3 sequence at 0x42d10a-0x42d11f.
#include <windows.h>
#include <stdlib.h>
#include <string.h>
class Class_004c4630 {
  public:
    char* FUN_004c4630(char* key);
};
class Class_00488e70 {
  public:
    void FUN_00488e70(char* category);
};
class Class_004402e0 {
  public:
    char data[32];
    Class_004402e0();
    ~Class_004402e0();
};
class Class_00440320 {
  public:
    void FUN_00440340(void* parser);
};
extern char* g_game;
short __stdcall FUN_00422e40(char* name);
void* __stdcall FUN_00440420(char* name);
char* __stdcall FUN_0049e5b0(char* name);
void* FUN_004d83b0(char* name, int size);

class Class_004c2ea0 {
  public:
    int field_0;
    void* current; // +0x4
    int field_8;
    Class_004c2ea0();
    ~Class_004c2ea0();
};

class Class_004c2f60 {
  public:
    int FUN_004c2f60(char* file);
};

class Class_004c3410 {
  public:
    int FUN_004c3410(char* name);
};

class Class_004c3240 {
  public:
    void FUN_004c3240();
};

class Class_004c48c0 {
  public:
    int FUN_004c48c0(char* dst, char* key, int size, char* def);
};

class Class_004c46c0 {
  public:
    int FUN_004c46c0(char* key, int def);
};

class Class_004c4800 {
  public:
    int* FUN_004c4800(int* out, char* key, int def);
};

class Class_004c4760 {
  public:
    float FUN_004c4760(char* key, int def, int def2);
};

class Class_00438760 {
  public:
    char value; // +0x0
    Class_00438760(char* text);
};

extern char DAT_005119b8[];
extern char DAT_00503ea0[];

void __stdcall FUN_004c58a0(void* parser, char* dst, char* key, int size, char* def);
void* __stdcall FUN_00488c50(char* text);

// FUNCTION: 0x42bf40
void __stdcall FUN_0042bf40(char* fbi_file, char* unitdef) {
    Class_004c2ea0 parser;
    int scratch;
    char buf[100];
    char weapon[128];
    char yard[1024];

    if (((Class_004c2f60*)&parser)->FUN_004c2f60(fbi_file)) {
        if (!((Class_004c3410*)&parser)->FUN_004c3410("UNITINFO")) {
            ((Class_004c3240*)&parser)->FUN_004c3240();
            goto FINISH;
        }
        {
            unsigned int value = 0;
            ((Class_004c48c0*)parser.current)
                ->FUN_004c48c0(unitdef + 0x20, "unitname", 0x20, DAT_005119b8);
            FUN_004c58a0(&parser, unitdef, "name", 0x20, 0);
            FUN_004c58a0(&parser, unitdef + 0x40, "description", 0x40, 0);
            ((Class_004c48c0*)parser.current)
                ->FUN_004c48c0(buf, "defaultmissiontype", 100, DAT_005119b8);
            Class_00438760 conv(buf);
            unitdef[0x230] = conv.value;
            ((Class_004c48c0*)parser.current)
                ->FUN_004c48c0(buf, "wpri_badTargetCategory", 100, DAT_00503ea0);
            *(void**)(unitdef + 0x231) = FUN_00488c50(buf);
            ((Class_004c48c0*)parser.current)
                ->FUN_004c48c0(buf, "wsec_badTargetCategory", 100, DAT_00503ea0);
            *(void**)(unitdef + 0x235) = FUN_00488c50(buf);
            ((Class_004c48c0*)parser.current)
                ->FUN_004c48c0(buf, "wspe_badTargetCategory", 100, DAT_00503ea0);
            *(void**)(unitdef + 0x239) = FUN_00488c50(buf);
            ((Class_004c48c0*)parser.current)
                ->FUN_004c48c0(buf, "noChaseCategory", 100, DAT_00503ea0);
            *(void**)(unitdef + 0x23d) = FUN_00488c50(buf);
            if (((Class_004c48c0*)parser.current)
                    ->FUN_004c48c0(unitdef + 0x80, "objectname", 0x20, DAT_005119b8) == 0) {
                strcpy(unitdef + 0x80, unitdef + 0x20);
            }
            int def = 0;
            scratch = ((Class_004c46c0*)parser.current)->FUN_004c46c0("buildcostenergy", def);
            *(float*)(unitdef + 0x186) = (float)scratch;
            scratch = ((Class_004c46c0*)parser.current)->FUN_004c46c0("buildcostmetal", def);
            *(float*)(unitdef + 0x18a) = (float)scratch;
            *(int*)(unitdef + 0x192) =
                *((Class_004c4800*)parser.current)->FUN_004c4800(&scratch, "maxvelocity", value);
            *(int*)(unitdef + 0x19a) =
                *((Class_004c4800*)parser.current)->FUN_004c4800(&scratch, "brakerate", value);
            *(int*)(unitdef + 0x19e) =
                *((Class_004c4800*)parser.current)->FUN_004c4800(&scratch, "acceleration", value);
            *(int*)(unitdef + 0x1a2) =
                *((Class_004c4800*)parser.current)->FUN_004c4800(&scratch, "bankscale", 0x10000);
            *(int*)(unitdef + 0x1a6) =
                *((Class_004c4800*)parser.current)->FUN_004c4800(&scratch, "pitchscale", value);
            *(int*)(unitdef + 0x1aa) = *((Class_004c4800*)parser.current)
                                            ->FUN_004c4800(&scratch, "damagemodifier", 0x10000);
            *(int*)(unitdef + 0x1ae) =
                *((Class_004c4800*)parser.current)
                     ->FUN_004c4800(&scratch, "moverate1", *(int*)(unitdef + 0x192) * 2);
            *(int*)(unitdef + 0x1b2) =
                *((Class_004c4800*)parser.current)
                     ->FUN_004c4800(&scratch, "moverate2", *(int*)(unitdef + 0x192) * 2);
            *(short*)(unitdef + 0x1ba) =
                (short)((Class_004c46c0*)parser.current)->FUN_004c46c0("turnrate", def);
            unitdef[0x22c] = (char)((Class_004c46c0*)parser.current)->FUN_004c46c0("waterline", def);
            unitdef[0x22a] =
                (char)((Class_004c46c0*)parser.current)->FUN_004c46c0("transportsize", def);
            unitdef[0x22b] =
                (char)((Class_004c46c0*)parser.current)->FUN_004c46c0("transportcapacity", def);
            *(float*)(unitdef + 0x1c2) =
                ((Class_004c4760*)parser.current)->FUN_004c4760("energymake", def, def);
            *(float*)(unitdef + 0x1c6) =
                ((Class_004c4760*)parser.current)->FUN_004c4760("energyuse", def, def);
            *(float*)(unitdef + 0x1ca) =
                ((Class_004c4760*)parser.current)->FUN_004c4760("metalmake", def, def);
            *(float*)(unitdef + 0x1ce) =
                ((Class_004c4760*)parser.current)->FUN_004c4760("extractsmetal", def, def);
            unitdef[0x22d] = (char)((Class_004c46c0*)parser.current)->FUN_004c46c0("makesmetal", def);
            *(float*)(unitdef + 0x1d2) =
                ((Class_004c4760*)parser.current)->FUN_004c4760("windgenerator", def, def);
            *(float*)(unitdef + 0x1d6) =
                ((Class_004c4760*)parser.current)->FUN_004c4760("tidalgenerator", def, def);
            *(float*)(unitdef + 0x1e2) =
                ((Class_004c4760*)parser.current)->FUN_004c4760("energystorage", def, def);
            *(float*)(unitdef + 0x1e6) =
                ((Class_004c4760*)parser.current)->FUN_004c4760("metalstorage", def, def);
            *(int*)(unitdef + 0x1ea) =
                ((Class_004c46c0*)parser.current)->FUN_004c46c0("buildtime", def);
            *(short*)(unitdef + 0x1fe) =
                (short)((Class_004c46c0*)parser.current)->FUN_004c46c0("workertime", def);
            *(short*)(unitdef + 0x200) =
                (short)((Class_004c46c0*)parser.current)->FUN_004c46c0("healtime", def);
            *(int*)(unitdef + 0x1fa) =
                ((Class_004c46c0*)parser.current)->FUN_004c46c0("maxdamage", def);
            *(short*)(unitdef + 0x202) =
                (short)((Class_004c46c0*)parser.current)->FUN_004c46c0("sightdistance", def);
            *(short*)(unitdef + 0x204) =
                (short)((Class_004c46c0*)parser.current)->FUN_004c46c0("radardistance", def);
            *(short*)(unitdef + 0x206) =
                (short)((Class_004c46c0*)parser.current)->FUN_004c46c0("sonardistance", def);
            *(short*)(unitdef + 0x20a) =
                (short)((Class_004c46c0*)parser.current)->FUN_004c46c0("radardistancejam", def);
            *(short*)(unitdef + 0x20c) =
                (short)((Class_004c46c0*)parser.current)->FUN_004c46c0("sonardistancejam", def);
            unitdef[0x22f] = (char)((Class_004c46c0*)parser.current)->FUN_004c46c0("bmcode", def);
            unsigned int value2;
            int number;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("standingmoveorder", 2);
            *(unsigned int*)(unitdef + 0x241) =
                (value ^ *(unsigned int*)(unitdef + 0x241)) & 3 ^ *(unsigned int*)(unitdef + 0x241);
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("standingfireorder", 2);
            *(unsigned int*)(unitdef + 0x241) =
                (value & 3) << 2 | *(unsigned int*)(unitdef + 0x241) & 0xfffffff3;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("init_cloaked", def);
            *(unsigned int*)(unitdef + 0x241) =
                (value & 1) << 4 | *(unsigned int*)(unitdef + 0x241) & 0xffffffef;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("downloadable", def);
            *(unsigned int*)(unitdef + 0x241) =
                (value & 1) << 5 | *(unsigned int*)(unitdef + 0x241) & 0xffffffdf;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("builder", def);
            *(unsigned int*)(unitdef + 0x241) =
                (value & 1) << 6 | *(unsigned int*)(unitdef + 0x241) & 0xffffffbf;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("stealth", def);
            *(unsigned int*)(unitdef + 0x241) =
                (value & 1) << 8 | *(unsigned int*)(unitdef + 0x241) & 0xfffffeff;
            scratch = ((Class_004c46c0*)parser.current)->FUN_004c46c0("cloakcost", def);
            *(float*)(unitdef + 0x1da) = (float)scratch;
            int cloakDefault = (int)*(float*)(unitdef + 0x1da);
            scratch =
                ((Class_004c46c0*)parser.current)->FUN_004c46c0("cloakcostmoving", cloakDefault);
            *(float*)(unitdef + 0x1de) = (float)scratch;
            number = ((Class_004c46c0*)parser.current)->FUN_004c46c0("mincloakdistance", def);
            *(short*)(unitdef + 0x208) = (short)number;
            number = ((Class_004c46c0*)parser.current)->FUN_004c46c0("buildangle", def);
            *(short*)(unitdef + 0x210) = (short)number;
            number = ((Class_004c46c0*)parser.current)->FUN_004c46c0("builddistance", def);
            *(short*)(unitdef + 0x212) = (short)number;
            number = ((Class_004c46c0*)parser.current)->FUN_004c46c0("sortbias", def);
            *(short*)(unitdef + 0x21a) = (short)number;
            number = ((Class_004c46c0*)parser.current)->FUN_004c46c0("cruisealt", def);
            *(short*)(unitdef + 0x21c) = (short)number;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("zbuffer", def);
            *(unsigned int*)(unitdef + 0x241) =
                (value & 1) << 7 | *(unsigned int*)(unitdef + 0x241) & 0xffffff7f;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("isairbase", def);
            *(unsigned int*)(unitdef + 0x241) =
                (value & 1) << 9 | *(unsigned int*)(unitdef + 0x241) & 0xfffffdff;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("istargetingupgrade", def);
            *(unsigned int*)(unitdef + 0x241) =
                (value & 1) << 10 | *(unsigned int*)(unitdef + 0x241) & 0xfffffbff;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("teleporter", def);
            *(unsigned int*)(unitdef + 0x241) =
                *(unsigned int*)(unitdef + 0x241) & 0xffffdfff | (value & 1) << 0xd;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("hidedamage", def);
            *(unsigned int*)(unitdef + 0x241) =
                *(unsigned int*)(unitdef + 0x241) & 0xffffbfff | (value & 1) << 0xe;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("shootme", def);
            *(unsigned int*)(unitdef + 0x241) =
                *(unsigned int*)(unitdef + 0x241) & 0xffff7fff | (value & 1) << 0xf;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("armoredstate", def);
            *(unsigned int*)(unitdef + 0x241) =
                (value & 1) << 0x11 | *(unsigned int*)(unitdef + 0x241) & 0xfffdffff;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("activatewhenbuilt", def);
            *(unsigned int*)(unitdef + 0x241) =
                (value & 1) << 0x12 | *(unsigned int*)(unitdef + 0x241) & 0xfffbffff;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("canfly", def);
            *(unsigned int*)(unitdef + 0x241) =
                (value & 1) << 0xb | *(unsigned int*)(unitdef + 0x241) & 0xfffff7ff;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("canhover", def);
            *(unsigned int*)(unitdef + 0x241) =
                *(unsigned int*)(unitdef + 0x241) & 0xffffefff | (value & 1) << 0xc;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("upright", def);
            *(unsigned int*)(unitdef + 0x241) =
                (value & 1) << 0x14 | *(unsigned int*)(unitdef + 0x241) & 0xffefffff;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("floater", def);
            *(unsigned int*)(unitdef + 0x241) =
                (value & 1) << 0x13 | *(unsigned int*)(unitdef + 0x241) & 0xfff7ffff;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("amphibious", def);
            *(unsigned int*)(unitdef + 0x241) =
                (value & 1) << 0x15 | *(unsigned int*)(unitdef + 0x241) & 0xffdfffff;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("isfeature", def);
            *(unsigned int*)(unitdef + 0x241) =
                (value & 1) << 0x18 | *(unsigned int*)(unitdef + 0x241) & 0xfeffffff;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("noshadow", def);
            *(unsigned int*)(unitdef + 0x241) =
                (value & 1) << 0x19 | *(unsigned int*)(unitdef + 0x241) & 0xfdffffff;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("immunetoparalyzer", def);
            *(unsigned int*)(unitdef + 0x241) =
                (value & 1) << 0x1a | *(unsigned int*)(unitdef + 0x241) & 0xfbffffff;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("hoverattack", def);
            *(unsigned int*)(unitdef + 0x241) =
                (value & 1) << 0x1b | *(unsigned int*)(unitdef + 0x241) & 0xf7ffffff;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("antiweapons", def);
            *(unsigned int*)(unitdef + 0x241) =
                *(unsigned int*)(unitdef + 0x241) & 0xdfffffff | (value & 1) << 0x1d;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("digger", def);
            *(unsigned int*)(unitdef + 0x241) =
                *(unsigned int*)(unitdef + 0x241) & 0xbfffffff | (value & 1) << 0x1e;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("onoffable", def);
            *(unsigned int*)(unitdef + 0x245) =
                (value & 1) << 2 | *(unsigned int*)(unitdef + 0x245) & 0xfffffffb;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("mobilestandorders", def);
            *(unsigned int*)(unitdef + 0x245) =
                (value ^ *(unsigned int*)(unitdef + 0x245)) & 1 ^ *(unsigned int*)(unitdef + 0x245);
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("firestandorders", def);
            *(unsigned int*)(unitdef + 0x245) =
                (value & 1) << 1 | *(unsigned int*)(unitdef + 0x245) & 0xfffffffd;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("canstop", def);
            *(unsigned int*)(unitdef + 0x245) =
                (value & 1) << 3 | *(unsigned int*)(unitdef + 0x245) & 0xfffffff7;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("canattack", def);
            *(unsigned int*)(unitdef + 0x245) =
                (value & 1) << 4 | *(unsigned int*)(unitdef + 0x245) & 0xffffffef;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("canguard", def);
            *(unsigned int*)(unitdef + 0x245) =
                (value & 1) << 5 | *(unsigned int*)(unitdef + 0x245) & 0xffffffdf;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("canpatrol", def);
            *(unsigned int*)(unitdef + 0x245) =
                (value & 1) << 6 | *(unsigned int*)(unitdef + 0x245) & 0xffffffbf;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("canmove", def);
            *(unsigned int*)(unitdef + 0x245) =
                (value & 1) << 7 | *(unsigned int*)(unitdef + 0x245) & 0xffffff7f;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("canload", def);
            *(unsigned int*)(unitdef + 0x245) =
                (value & 1) << 8 | *(unsigned int*)(unitdef + 0x245) & 0xfffffeff;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("canreclamate", def);
            *(unsigned int*)(unitdef + 0x245) =
                (value & 1) << 10 | *(unsigned int*)(unitdef + 0x245) & 0xfffffbff;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("canresurrect", def);
            *(unsigned int*)(unitdef + 0x245) = (*(unsigned int*)(unitdef + 0x245) & 0x400) >> 1 |
                                                (value & 1) << 0xb |
                                                *(unsigned int*)(unitdef + 0x245) & 0xfffff5ff;
            value2 = ((Class_004c46c0*)parser.current)->FUN_004c46c0("cancapture", def);
            value = *(unsigned int*)(unitdef + 0x245);
            value2 = (value2 & 1) << 0xc;
            *(unsigned int*)(unitdef + 0x245) = value & 0xffffefff | value2;
            *(unsigned int*)(unitdef + 0x245) = value & 0xffffcfff | value2 |
                                                (unsigned int)(0.0 < *(float*)(unitdef + 0x1da))
                                                    << 0xd;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("candgun", def);
            *(unsigned int*)(unitdef + 0x245) =
                *(unsigned int*)(unitdef + 0x245) & 0xffffbfff | (value & 1) << 0xe;
            number = ((Class_004c46c0*)parser.current)->FUN_004c46c0("maneuverleashlength", def);
            *(short*)(unitdef + 0x214) = (short)number;
            number = ((Class_004c46c0*)parser.current)->FUN_004c46c0("attackrunlength", def);
            *(short*)(unitdef + 0x216) = (short)number;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("kamikaze", def);
            *(unsigned int*)(unitdef + 0x241) =
                *(unsigned int*)(unitdef + 0x241) & 0xefffffff | (value & 1) << 0x1c;
            number = ((Class_004c46c0*)parser.current)->FUN_004c46c0("kamikazedistance", def);
            *(short*)(unitdef + 0x218) = (short)number;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("norestrict", def);
            *(unsigned int*)(unitdef + 0x245) =
                *(unsigned int*)(unitdef + 0x245) & 0xffff7fff | (value & 1) << 0xf;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("showplayername", def);
            *(unsigned int*)(unitdef + 0x245) =
                (value & 1) << 0x11 | *(unsigned int*)(unitdef + 0x245) & 0xfffdffff;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("commander", def);
            *(unsigned int*)(unitdef + 0x245) =
                (value & 1) << 0x12 | *(unsigned int*)(unitdef + 0x245) & 0xfffbffff;
            value = ((Class_004c46c0*)parser.current)->FUN_004c46c0("cantbetransported", def);
            *(unsigned int*)(unitdef + 0x245) =
                (value & 1) << 0x13 | *(unsigned int*)(unitdef + 0x245) & 0xfff7ffff;

            char* countdown =
                ((Class_004c4630*)parser.current)->FUN_004c4630("selfdestructcountdown");
            unsigned int* flags = (unsigned int*)(unitdef + 0x245);
            if (countdown == 0)
                *flags = (*flags & 0xffdfffff) | 0x500000;
            else
                *flags = (*flags & 0xff8fffff) | ((unsigned int)atoi(countdown) & 7) << 20;
            ((Class_004c48c0*)parser.current)->FUN_004c48c0(buf, "category", 100, DAT_005119b8);
            ((Class_00488e70*)unitdef)->FUN_00488e70(buf);
            int sound = 0;
            if (((Class_004c48c0*)parser.current)
                    ->FUN_004c48c0(buf, "soundcategory", 100, DAT_005119b8)) {
                while (sound < *(int*)(g_game + 0x37e17)) {
                    if (_strcmpi(*(char**)(g_game + 0x37e13) + sound * 0x160, buf) == 0)
                        goto SOUND_FOUND;
                    sound++;
                }
                sound = atoi(buf);
            }
        SOUND_FOUND:
            *(short*)(unitdef + 0x20e) = (short)sound;
            *(short*)(unitdef + 0x1bc) = -1;
            if (((Class_004c48c0*)parser.current)->FUN_004c48c0(buf, "corpse", 100, DAT_005119b8))
                *(short*)(unitdef + 0x1bc) = FUN_00422e40(buf);
            *(void**)(unitdef + 0x1b6) = 0;
            if (((Class_004c48c0*)parser.current)
                    ->FUN_004c48c0(buf, "movementclass", 100, DAT_005119b8))
                *(void**)(unitdef + 0x1b6) = FUN_00440420(buf);
            Class_004402e0 movement;
            char* move = *(char**)(unitdef + 0x1b6);
            if (move == 0) {
                ((Class_00440320*)&movement)->FUN_00440340(&parser);
                move = (char*)&movement;
            }
            *(short*)(unitdef + 0x14a) = *(short*)(move + 4);
            *(short*)(unitdef + 0x14c) = *(short*)(move + 6);
            *(short*)(unitdef + 0x1be) = *(short*)(move + 8);
            *(short*)(unitdef + 0x1c0) = *(short*)(move + 10);
            unitdef[0x228] = move[12];
            unitdef[0x229] = move[14];
            *(int*)(unitdef + 0x196) = (int)(((__int64)*(int*)(unitdef + 0x192) << 16) /
                                             (((unsigned char)unitdef[0x228] + 1) * 0x10000));
            char* defaultWeapon = g_game + 0x2cf3;
            ((Class_004c48c0*)parser.current)->FUN_004c48c0(weapon, "weapon1", 128, DAT_005119b8);
            char* weapon1 = FUN_0049e5b0(weapon);
            *(char**)(unitdef + 0x1ee) = weapon1 ? weapon1 : defaultWeapon;
            ((Class_004c48c0*)parser.current)->FUN_004c48c0(weapon, "weapon2", 128, DAT_005119b8);
            char* weapon2 = FUN_0049e5b0(weapon);
            *(char**)(unitdef + 0x1f2) = weapon2 ? weapon2 : defaultWeapon;
            ((Class_004c48c0*)parser.current)->FUN_004c48c0(weapon, "weapon3", 128, DAT_005119b8);
            char* weapon3 = FUN_0049e5b0(weapon);
            *(char**)(unitdef + 0x1f6) = weapon3 ? weapon3 : defaultWeapon;
            ((Class_004c48c0*)parser.current)->FUN_004c48c0(weapon, "explodeas", 128, DAT_005119b8);
            char* explodeas = FUN_0049e5b0(weapon);
            *(char**)(unitdef + 0x220) = explodeas ? explodeas : defaultWeapon;
            ((Class_004c48c0*)parser.current)
                ->FUN_004c48c0(weapon, "selfdestructas", 128, DAT_005119b8);
            char* selfdestructas = FUN_0049e5b0(weapon);
            *(char**)(unitdef + 0x224) = selfdestructas ? selfdestructas : defaultWeapon;
            if (*(char**)(unitdef + 0x1ee) == defaultWeapon &&
                *(char**)(unitdef + 0x1f2) == defaultWeapon &&
                *(char**)(unitdef + 0x1f6) == defaultWeapon)
                *(unsigned int*)(unitdef + 0x241) &= ~0x10000;
            else
                *(unsigned int*)(unitdef + 0x241) |= 0x10000;
            *(void**)(unitdef + 0x14e) = 0;
            if (unitdef[0x22f] == 0) {
                ((Class_004c48c0*)parser.current)
                    ->FUN_004c48c0(yard, "YardMap", 1024, DAT_005119b8);
                *(char**)(unitdef + 0x14e) = (char*)FUN_004d83b0(
                    "BUILDING YARD", *(short*)(unitdef + 0x14a) * *(short*)(unitdef + 0x14c));
                int cell = 0;
                char* cursor = yard;
                for (int y = 0; y < *(short*)(unitdef + 0x14c); y++) {
                    for (int x = 0; x < *(short*)(unitdef + 0x14a);) {
                        switch (*cursor) {
                        case '.':
                            (*(char**)(unitdef + 0x14e))[cell] = 0x0;
                            break;
                        case 'C':
                            (*(char**)(unitdef + 0x14e))[cell] = 0x35;
                            break;
                        case 'G':
                            (*(char**)(unitdef + 0x14e))[cell] = 0x8f;
                            break;
                        case 'O':
                            (*(char**)(unitdef + 0x14e))[cell] = 0x2b;
                            break;
                        case 'Y':
                            (*(char**)(unitdef + 0x14e))[cell] = 0x31;
                            break;
                        case 'c':
                            (*(char**)(unitdef + 0x14e))[cell] = 0x2d;
                            break;
                        case 'f':
                            (*(char**)(unitdef + 0x14e))[cell] = 0x6f;
                            break;
                        case 'o':
                            (*(char**)(unitdef + 0x14e))[cell] = 0x2f;
                            break;
                        case 'w':
                            (*(char**)(unitdef + 0x14e))[cell] = 0x37;
                            break;
                        case 'y':
                            (*(char**)(unitdef + 0x14e))[cell] = 0x29;
                            break;
                        default:
                            cursor++;
                            continue;
                        }
                        x++;
                        cell++;
                        if (cursor[1] != 0)
                            cursor++;
                    }
                }
            }
            *(int*)(unitdef + 0x15e) = (*(short*)(unitdef + 0x14a) * -0x100000) / 2;
            *(int*)(unitdef + 0x166) = (*(short*)(unitdef + 0x14c) * -0x100000) / 2;
            *(int*)(unitdef + 0x16a) = (*(short*)(unitdef + 0x14a) << 20) / 2;
            *(int*)(unitdef + 0x172) = (*(short*)(unitdef + 0x14c) << 20) / 2;
            *(int*)(unitdef + 0x176) = *(int*)(unitdef + 0x16a) - *(int*)(unitdef + 0x15e);
            *(int*)(unitdef + 0x17a) = *(int*)(unitdef + 0x16e) - *(int*)(unitdef + 0x162);
            *(int*)(unitdef + 0x17e) = *(int*)(unitdef + 0x172) - *(int*)(unitdef + 0x166);
            *(int*)(unitdef + 0x182) = (*(int*)(unitdef + 0x17e) + *(int*)(unitdef + 0x176)) / 3;
            ((Class_004c3240*)&parser)->FUN_004c3240();
            if ((*(unsigned int*)(unitdef + 0x245) & 0x2000) && *(short*)(unitdef + 0x208) == 0)
                *(short*)(unitdef + 0x208) = 80;

        }
    }
FINISH:;
}
