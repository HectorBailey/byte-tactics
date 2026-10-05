// Decompiled by deepseek-v4.1-flash. Names are provisional.
// 585-byte GUI list entry (same class as 0x432fb0.cpp). This is MSVC 5's
// <algorithm> _Sort instantiated for the 585-byte element with a __stdcall
// comparison callback, so the real header supplies _Median and
// _Unguarded_partition (both inlined here). The 4th parameter is the
// template's unused _Ty* tag, passed as 0 and never read.
// <windows.h> must come first: it flips the evaluation order of the final
// element-count comparison to match the original.
#include <windows.h>
#include <algorithm>

#pragma pack(push, 1)
class UnitType {
public:
    char unknown_0[0x249];
    UnitType& operator=(const UnitType& src);
};
#pragma pack(pop)

typedef int (__stdcall* Compare)(const UnitType&, const UnitType&);

// FUNCTION: 0x432d40
void __stdcall FUN_00432d40(UnitType* first, UnitType* last,
                            Compare comp, int unused)
{
    for (; std::_SORT_MAX < last - first; ) {
        UnitType* _M = std::_Unguarded_partition(first, last,
            std::_Median(UnitType(*first),
                UnitType(*(first + (last - first) / 2)),
                UnitType(*(last - 1)), comp), comp);
        if (last - _M <= _M - first)
            FUN_00432d40(_M, last, comp, 0), last = _M;
        else
            FUN_00432d40(first, _M, comp, 0), first = _M;
    }
}
