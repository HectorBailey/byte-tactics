// Game: the game state (Thaldren's GameState), 0x3924d bytes. The names are the
// settled ones from the #6655 measurement. Types with a shared header are included;
// the headerless ones it holds by value are laid out in front, pointer targets are
// forward declarations.
#ifndef GAME_H
#define GAME_H

#include <windows.h>
#include <ddraw.h>
#include <dsound.h>
#include <dplay.h>
#include <stdio.h>
#include <vector>
#include <list>
#include <map>
#include "../util/vec3.h"
#include "../graphics/rect.h"
#include "../map/grid.h"
#include "../graphics/handle.h"
#include "../ingame/frame_timers.h"
#include "../network/settings.h"
#include "../network/player.h"
#include "../gui/gui.h"

#pragma pack(push, 1)

struct Game;

struct AnimEntry_00429700;
struct Arg_00419670;
struct Bits_0045c570;
struct BuildList_0041ace0;
struct Cell;
struct Cell_004816a0;
struct Class_00437c80;
struct Class_0045ae80;
struct Class_00472200;
struct Class_0047cb00;
struct Class_00480160;
struct Class_0048ff40;
class Sound;
class UnitSync;
struct HapinetProviderConnEntry;
struct Ctx_00499890;
struct CursorState;
struct Data_0046d970;
struct WorldEffect;
struct Desc_004437c0;
struct SoundCategory;
struct ChatHudEntry;
struct Entry_00479620;
struct Eye_00482130;
struct FaceFlags_459830;
struct Feature;
struct FeatureSpot;
struct SoundFlags;
struct VisualFlags;
struct FrameTimers;
struct Game;
struct Grid;
struct Handle;
struct IconSet_00466780;
struct List_00420960;
struct Map_0042e440;
class Mission;
struct Net_4517b0;
struct Obj_00406f50;
struct Object3do;
struct PathMap;
struct PieceFrame;
struct PlayerRef;
struct Point_0041cd50;
struct Rect;
struct Settings;
struct EndGamePlayerStatRow;
struct Src_004ab400;
struct ResourceHudCache;
struct SelectionInfoCache;
struct Unit;
struct UnitDef;
struct V4i;
struct Vec3;





struct Arg_00419670 {  // 0x18 bytes
    int x;  // +0x0
    int y;  // +0x4
    int unknown_8[4];  // +0x8
};

struct Point_0041cd50 {  // 0x8 bytes, 1 view
    int x;  // +0x0
    int y;  // +0x4
};

// The middle-mouse scroll (control_panel.cpp's CursorState; Thaldren's
// aMiddleMouseSavedCursor and the nMiddleMouse fields after it).
struct CursorState {  // 0x2c bytes
    Arg_00419670 start;  // +0x0, the mouse when the scroll began
    int active;  // +0x18, Thaldren's nMiddleMouseScrollActive
    Point_0041cd50 center;  // +0x1c
    Point_0041cd50 savedScroll;  // +0x24
};


struct EndGamePlayerStatRow {  // 0x3a bytes, Thaldren's: one player's line of the end-game table
    char name[30];  // +0x0
    int kills;  // +0x1e
    int losses;  // +0x22
    int energyProduced;  // +0x26
    int metalProduced;  // +0x2a
    int energyWasted;  // +0x2e
    int metalWasted;  // +0x32
    int score;  // +0x36
};

struct Object3do {  // 0x34 bytes
    unsigned char state;  // +0x0
    char unknown_1[3];
    int vertexCount;  // +0x4
    int faceCount;  // +0x8
    int firstFace;  // +0xc
    int f10;  // +0x10
    int f14;  // +0x14
    unsigned short* color;  // +0x18
    char* name;  // +0x1c
    int active;  // +0x20
    void* vertices;  // +0x24
    PieceFrame* faces;  // +0x28
    Object3do* sibling;  // +0x2c
    Object3do* child;  // +0x30
};


