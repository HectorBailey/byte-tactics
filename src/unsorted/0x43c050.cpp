// Decompiled by deepseek-v4.1-flash. Names are provisional.
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

static void SortRange_0043c050(Elem_0043c390* F, Elem_0043c390* L, Pred_0043c050 P)
{
    for (; 16 < L - F; ) {
        Elem_0043c390* M = FUN_0043cb20(F, L,
            FUN_0043ca70(*F, *(F + (L - F) / 2), *(L - 1), P), P);
        if (L - M <= M - F) {
            FUN_0043c720(M, L, P, 0);
            L = M;
        } else {
            FUN_0043c720(F, M, P, 0);
            F = M;
        }
    }
}

static void UnguardedInsert_0043c050(Elem_0043c390* L, Elem_0043c390 V, Pred_0043c050 P)
{
    for (Elem_0043c390* M = L; P(V, *--M); L = M)
        *L = *M;
    *L = V;
}

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

    Elem_0043c390* F0 = DAT_00512340.begin();
    Elem_0043c390* L0 = DAT_00512340.end();
    if (L0 - F0 <= 16) {
        FUN_0043c990(F0, L0, FUN_0043c020, 0);
    } else {
        SortRange_0043c050(F0, L0, FUN_0043c020);
        FUN_0043c990(F0, F0 + 16, FUN_0043c020, 0);
        for (F0 += 16; F0 != L0; ++F0)
            UnguardedInsert_0043c050(F0, *F0, FUN_0043c020);
    }

    FUN_00406bf0();
    FUN_00415b20();
    FUN_00406f00();
    FUN_00403180();
}
