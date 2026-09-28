// Decompiled by deepseek-v4.1-flash. Names are provisional.
// Appends one 25-byte record from the static table at 0x4fd288 to the global
// std::vector<Elem> at 0x512340 (element: 0x19 bytes, char* name at +0x15),
// std::sorts it with the __stdcall name compare 0x43c020, then calls four
// registration helpers. The original inlines vector::reserve and std::sort's
// _Sort_0; the recursive _Sort (0x43c720), _Median (0x43ca70),
// _Unguarded_partition (0x43cb20) and _Insertion_sort (0x43c990) stay
// out-of-line, so those are called by name here.
//
// 82.2% with every reference resolving. Still differs:
//  - register allocation in the sort range setup (the original keeps _F in esi
//    and _L in ebp; this keeps _L in esi) and in the tail _Insertion_sort call;
//  - the _Median argument setup (index temp edx vs ecx, return buffer edx vs
//    eax).
// Approaches that did not reach 100%:
//  - std::sort: the recursive call is the mangled std::_Sort, which does not
//    resolve to FUN_0043c720 in data/symbols.csv.
//  - vector::reserve: its size() call is the mangled
//    UElem_0043c390::?$vector::size, but data/symbols.csv names 0x43c360
//    Class_0043c360::FUN_0043c360 (every other vector size in the table is
//    UElem_<addr>::?$vector::size). That version scores 87.3% but that
//    reference would be a mismatch, and vector::reserve with a trivial element
//    inlines _Destroy (0x43c390) away, so the call at 0x43c108 is lost.
//  - giving the element a user destructor makes _Destroy out of line (as in the
//    original, whose _Destroy is `ret 8`) but changes the sort's allocation.
//    With the _Sort_0 body as a separate inline function it scores 84.5% and
//    matches the _Median area, but then _Destroy is inlined as a loop calling
//    the scalar deleting destructor instead of the single call at 0x43c108.
#include <vector>
#include <string.h>

#pragma pack(push, 1)
struct Elem_0043c390 {
    char unknown_0[0x15];
    char* name;                        // +0x15

    ~Elem_0043c390() {}

    int operator<(const Elem_0043c390&) const { return 0; }
    int operator==(const Elem_0043c390&) const { return 0; }
    int operator!=(const Elem_0043c390&) const { return 0; }
};
#pragma pack(pop)

typedef std::vector<Elem_0043c390> Vec_0043c390;
typedef int (__stdcall* Pred_0043c050)(const Elem_0043c390&, const Elem_0043c390&);

extern Vec_0043c390 DAT_00512340;
extern Elem_0043c390 DAT_004fd288;
extern Elem_0043c390 DAT_004fd2a1;

class Class_0043c360 {
public:
    char unknown_0[4];
    int field_4;
    int field_8;
    int FUN_0043c360(void);
};

struct Access_0043c390 : Vec_0043c390 {
    void grow(size_type N) {
        if (capacity() < N) {
            iterator S = allocator.allocate(N, (void*)0);
            _Ucopy(_First, _Last, S);
            _Destroy(_First, _Last);
            allocator.deallocate(_First, _End - _First);
            _End = S + N;
            _Last = S + ((Class_0043c360*)this)->FUN_0043c360();
            _First = S;
        }
    }
};

template class std::vector<Elem_0043c390>;

extern "C" void FUN_00406bf0();
extern "C" void FUN_00415b20();
extern "C" void FUN_00406f00();
extern "C" void FUN_00403180();
void __stdcall FUN_0043c720(Elem_0043c390*, Elem_0043c390*, Pred_0043c050, Elem_0043c390*);
void __stdcall FUN_0043c990(Elem_0043c390*, Elem_0043c390*, Pred_0043c050, int*);
Elem_0043c390 __stdcall FUN_0043ca70(Elem_0043c390, Elem_0043c390, Elem_0043c390, Pred_0043c050);
Elem_0043c390* __stdcall FUN_0043cb20(Elem_0043c390*, Elem_0043c390*, Elem_0043c390, Pred_0043c050);

int __stdcall FUN_0043c020(const Elem_0043c390& a, const Elem_0043c390& b)
{
    return _strcmpi(a.name, b.name) < 0 ? 1 : 0;
}

template<class _RI, class _Ty, class _Pr>
void _Sort_0043c050(_RI _F, _RI _L, _Pr _P, _Ty *)
{for (; 16 < _L - _F; )
    {_RI _M = FUN_0043cb20(_F, _L,
        FUN_0043ca70(_Ty(*_F), _Ty(*(_F + (_L - _F) / 2)), _Ty(*(_L - 1)), _P), _P);
    if (_L - _M <= _M - _F)
        FUN_0043c720(_M, _L, _P, (_Ty *)0), _L = _M;
    else
        FUN_0043c720(_F, _M, _P, (_Ty *)0), _F = _M; }}

template<class _BI, class _Ty, class _Pr>
void _Unguarded_insert_0043c050(_BI _L, _Ty _V, _Pr _P)
{for (_BI _M = _L; _P(_V, *--_M); _L = _M)
    *_L = *_M;
*_L = _V; }

template<class _RI, class _Ty, class _Pr>
void _Sort_0_0043c050(_RI _F, _RI _L, _Pr _P, _Ty *)
{if (_L - _F <= 16)
    FUN_0043c990(_F, _L, _P, 0);
else
    {_Sort_0043c050(_F, _L, _P, (_Ty *)0);
    FUN_0043c990(_F, _F + 16, _P, 0);
    for (_F += 16; _F != _L; ++_F)
        _Unguarded_insert_0043c050(_F, _Ty(*_F), _P); }}

// FUNCTION: 0x43c050
void FUN_0043c050()
{
    Access_0043c390* v = (Access_0043c390*)&DAT_00512340;
    Elem_0043c390* first = v->begin();
    int N = (first == 0 ? 0 : v->end() - first) + 1;
    v->grow(N);

    Elem_0043c390* p = &DAT_004fd288;
    do {
        DAT_00512340.push_back(*p);
        ++p;
    } while (p != &DAT_004fd2a1);

    Elem_0043c390* F = DAT_00512340.begin();
    Elem_0043c390* L = DAT_00512340.end();
    if (L - F <= 16) {
        FUN_0043c990(F, L, FUN_0043c020, 0);
    } else {
        _Sort_0043c050(F, L, FUN_0043c020, (Elem_0043c390*)0);
        FUN_0043c990(F, F + 16, FUN_0043c020, 0);
        for (F += 16; F != L; ++F)
            _Unguarded_insert_0043c050(F, Elem_0043c390(*F), FUN_0043c020);
    }

    FUN_00406bf0();
    FUN_00415b20();
    FUN_00406f00();
    FUN_00403180();
}
