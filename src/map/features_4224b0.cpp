// Decompiled by Claude Opus 5.5. Names are provisional.
// Stays in its own file: in features.cpp the merged file's symbol ids schedule
// the `name` load after the strncpy setup instead of before it
// (docs/c2-regalloc.md, "Symbol ids").
// Loads the feature type `name` from the feature files: appends a 0x100-byte
// record to the feature table in g_game, fills it from the record's TDF
// fields (3D object, or GAF file and sequences, flags, resources, burn
// weapon) and returns the new feature type number.
// <windows.h> must come before <stdio.h>: it decides the registers of
// `features[j].anims`.
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "../util/tdf.h"
// g_game is declared before game.h: its symbol id decides the base and
// index order of the features[j].anims load (docs/c2-regalloc.md).
struct Game;
extern Game* g_game;
#include "../game/game.h"

// Thaldren's FeatureTdfParserVector, 0x10 bytes: the dummy allocator byte and
// its pad, then the begin and end pointers. The capacity word at +0xc is not
// read here, so the view stops after ppEnd.
struct FeatureTdfParserVector {
    int unknown_0;                     // +0x0, the allocator byte and its pad
    TdfFile** ppBegin;                 // +0x4
    TdfFile** ppEnd;                   // +0x8
};

struct AnimSrc {
    unsigned short count;               // +0x0
    unsigned char kind;                 // +0x2
};

struct Gaf;

struct Anim {
    unsigned short index;               // +0x0
    unsigned short value;               // +0x2
    unsigned char kind;                 // +0x4
    char unknown_5[3];
    AnimSrc* src;                  // +0x8
};

struct Feature {
    char name[0x80];                    // +0x00
    char description[0x14];             // +0x80
    short footprintx;                   // +0x94
    short footprintz;                   // +0x96
    union {
        void* object;                   // +0x98
        char filename[0x10];            // +0x98
    };
    Gaf* anims;                // +0xa8
    AnimSrc* seq;                  // +0xac
    AnimSrc* seqshad;              // +0xb0
    AnimSrc* seqburn;              // +0xb4
    AnimSrc* seqburnshad;          // +0xb8
    AnimSrc* seqdie;               // +0xbc
    AnimSrc* seqdieshad;           // +0xc0
    AnimSrc* seqreclamate;         // +0xc4
    AnimSrc* seqreclamateshad;     // +0xc8
    Anim ref;                   // +0xcc
    Anim refshad;               // +0xd8
    char* burnweapon;                   // +0xe4
    short sparktime;                    // +0xe8
    short damage;                       // +0xea
    float energy;                       // +0xec
    float metal;                        // +0xf0
    unsigned short dead;                // +0xf4
    unsigned short burnt;               // +0xf6
    unsigned short reclamate;           // +0xf8
    char height;                        // +0xfa
    char spreadchance;                  // +0xfb
    char reproduce;                     // +0xfc
    char reproducearea;                 // +0xfd
    unsigned short noobject : 1;        // +0xfe
    unsigned short animating : 1;
    unsigned short animtrans : 1;
    unsigned short shadtrans : 1;
    unsigned short flamable : 1;
    unsigned short geothermal : 1;
    unsigned short blocking : 1;
    unsigned short reclaimable : 1;
    unsigned short autoreclaimable : 1;
    unsigned short indestructible : 1;
    unsigned short nodisplayinfo : 1;
    unsigned short nodrawundergray : 1;
};

extern FeatureTdfParserVector* s_featureTdfParsers;
extern char g_emptyStringLiteral[];

void __stdcall BuildDataPath(char* out, const char* dir, const char* name, const char* ext);
void __stdcall FatalError(char* path);
void* __cdecl GameReallocTagged(void* p, const char* name, unsigned int size);
void* __stdcall LoadObject3d(const char* name);
Gaf* __stdcall LoadGaf(char* path);
AnimSrc* __stdcall FindGafEntry(Gaf* gaf, const char* name);
char* __stdcall FindWeaponByName(char* name);
void __stdcall InitGafSequence(Anim* ref, AnimSrc* src, int index);

// FindFeatureFile, inlined
static inline TdfFile* FindEntry(char* name)
{
    for (TdfFile** p = s_featureTdfParsers->ppBegin; p < s_featureTdfParsers->ppEnd; p++) {
        (*p)->ResetCurrentRecord();
        if ((*p)->SelectRecord(name))
            return *p;
    }
    return 0;
}

// FindOptionalGafEntry, inlined
static inline AnimSrc* SeqByName(Gaf* gaf, int unused, char* name)
{
    if (strlen(name) == 0) {
        return 0;
    }
    return FindGafEntry(gaf, name);
}

// Reads one seqname-style field into *out: the sequence named by key from the
// feature's GAF, or 0. The tested lookup goes through an int local: it changes
// the zero compare.
static inline void LoadFeatureSeq(TdfRecord* rec, char* key, char* seqname, Gaf* anims, AnimSrc** out)
{
    int ok = rec->GetFieldString(seqname, key, 0x100, g_emptyStringLiteral);
    if (ok)
        *out = SeqByName(anims, 0, seqname);
    else
        *out = 0;
}

// The same read for the burn, die and reclaim sequences, which play once: the
// sequence's loop flag is cleared.
static inline void LoadFeatureSeqOnce(TdfRecord* rec, char* key, char* seqname, Gaf* anims, AnimSrc** out)
{
    int ok = rec->GetFieldString(seqname, key, 0x100, g_emptyStringLiteral);
    if (ok) {
        *out = SeqByName(anims, 0, seqname);
        if (*out)
            (*out)->kind = 0;
    } else {
        *out = 0;
    }
}