struct WorldEffect {  // 0x54 bytes, Thaldren's
    int explodePiece;  // +0x0, Thaldren's pExplodePiece
    Handle ref1;  // +0x4
    Handle ref2;  // +0x10
    Vec3 pos;  // +0x1c
    Vec3 size;  // +0x28
    Vec3 vel;  // +0x34
    Vec3 spin;  // +0x40
    short angle_x;  // +0x4c
    short angle_y;  // +0x4e
    short angle_z;  // +0x50
    unsigned short flag : 1;  // +0x52
    unsigned short : 15;
};

struct FaceFlags_459830 {  // 0x4 bytes
    unsigned int textured : 1;  // +0x0
    unsigned int usePic : 1;
    unsigned int shaded : 1;
    unsigned int rest : 29;
};

struct PieceFrame {  // 0x20 bytes
    int unknown_0;  // +0x0
    int count;  // +0x4
    int unknown_8;  // +0x8
    unsigned short* indices;  // +0xc
    char unknown_10[8];
    unsigned short* color;  // +0x18
    FaceFlags_459830 flags;  // +0x1c
};

#include "../weapons/weapon_def.h"

// The visual options word (Thaldren's wVisualFlags), with the bit names
// settings.cpp reads from the registry.
struct VisualFlags {  // 0x2 bytes
    unsigned short damagebars : 1;      // bit 0
    unsigned short antiAlias : 1;       // bit 1
    unsigned short shadows : 1;         // bit 2
    unsigned short vehicleShadows : 1;  // bit 3
    unsigned short featureShadows : 1;  // bit 4
    unsigned short shading : 1;         // bit 5
    unsigned short ditheredFog : 1;     // bit 6
    unsigned short b7 : 1;              // bit 7, toggled by key 0xe5 (keys.cpp), not saved
    unsigned short switchAlt : 1;       // bit 8
    unsigned short rest : 7;
};

struct ChatHudEntry {  // 0x48 bytes, Thaldren's: one line of the chat HUD ring
    char text[64];  // +0x0
    unsigned int time;  // +0x40
    unsigned short unit;  // +0x44, the unit the message points at (net_chat.cpp)
    char player;  // +0x46, the sender's slot, 10 for none
    unsigned char flags;  // +0x47
};


struct ResourceHudCache {  // 0x21 bytes, Thaldren's: the resource bar's per-slot cache
    unsigned char slotIndex;  // +0x0
    float energyAmountDisp;  // +0x1
    float energyIncome;  // +0x5
    float energyUsage;  // +0x9
    float metalAmountDisp;  // +0xd
    float metalIncome;  // +0x11
    float metalUsage;  // +0x15
    float energyStorage;  // +0x19
    float metalStorage;  // +0x1d
};

struct SelectionInfoCache {  // 0x38 bytes of Thaldren's 0x3c; its +0x38 is Game's optionsLightbarDirty
    char unknown_0[48];  // Thaldren's hoverKey, a selection hover snapshot
    int totalUnitsPanelSlide;  // +0x30
    unsigned short* lightbar;  // +0x34, the cached lightbar frame
};


// The sound options word (Thaldren's wSoundFlags), with settings.cpp's bit names.
struct SoundFlags {  // 2 bytes
    unsigned short soundMode : 3;       // bits 0-2
    unsigned short restoreVolume : 1;   // bit 3
    unsigned short ackfx : 1;           // bit 4
    unsigned short buildfx : 1;         // bit 5
    unsigned short speechfx : 1;        // bit 6
    unsigned short rest : 9;
};

// Unused here: the symbol ids these declarations take stand where game.h's own
// copy of FrameTimers stood, keeping UpdateBattleRoom (0x44a680) matching
// (docs/c2-regalloc.md).
void RegisterUnitOrders(void);
void RegisterGroundOrders(void);
void EnableAICommands(void);
void RegisterAICommands(void);
void FUN_00406f40(void);
void ResetAIPlayers(void);
void RegisterVtolOrders(void);
void StepAllGafSequences(void);
void ResetNetStats(void);

