// Decompiled by Opus. Names are provisional.

class HapiBank {
public:
    void OpenAccount(const char* name);
};

class Class_004b4630 {
public:
    void SetIntegerItem(const char* name, int value);
};

// The "any unit passes Z" defeat condition; writes its state to a section.
class Class_0048fc70 {
public:
    int satisfied;                       // +0x4
    int celebrated;                      // +0x8

    virtual void FUN_0048f840(HapiBank* obj);
};

// FUNCTION: 0x48fcd0
void Class_0048fc70::FUN_0048f840(HapiBank* obj)
{
    obj->OpenAccount("DefeatCondition_AnyUnitPassesZ");
    ((Class_004b4630*)obj)->SetIntegerItem("Satisfied", satisfied);
    ((Class_004b4630*)obj)->SetIntegerItem("Celebrated", celebrated);
}
