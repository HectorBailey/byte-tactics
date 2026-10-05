// Decompiled by space-bunny-free. Names are provisional.

// Element type: 0x249 bytes with a name string at +0x20 (see 0x42db60.cpp).
class UnitType {
public:
    char unknown_0[0x249];
    UnitType& operator=(const UnitType& src);
};

typedef int (__stdcall* Compare)(const UnitType&, const UnitType&);

static UnitType* __inline Copy_backward(UnitType* F, UnitType* L, UnitType* X)
{
    while (F != L)
        *--X = *--L;
    return X;
}

static void __inline Unguarded_insert(UnitType* L, UnitType V, Compare P)
{
    for (UnitType* M = L; P(V, *--M); L = M)
        *L = *M;
    *L = V;
}

// FUNCTION: 0x432fb0
void __stdcall FUN_00432fb0(UnitType* first, UnitType* last, Compare comp, void* tag)
{
    if (first != last)
        for (UnitType* M = first; ++M != last; ) {
            UnitType V = *M;
            if (!comp(V, *first))
                Unguarded_insert(M, V, comp);
            else {
                Copy_backward(first, M, M + 1);
                *first = V;
            }
        }
}