// The return value is the full old count (0x422e40.cpp declares it unsigned
// short).
// The sequence references must be cleared field by field, not by a Reset()
// method or a pointer local.
// FUNCTION: 0x4224b0
int __stdcall LoadFeatureType(char* name)
{
    Gaf* anims;
    int j;
    // Tested TDF lookups go through an int local: it changes the zero compare.
    int ok;
    char seqname[0x100];
    char file[0x100];
    char path[0x100];

    TdfFile* entry = FindEntry(name);
    if (entry == 0) {
        sprintf(path, "Record \"%s\" missing from feature files", name);
        FatalError(path);
    }
    g_game->features = (Feature*)GameReallocTagged(g_game->features, "FEATURES",
                                                          (g_game->featureCount + 1) * 0x100);
    Feature* def = &g_game->features[g_game->featureCount];
    strncpy(def->name, name, 0x80);
    entry->current->GetFieldString(def->description, "Description", 0x14, g_emptyStringLiteral);
    def->footprintx = entry->current->GetFieldInt("footprintx", 0);
    def->footprintz = entry->current->GetFieldInt("footprintz", 0);
    def->height = entry->current->GetFieldInt("height", 0);
    ok = entry->current->GetFieldString(file, "object", 0x100, g_emptyStringLiteral);
    if (ok) {
        def->noobject = 0;
        def->object = LoadObject3d(file);
    } else {
        def->noobject = 1;
        entry->current->GetFieldString(file, "filename", 0x100, g_emptyStringLiteral);
        for (j = 0; j < g_game->featureCount; j++) {
            if (strncmp(g_game->features[j].filename, file, 0x10) == 0) {
                def->anims = 0;
                anims = g_game->features[j].anims;
                strcpy(def->filename, "REUSE");
                break;
            }
        }
        if (j == g_game->featureCount) {
            BuildDataPath(path, "anims", file, "GAF");
            def->anims = LoadGaf(path);
            strncpy(def->filename, file, 0x10);
            anims = def->anims;
        }
        LoadFeatureSeq(entry->current, "seqname", seqname, anims, &def->seq);
        LoadFeatureSeq(entry->current, "seqnameshad", seqname, anims, &def->seqshad);
        LoadFeatureSeqOnce(entry->current, "seqnameburn", seqname, anims, &def->seqburn);
        LoadFeatureSeqOnce(entry->current, "seqnameburnshad", seqname, anims, &def->seqburnshad);
        LoadFeatureSeqOnce(entry->current, "seqnamedie", seqname, anims, &def->seqdie);
        LoadFeatureSeqOnce(entry->current, "seqnamedieshad", seqname, anims, &def->seqdieshad);
        LoadFeatureSeqOnce(entry->current, "seqnamereclamate", seqname, anims, &def->seqreclamate);
        LoadFeatureSeqOnce(entry->current, "seqnamereclamateshad", seqname, anims, &def->seqreclamateshad);
    }
    def->spreadchance = entry->current->GetFieldInt("spreadchance", 0);
    def->reproduce = entry->current->GetFieldInt("reproduce", 0);
    def->reproducearea = entry->current->GetFieldInt("reproducearea", 0);
    def->metal = (unsigned short)entry->current->GetFieldInt("metal", 0);
    def->energy = (unsigned short)entry->current->GetFieldInt("energy", 0);
    def->damage = entry->current->GetFieldInt("damage", 0);
    def->animating = entry->current->GetFieldInt("animating", 0);
    def->animtrans = entry->current->GetFieldInt("animtrans", 0);
    def->shadtrans = entry->current->GetFieldInt("shadtrans", 0);
    def->flamable = entry->current->GetFieldInt("flamable", 0);
    def->geothermal = entry->current->GetFieldInt("geothermal", 0);
    def->blocking = entry->current->GetFieldInt("blocking", 0);
    def->reclaimable = entry->current->GetFieldInt("reclaimable", 0);
    def->autoreclaimable = entry->current->GetFieldInt("autoreclaimable", 1);
    def->indestructible = entry->current->GetFieldInt("indestructible", 0);
    def->nodisplayinfo = entry->current->GetFieldInt("nodisplayinfo", 0);
    def->nodrawundergray = entry->current->GetFieldInt("nodrawundergray", 0);
    if (_strcmpi(name, "DragonsTeeth") == 0)
        def->nodrawundergray = 1;
    if (_strcmpi(name, "DragonsTeeth_Core") == 0)
        def->nodrawundergray = 1;
    if (_strcmpi(name, "Fortification") == 0)
        def->nodrawundergray = 1;
    if (_strcmpi(name, "Fortification_Core") == 0)
        def->nodrawundergray = 1;
    def->sparktime = (short)(entry->current->GetFieldDouble("sparktime", 0.0) * 30.0);
    entry->current->GetFieldString(seqname, "burnweapon", 0x100, g_emptyStringLiteral);
    def->burnweapon = FindWeaponByName(seqname);
    def->ref.index = 0;
    def->ref.value = 0;
    def->ref.kind = 0;
    def->ref.src = 0;
    def->refshad.index = 0;
    def->refshad.value = 0;
    def->refshad.kind = 0;
    def->refshad.src = 0;
    if (def->animating) {
        if (def->seq)
            InitGafSequence(&def->ref, def->seq, 0);
        if (def->seqshad)
            InitGafSequence(&def->refshad, def->seqshad, 0);
    }
    return g_game->featureCount++;
}
