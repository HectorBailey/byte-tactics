// Decompiled by Claude Opus 5.5. Names are provisional.
// Stays in its own file: in features.cpp the merged file's symbol ids change
// the base and index of the feature-table address and of the dead, burnt and
// reclamate stores (docs/c2-regalloc.md, "Symbol ids").
// Resolves every feature type's "featuredead", "featurereclamate" and
// "featureburnt" names to feature type numbers (loading them when needed),
// updating the load progress byte as it goes.
// <string.h> and map/feature.h are this file's only headers: its parser
// classes have to stay views of their own.
#include <string.h>

// Keeps its own view of the parser classes: with the shared tdf.h the inlined
// FindEntry's register allocation changes and 0x422ea0 drops.
class TdfFile {
public:
    char unknown_0[4];
    void* parser;                       // +0x4

    void ResetCurrentRecord();
    int SelectRecord(char* name);
};

class TdfRecord {
public:
    int GetFieldString(char* dst, char* key, int size, char* def);
};

// Thaldren's FeatureTdfParserVector, 0x10 bytes: the dummy allocator byte and
// its pad, then the begin and end pointers. The capacity word at +0xc is not
// read here, so the view stops after ppEnd.
struct FeatureTdfParserVector {
    int unknown_0;                     // +0x0, the allocator byte and its pad
    TdfFile** ppBegin;                 // +0x4
    TdfFile** ppEnd;                   // +0x8
};

// The feature record: map/feature.h's Feature, the 0x100-byte pool element.
#include "../map/feature.h"

#pragma pack(push, 1)
struct Game {
    char unknown_0[0x14253];
    int featureCount;                   // +0x14253
    char unknown_14257[0x1426f - 0x14257];
    Feature* features;      // +0x1426f
    char unknown_14273[0x38d72 - 0x14273];
    unsigned char loadPercent;          // +0x38d72
};
#pragma pack(pop)

extern Game* g_game;
extern FeatureTdfParserVector* s_featureTdfParsers;
extern char g_emptyStringLiteral[];

unsigned short __stdcall LoadFeatureType(char* name);

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

// FindFeatureType-like search, inlined
static inline unsigned short FindName(char* name)
{
    for (int i = 0; i < g_game->featureCount; i++) {
        if (_strcmpi(name, g_game->features[i].name) == 0) {
            return (unsigned short)i;
        }
    }
    return 0xffff;
}

// FindOrLoadFeatureType, inlined
static inline unsigned short FeatureIndex(char* name)
{
    unsigned short i = FindName(name);
    if (i != 0xffff)
        return i;
    return LoadFeatureType(name);
}

// Do/while behind its own guard, table pointer taken after the guard; i = 0 is
// stored before the count pointer.
// FUNCTION: 0x422ea0
void ResolveFeatureLinks()
{
    char buf[0x100];
    Feature** features;
    int i = 0;
    int* count = &g_game->featureCount;
    if (*count > 0) {
        features = &g_game->features;
        do {
            TdfRecord* parser = (TdfRecord*)FindEntry((*features)[i].name)->parser;
            if (parser->GetFieldString(buf, "featuredead", 0x100, g_emptyStringLiteral))
                (*features)[i].dead = FeatureIndex(buf);
            else
                (*features)[i].dead = 0xffff;
            if (parser->GetFieldString(buf, "featurereclamate", 0x100, g_emptyStringLiteral))
                (*features)[i].reclamate = FeatureIndex(buf);
            else
                (*features)[i].reclamate = 0xffff;
            if (parser->GetFieldString(buf, "featureburnt", 0x100, g_emptyStringLiteral))
                (*features)[i].burnt = FeatureIndex(buf);
            else
                (*features)[i].burnt = 0xffff;
            g_game->loadPercent = (unsigned char)((i + 1) * 100 / *count);
            i++;
        } while (i < *count);
    }
}
