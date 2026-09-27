// Decompiled by space-bunny-free. Names are provisional.
// Gives a unit its object-state block, from the piece tree the unit's id
// selects out of the table at g_game+0x14377. Which state block it gets
// depends on the unit type: with a build list at type+0x18e it also creates
// the 0x544-byte Class_00485e30 (the only class derived from Class_004b0610,
// see src/unsorted/0x485e30.cpp), hands it the list's data, the sorted state
// and then the "Create" call; without one it takes the plain state from
// FUN_0045a8d0 and stores the unit at +0xc. The last statement is shared by
// both paths, so both copies reload the state field into eax; that is what
// puts the +0x10 store after the pops in the second path.

struct Class_0045ae80;
struct UnitType_00485d40;
struct Unit_00485d40;
class Class_00485e30;

#pragma pack(push, 1)
struct UnitType_00485d40 {
    char unknown_0[0x18e];
    void* field_18e;                  // +0x18e, the build list
};

struct ObjectState_00485d40 {
    char unknown_0[0xc];
    void* field_c;                    // +0xc
    int field_10;                     // +0x10
    char unknown_14[4];
};

struct Unit_00485d40 {
    char unknown_0[0x92];
    UnitType_00485d40* type;          // +0x92
    char unknown_96[0x9a - 0x96];
    Class_00485e30* state;            // +0x9a, the Class_00485e30
    ObjectState_00485d40* objstate;   // +0x9e
    char unknown_a2[0xa6 - 0xa2];
    unsigned short field_a6;           // +0xa6, the id
};

struct Game_00485d40 {
    char unknown_0[0x14377];
    Class_0045ae80** pieces;          // +0x14377
};
#pragma pack(pop)

class Class_004b0610 {
public:
    int field_4;                      // +0x4
    int field_8;                      // +0x8
    char unknown_c[0x10 - 0xc];
    void* ptr10;                      // +0x10
    void* ptr14;                      // +0x14
    char unknown_18[0x540 - 0x18];

    Class_004b0610();
    void FUN_004b0720(void* data);

    virtual void FUN_00480c50(int, int, int);          // slot 0
    virtual void FUN_00480ce0(int, int, int);          // slot 1
    virtual void FUN_00480d50(int, int);               // slot 2
    virtual void FUN_00480db0(int, int);               // slot 3
    virtual void FUN_00480df0(int, int);               // slot 4
    virtual int FUN_00480c30(int, int);                // slot 5
    virtual int FUN_00480cb0(int, int);                // slot 6
    virtual int FUN_004b1e50(int);                     // slot 7
    virtual int FUN_004b1e60(int);                     // slot 8
    virtual int FUN_004b1e70(int);                     // slot 9
    virtual void FUN_004b1e80(int, int, int);          // slot 10
    virtual void FUN_004b1e90(int);                    // slot 11
    virtual void FUN_004b1ea0(int, int);               // slot 12
    virtual void FUN_004b1eb0(int, int);               // slot 13
    virtual void FUN_004b0650(int, int, int);          // slot 14
    virtual void FUN_004b0660(int);                    // slot 15
    virtual void FUN_004b0670(int, int);               // slot 16
    virtual int FUN_004b0680(int, int, int, int, int); // slot 17
    virtual int FUN_004b0690(int);                     // slot 18
    virtual int FUN_004b06a0();                        // slot 19
    virtual ~Class_004b0610();                         // slot 20
};

class Class_00485e30 : public Class_004b0610 {
public:
    int field_540;                    // +0x540

    Class_00485e30() {}

    virtual void FUN_00480c50(int, int, int);         // slot 0
    virtual void FUN_00480ce0(int, int, int);         // slot 1
    virtual void FUN_00480d50(int, int);              // slot 2
    virtual void FUN_00480db0(int, int);              // slot 3
    virtual void FUN_00480df0(int, int);              // slot 4
    virtual int FUN_00480c30(int, int);               // slot 5
    virtual int FUN_00480cb0(int, int);               // slot 6
    virtual int FUN_004b1e50(int);                    // slot 7
    virtual int FUN_004b1e60(int);                    // slot 8
    virtual int FUN_004b1e70(int);                    // slot 9
    virtual void FUN_004b1e80(int, int, int);         // slot 10
    virtual void FUN_004b1e90(int);                   // slot 11
    virtual void FUN_004b1ea0(int, int);              // slot 12
    virtual void FUN_004b1eb0(int, int);              // slot 13
    virtual void FUN_004b0650(int, int, int);         // slot 14
    virtual void FUN_004b0660(int);                   // slot 15
    virtual void FUN_004b0670(int, int);              // slot 16
    virtual int FUN_004b0680(int, int, int, int, int); // slot 17
    virtual int FUN_004b0690(int);                    // slot 18
    virtual int FUN_004b06a0();                       // slot 19
};

class Class_00480d40 {
public:
    char unknown_0[0x540];
    int field_540;

    void FUN_00480d40(int param_1);
};

class Class_004b0940 {
public:
    void FUN_004b0940(const char* name, int param_2, int param_3);
};

extern Game_00485d40* g_game;

ObjectState_00485d40* __stdcall FUN_0045a8d0(Class_0045ae80* obj);
ObjectState_00485d40* __stdcall FUN_0045a950(Class_0045ae80* obj, void* list,
    int player);

// FUNCTION: 0x485d40
void __stdcall FUN_00485d40(Unit_00485d40* unit)
{
    unsigned int index = 0;
    index = unit->field_a6;
    Class_0045ae80* piece = g_game->pieces[index];
    if (unit->type->field_18e) {
        unit->state = new Class_00485e30;
        unit->state->FUN_004b0720(unit->type->field_18e);
        unit->objstate = FUN_0045a950(piece, unit->type->field_18e, (int)unit);
        ((Class_00480d40*)unit->state)->FUN_00480d40((int)unit->objstate);
        ((Class_004b0940*)unit->state)->FUN_004b0940("Create", 0, 1);
    } else {
        unit->state = 0;
        unit->objstate = FUN_0045a8d0(piece);
        unit->objstate->field_c = unit;
    }
    unit->objstate->field_10 = 0;
}
