// Decompiled by Haiku. Names are provisional.

class SJE_CdPlayerClass {
public:
    int GetCurrentTrack();
};

// FUNCTION: 0x4ce7f0
int SJE_CdPlayerClass::GetCurrentTrack()
{
    return *(int*)((char*)this + 0x208);
}