struct Game {  // 0x3924d bytes
    char unknown_0;  // +0x0
    unsigned char gameVersionMajor;  // +0x1
    unsigned char gameVersionMinor;  // +0x2
    unsigned char gameVersionBuild;  // +0x3
    char* build_date;  // +0x4
    char* build_time;  // +0x8
    Ctx_00499890* displayContext;  // +0xc
    Sound* sound;              // +0x10
    char session[4];  // +0x14
    char unknown_18[1069];
    V4i* guids;  // +0x445
    char unknown_449[40];
    Settings settings;  // +0x471
    char unknown_4c1[8];
    int lobby2;  // +0x4c9
    int local_id;  // +0x4cd
    char unknown_4d1[20];
    Net_4517b0* dplConnection;  // +0x4e5
    char unknown_4e9[4];
    int compressionOff;  // +0x4ed
    int netMode;  // +0x4f1
    char unknown_4f5[4];
    int count;  // +0x4f9
    int field_4fd;  // +0x4fd
    char unknown_501[16];
    Player* slowest;  // +0x511
    int lag;  // +0x515
    Gui gui;  // +0x519, the GUI root (gui.h), 0xcf6 bytes
    char unknown_120f[0xe0];
    ChatHudEntry chatHudRing[30];  // +0x12ef, Thaldren's aChatHudRing
    int lobbySyncTick;  // +0x1b5f
    Player players[10];  // +0x1b63
    char unknown_2851[331];
    int duplicateIds;  // +0x299c
    Entry_00479620* options;  // +0x29a0
    int loaded[10];  // +0x29a4
    char unknown_29cc[4];
    int field_29d0[11];  // +0x29d0
    int startPosShuffle[10];  // +0x29fc
    char unknown_2a24[4];
    int startPosShuffleReady;  // +0x2a28
    char unknown_2a2c[4];
    UnitSync* sync;              // +0x2a30
    int recvPacketSize;  // +0x2a34
    unsigned char* buffer;  // +0x2a38
    unsigned short numPlayers;  // +0x2a3c
    unsigned short tail;  // +0x2a3e
    unsigned short head;  // +0x2a40
    unsigned char localPlayer;  // +0x2a42
    unsigned char playerIndex;  // +0x2a43
    unsigned char netGameFlags;  // +0x2a44, the low byte; the word is net_game.cpp's netGameFlagsWord
    unsigned char netGameFlagsHi;  // +0x2a45, the high byte of the same word
    unsigned char netOrCdSentinel;  // +0x2a46
    char unknown_2a47[4];
    void* data[15];  // +0x2a4b
    char unknown_2a87[20];
    void* list;  // +0x2a9b
    V4i* sessions;  // +0x2a9f
    HapinetProviderConnEntry* conns;  // +0x2aa3, the DPLAY_CONNECTIONS array
    Desc_004437c0* desc;  // +0x2aa7
    char* shared;  // +0x2aab
    unsigned short bit0 : 1;  // +0x2aaf
    unsigned short bit1 : 1;
    unsigned short bits2 : 14;
    char unknown_2ab1[155];
    unsigned short low_2b4c : 4;  // +0x2b4c
    unsigned short flag_2b4c : 1;
    unsigned short rest_2b4c : 11;
    char unknown_2b4e[28];
    char game_info[84];  // +0x2b6a
    unsigned char frontendMenuState;  // +0x2bbe
    unsigned char frontendSubstate;  // +0x2bbf
    unsigned char frontendSubstateRequest;  // +0x2bc0
    char gameName[16];  // +0x2bc1
    char unknown_2bd1[1];
    char nickname[17];  // +0x2bd2
    char password[11];  // +0x2be3
    unsigned short flags;  // +0x2bee
    unsigned char mode_2bf0;  // +0x2bf0
    unsigned char chatRecipients[11];  // +0x2bf1, ten players and the saved byte
    char unknown_2bfc[44];
    int table_2c28[10];  // +0x2c28
    char unknown_2c50[38];
    Arg_00419670 view;  // +0x2c76
    short x;  // +0x2c8e
    short y;  // +0x2c90
    int boxStartX;   // +0x2c92
    int boxStartHeight;  // +0x2c96
    int boxStartZ;   // +0x2c9a
    int boxEndX;     // +0x2c9e
    int boxEndHeight;  // +0x2ca2
    int boxEndZ;     // +0x2ca6
    Vec3 pos;  // +0x2caa
    int boxStartTick;  // +0x2cb6
    unsigned short hoverUnitId;  // +0x2cba
    unsigned short cellFeature;  // +0x2cbc
    char selected_2cbe;  // +0x2cbe
    char unknown_2cbf[4];
    unsigned char orderMode;   // +0x2cc3
    unsigned short buildTypeIndex;  // +0x2cc4
    unsigned char pad_2cc6 : 5;  // +0x2cc6
    unsigned char bit5_2cc6 : 1;
    unsigned char bit6_2cc6 : 1;
    unsigned char rest_2cc6 : 1;
    CursorState cursor;  // +0x2cc7
    WeaponDef weapons[256];  // +0x2cf3, Thaldren's aWeaponDefs
    int projectileCount;  // +0x141f3
    Obj_00406f50* projectiles;  // +0x141f7
    void* sortUnitList;  // +0x141fb
    void* sortIndices;  // +0x141ff
    void* sortLineCount;  // +0x14203
    PathMap* pathfinder;   // +0x14207
    FeatureSpot* spots;  // +0x1420b
    Unit* unit;  // +0x1420f
    char unknown_14213[8];
    int list_1421b;  // +0x1421b
    void** grid;         // +0x1421f
    int mapWidthWorld;  // +0x14223, Thaldren's nMapWidthWorld
    int mapHeightWorld;  // +0x14227
    int mapWidth;  // +0x1422b
    int mapHeight;  // +0x1422f
    int mapWidthTiles;  // +0x14233, Thaldren's nMapWidthTiles
    int mapHeightTiles;  // +0x14237
    int screenTilesX;  // +0x1423b
    int screenTilesY;  // +0x1423f
    char unknown_14243[16];
    int featureCount;  // +0x14253
    int scanIndex;  // +0x14257
    int windSpeedMin;  // +0x1425b
    int windSpeedMax;  // +0x1425f
    int gravity;  // +0x14263
    float tidal;  // +0x14267
    void* radarFrame;  // +0x1426b
    Feature* features;  // +0x1426f
    unsigned short* visibilityMask;  // +0x14273
    int count_14277;  // +0x14277
    Eye_00482130* eyes;  // +0x1427b
    unsigned char seaLevel;  // +0x1427f
    unsigned char debugMode;    // +0x14280
    unsigned short mapFlags;  // +0x14281, Thaldren's wMapFlags
    IconSet_00466780* iconSet;  // +0x14283
    Cell* heightMap;  // +0x14287, Thaldren's pHeightMap
    unsigned short* mapValues;  // +0x1428b
    Grid grid1;  // +0x1428f
    Class_0047cb00* cells_1429f;  // +0x1429f
    unsigned int width_142a3;  // +0x142a3
    unsigned int height_142a7;  // +0x142a7
    char unknown_142ab[4];
    int worldWidthFixed;  // +0x142af
    int worldHeightFixed;  // +0x142b3
    int overflowBucket;  // +0x142b7
    Rect viewLimit;  // +0x142bb
    int minimapViewRect[4];  // +0x142cb, the radar viewport rectangle
    void* finalSurface;  // +0x142db
    void* mappedSurface;  // +0x142df
    void* pictureSurface;  // +0x142e3
    short posX;  // +0x142e7
    short posY;  // +0x142e9
    short width_142eb;  // +0x142eb
    short screen_h;  // +0x142ed
    short blinkTimer;  // +0x142ef
    unsigned short flags_142f1;  // +0x142f1
    int value_142f3;  // +0x142f3
    Obj_00406f50* tracked;  // +0x142f7
    int xs[4];  // +0x142fb
    int ys[4];  // +0x1430b
    unsigned char valid[4];  // +0x1431b
    int scroll_x;  // +0x1431f
    int scroll_y;  // +0x14323
    int x2;  // +0x14327
    int y2;  // +0x1432b
    int value_1432f;  // +0x1432f
    int value_14333;  // +0x14333
    int sum_x;  // +0x14337
    int sum_y;  // +0x1433b
    Vec3 trackedPos;  // +0x1433f
    short value_1434b;  // +0x1434b
    unsigned char scrollSpeed;  // +0x1434d
    unsigned char flags_1434e;  // +0x1434e
    unsigned short unitsPerPlayer;  // +0x1434f
    unsigned short poolCount;  // +0x14351
    int f14353;  // +0x14353
    Unit* units;  // +0x14357
    Unit* unitsEnd;  // +0x1435b, Thaldren's pUnitArrayEnd
    unsigned short* list_1435f;  // +0x1435f
    void* hotRadar;  // +0x14363
    int count_14367;  // +0x14367
    int hotRadarUnitCount;  // +0x1436b
    unsigned short focusUnitId;  // +0x1436f
    unsigned short autoFollowTicks;  // +0x14371
    unsigned int bit0_14373 : 1;  // +0x14373
    unsigned int paused : 1;
    unsigned int rest : 30;
    Class_0045ae80** types;  // +0x14377
    Class_00437c80* obj;  // +0x1437b
    char unknown_1437f[4];
    void* xform;  // +0x14383
    void* projected;  // +0x14387
    void* assemPts;     // +0x1438b
    int unitDefCount;  // +0x1438f, units with a UnitDef
    int unitDefCountBits;  // +0x14393
    int unitDefEnumDirty;  // +0x14397
    UnitDef* unitDefs;  // +0x1439b
    char unknown_1439f[8];
    char paletteRgba[0x400];  // +0x143a7, 256 RGB entries
    int baseHeight;  // +0x147a7
    int count_147ab;  // +0x147ab
    AnimEntry_00429700* entries_147af;  // +0x147af
    char unknown_147b3[8];
    void* gaf_147bb;  // +0x147bb
    void* gaf_147bf;  // +0x147bf
    void* gaf_147c3;  // +0x147c3
    void* gaf_147c7;  // +0x147c7
    void* gaf_147cb;  // +0x147cb
    void* smoke1;  // +0x147cf, Thaldren's pSmoke1Seq
    void* smoke2;  // +0x147d3, Thaldren's pSmoke2Seq
    int fire1;  // +0x147d7
    int alfboom1;  // +0x147db
    void* radlogo;      // +0x147df
    void* radlogohigh;  // +0x147e3
    void* nuclogo;      // +0x147e7
    void* src1;  // +0x147eb
    void* src2;  // +0x147ef
    void* flameStreamSeq;  // +0x147f3
    void* src3;  // +0x147f7
    int explode2;  // +0x147fb
    int explode3;  // +0x147ff
    int explode4;  // +0x14803
    int explode5;  // +0x14807
    int nuke1;  // +0x1480b
    void* gaf_1480f;  // +0x1480f
    unsigned short* image_14813;  // +0x14813
    unsigned short* image_14817;  // +0x14817
    int igpaused;  // +0x1481b
    unsigned short* sidePanelTopSeq[5];   // +0x1481f
    unsigned short* sidePanelBotSeq[5];   // +0x14833
    unsigned short* sidePanelSideSeq[5];  // +0x14847
    Cell_004816a0* losTable;     // +0x1485b
    void* black[4];  // +0x1485f
    void* gray[4];  // +0x1486f
    Src_004ab400* table[1];  // +0x1487f
    int cursorAttack;  // +0x14883
    int cursorAirstrike;  // +0x14887
    int cursorTooFar;  // +0x1488b
    int cursorCapture;  // +0x1488f
    int cursorDefend;  // +0x14893
    int cursorRepair;  // +0x14897
    int cursorPatrol;  // +0x1489b
    int cursorPickup;  // +0x1489f
    int cursorTeleport;  // +0x148a3
    int cursorRevive;  // +0x148a7
    int cursorReclamate;  // +0x148ab
    int cursorLoad;  // +0x148af
    int cursorUnload;  // +0x148b3
    int cursorMove;  // +0x148b7
    int cursorSelect;  // +0x148bb
    int cursorFindSite;  // +0x148bf
    int cursorRed;  // +0x148c3
    int cursorGrn;  // +0x148c7
    unsigned short* cursorNormal;  // +0x148cb
    int cursorHourglass;  // +0x148cf
    int pathIcon;  // +0x148d3
    void* logos;  // +0x148d7
    void* logos32;  // +0x148db
    int count_148df;  // +0x148df
    void** blocks;  // +0x148e3
    int animplayCursorCount;  // +0x148e7
    void** animplayCursors;  // +0x148eb
    int gafAnimClearedSlot;  // +0x148ef
    int fxGaf;  // +0x148f3
    int igTitles;  // +0x148f7
    int vismasks;  // +0x148fb
    int fog;  // +0x148ff
    int cursors;      // +0x14903
    int array_14907[1];  // +0x14907
    char unknown_1490b[16];
    int count_1491b;  // +0x1491b
    WorldEffect debris[300];  // +0x1491f, Thaldren's aWorldEffects
    List_00420960* lists[3];  // +0x1ab8f
    void* explosionLensFrame;  // +0x1ab9b
    Object3do slots[300];  // +0x1ab9f
    char vertices[300][96];  // +0x1e88f
    PieceFrame frames[300][6];  // +0x2590f
    int soundCount;  // +0x33a0f
    int soundIds[1];  // +0x33a13
    char unknown_33a17[1020];
    char soundNames[1][32];  // +0x33e13
    char unknown_33e33[8160];
    char soundFiles[256][32];  // +0x35e13
    SoundCategory* entries_37e13;  // +0x37e13
    int entry_count;  // +0x37e17
    int offscreen;    // +0x37e1b
    int width_37e1f;  // +0x37e1f
    int screenHeight;  // +0x37e23
    Rect rect;  // +0x37e27
    int viewWidth;  // +0x37e37
    int viewHeight;  // +0x37e3b
    ResourceHudCache resourceHudCache;  // +0x37e3f
    SelectionInfoCache selectionInfoCache;  // +0x37e60
    int optionsLightbarDirty;  // +0x37e98
    unsigned short unitIndex;  // +0x37e9c
    unsigned short unitDefIndex;  // +0x37e9e
    char name[30];  // +0x37ea0
    unsigned short flags_37ebe;  // +0x37ebe
    unsigned short moveOrder : 3;  // +0x37ec0
    unsigned short cloak : 2;
    unsigned short activation : 2;
    unsigned short flags_7 : 9;
    unsigned short ordersCapabilityBitsHi;  // +0x37ec2
    unsigned int windCounter;  // +0x37ec4
    int windSpeedDenominator;  // +0x37ec8
    int windX;  // +0x37ecc
    int windY;  // +0x37ed0
    int windZ;  // +0x37ed4
    unsigned short windDirection;  // +0x37ed8
    int windSpeed;  // +0x37eda
    float windFraction;  // +0x37ede
    int windEnabled;  // +0x37ee2
    unsigned short maxUnits;  // +0x37ee6
    unsigned short lobbyInitScratch;  // +0x37ee8
    unsigned short maxUnitsLobby;  // +0x37eea
    unsigned short maxUnits_37eec;  // +0x37eec
    int difficulty;  // +0x37eee
    int side;  // +0x37ef2
    int battleCommanderDeath;  // +0x37ef6
    int interfaceType;  // +0x37efa
    int mode;  // +0x37efe
    int screenChat;  // +0x37f02
    VisualFlags visualFlags;  // +0x37f06
    int gamma;  // +0x37f08
    int fxVolume;  // +0x37f0c
    int musicVolume;  // +0x37f10
    unsigned short musicFlags;  // +0x37f14
    unsigned char cdMode;       // +0x37f16
    unsigned char unitChat;     // +0x37f17
    unsigned char unitChatText;  // +0x37f18
    SoundFlags soundFlags;  // +0x37f19
    int displayWidth;  // +0x37f1b
    int displayHeight;  // +0x37f1f
    int textScroll;   // +0x37f23
    int textLines;  // +0x37f27
    int mousespeed;  // +0x37f2b
    unsigned short uiOptionFlags;  // +0x37f2f
    unsigned int lobbyTimeoutSecs;  // +0x37f31
    int netSendDropPct;  // +0x37f35
    int sides;  // +0x37f39
    char names[1][562];  // +0x37f3d
    char unknown_3816f[2248];
    unsigned int lastTick;  // +0x38a37
    int frames_38a3b;  // +0x38a3b
    int scrollScale;  // +0x38a3f
    int carry;        // +0x38a43
    int gameTick;     // +0x38a47
    unsigned short speedCtrl;    // +0x38a4b
    short effectiveGameSpeed;  // +0x38a4d
    short streak;  // +0x38a4f
    unsigned short otherFlags;  // +0x38a51
    char installPath[1];  // +0x38a53
    char unknown_38a54[255];
    char text_38b53[256];  // +0x38b53
    int movieCaptureIndex;  // +0x38c53
    int movieOutputRate;  // +0x38c57
    unsigned int nextMovieFrameTick;  // +0x38c5b
    int imageOutputDirDirty;  // +0x38c5f
    int movieOutputRateDirty;  // +0x38c63
    int unusedAfterMovieOutputRateDirty;  // +0x38c67
    char saveName[1];  // +0x38c6b
    char unknown_38c6c[255];
    int pendingSaveStore;  // +0x38d6b
    char progress;  // +0x38d6f
    unsigned char loadProgress;  // +0x38d70
    unsigned char loadPctUnits;  // +0x38d71
    unsigned char loadPercent;  // +0x38d72
    unsigned char progress_38d73;  // +0x38d73
    unsigned char shade;  // +0x38d74
    unsigned short net_flags;  // +0x38d75
    Class_00472200* lists_38d77;  // +0x38d77
    void* moviePlayer;  // +0x38d7b
    unsigned short allMissionsFlags;  // +0x38d7f
    int numSkirmishPlayers;  // +0x38d81
    FrameTimers timers;  // +0x38d85, Thaldren's profileTimingBars
    int profileBarsEnabled;  // +0x38dd5
    EndGamePlayerStatRow endGamePlayerStats[10];  // +0x38dd9
    char unknown_3901d[58];
    int field_39057;  // +0x39057
    unsigned int deadline;  // +0x3905b
    unsigned int nextTime;  // +0x3905f
    int done;  // +0x39063
    int steps;  // +0x39067
    int statBarRevealIndex;  // +0x3906b
    int unknown3906f;  // +0x3906f
    int skip;  // +0x39073
    void* lastFrame;  // +0x39077
    void* image_3907b;  // +0x3907b
    void* palette_3907f;  // +0x3907f
    void* currentPalette;  // +0x39083
    void* desiredPalette;  // +0x39087
    void* fadeTable;  // +0x3908b
    char glamourBriefPath[256];  // +0x3908f
    int maxStats[7];  // +0x3918f
    int mission;  // +0x391ab
    int won;          // +0x391af
    int unitInfoHoverTrack;  // +0x391b3
    unsigned short unitInfoHoverUnitId;  // +0x391b7
    int unitBuilderHoverTrack;  // +0x391b9
    unsigned short unitBuilderHoverUnitId;  // +0x391bd
    int showRanges;   // +0x391bf
    int showBps;      // +0x391c3
    int buildListCount;  // +0x391c7
    BuildList_0041ace0* buildLists;  // +0x391cb
    char campaignMissionStatus[26];  // +0x391cf, Thaldren's one 26-byte field
    Mission* mapInfo;  // +0x391e9
    Class_0048ff40* list_391ed;  // +0x391ed
    int frontendState;  // +0x391f1, the frontend mode eight other views name this way
    void (__stdcall *handler)(void);  // +0x391f5
    int fontComix;  // +0x391f9
    void* fontSmlFont;  // +0x391fd
    char providerGuid[16];  // +0x39201, the selected service provider's GUID
    int connection;  // +0x39211
    int selectedDplayConnection;  // +0x39215
    int singleCommanderDeath;  // +0x39219
    int mapping;  // +0x3921d
    int lineOfSight;  // +0x39221
    int singleLOSType;  // +0x39225
    int multiCommanderDeath;  // +0x39229
    int multiMapping;  // +0x3922d
    int multiLineOfSight;  // +0x39231
    int multiLosTypeElev;  // +0x39235
    short endGameCountdown;  // +0x39239
    unsigned short endGameFlags;  // +0x3923b
    int playMovie;  // +0x3923d
    int introMovieLoop;  // +0x39241
    int cmdlineSkipIntroMovie;  // +0x39245
    int restartMissionRequest;  // +0x39249
    Game(void);
    Player* Current(void);
    Player* Current2(void);
};

struct Bits_0045c570 {  // 0x2 bytes
    char unknown_0[2];
};


#pragma pack(pop)

#endif