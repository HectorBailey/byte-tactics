// Decompiled by Haiku. Names are provisional.

class SJE_CdPlayerClass {
public:
    int GetMaxBuffers();
};

// FUNCTION: 0x4cf220
int SJE_CdPlayerClass::GetMaxBuffers()
{
    return *(int*)((char*)this + 0x2c);
}
