// Decompiled by deepseek-v4.1. Names are provisional.
// This TU was built with /Gz (__stdcall default), so the <algorithm> sort
// templates it instantiated are __stdcall. Standing in for them here with
// __stdcall copies reproduces the original's `ret 0xc`/`ret 0x10` helpers
// (0x488810, 0x488920, 0x488960) and removes the caller-side `add esp`
// (84.8% -> 92.6%). The two remaining differences are allocator tie-breaks,
// both from the STL code this TU inlines; the rest of the function is
// byte-identical. What still differs:
//   - 0x485750 (_Sort's `_L - _M <= _M - _F` test): the original subtracts
//     into ecx (left) and edx (right); ours picks edx (left) and ecx (right),
//     so every byte to the end of the block is swapped but semantically equal.
//     The mirrored conditions (`_L - _M > _M - _F` with the bodies swapped,
//     and `_M - _F >= _L - _M`) keep the same jump targets but emit jg from
//     the other side (jl) or recolor the whole inlined region (73.2%).
//   - 0x485894 tail fill loop: the original keeps `slot` in esi and the
//     zero-extended copy of unitsPerPlayer in edx; ours keeps `slot` in edx
//     and loads unitsPerPlayer into esi (adding an `xor esi, esi`), and the
//     induction variable therefore lands on q+0xff instead of q. Tried: named
//     end/u locals (90.8%/89.6%), q hoisted above the stores (identical),
//     0x6b/0x67 store order (89.7%), q declared above the stores and the
//     bound kept as an item field reload (identical bytes); none move
//     the edx/esi split, so this is function-wide allocator state.

#include <string.h>

// <xutility>/<algorithm> as the original file compiled them (/Gz: __stdcall).
// Defining _ALGORITHM_ keeps out the header's __cdecl copies.
#define _ALGORITHM_
namespace std {

const int _SORT_MAX = 16;

template<class _Ty> inline
_Ty* __stdcall _Val_type(const _Ty*)
{
    return ((_Ty*)0);
}

template<class _Ty, class _Pr> inline
_Ty __stdcall _Median(_Ty _X, _Ty _Y, _Ty _Z, _Pr _P)
{
    if (_P(_X, _Y))
        return (_P(_Y, _Z) ? _Y : _P(_X, _Z) ? _Z : _X);
    else
        return (_P(_X, _Z) ? _X : _P(_Y, _Z) ? _Z : _Y);
}

template<class _BI1, class _BI2> inline
_BI2 __stdcall copy_backward(_BI1 _F, _BI1 _L, _BI2 _X)
{
    while (_F != _L)
        *--_X = *--_L;
    return (_X);
}

template<class _FI1, class _FI2, class _Ty> inline
void __stdcall _Iter_swap(_FI1 _X, _FI2 _Y, _Ty*)
{
    _Ty _Tmp = *_X;
    *_X = *_Y, *_Y = _Tmp;
}

template<class _FI1, class _FI2> inline
void __stdcall iter_swap(_FI1 _X, _FI2 _Y)
{
    _Iter_swap(_X, _Y, _Val_type(_X));
}

template<class _RI, class _Ty, class _Pr> inline
void __stdcall _Sort(_RI _F, _RI _L, _Pr _P, _Ty*)
{
    for (; _SORT_MAX < _L - _F; ) {
        _RI _M = _Unguarded_partition(_F, _L, _Median(_Ty(*_F),
            _Ty(*(_F + (_L - _F) / 2)), _Ty(*(_L - 1)), _P), _P);
        if (_L - _M <= _M - _F)
            _Sort(_M, _L, _P, _Val_type(_F)), _L = _M;
        else
            _Sort(_F, _M, _P, _Val_type(_F)), _F = _M;
    }
}

template<class _RI, class _Ty, class _Pr> inline
_RI __stdcall _Unguarded_partition(_RI _F, _RI _L, _Ty _Piv, _Pr _P)
{
    for (; ; ++_F) {
        for (; _P(*_F, _Piv); ++_F)
            ;
        for (; _P(_Piv, *--_L); )
            ;
        if (_L <= _F)
            return (_F);
        iter_swap(_F, _L);
    }
}

template<class _RI, class _Ty, class _Pr> inline
void __stdcall _Unguarded_insert(_RI _L, _Ty _V, _Pr _P)
{
    for (_RI _M = _L; _P(_V, *--_M); _L = _M)
        *_L = *_M;
    *_L = _V;
}

template<class _RI, class _Ty, class _Pr> inline
void __stdcall _Insertion_sort_1(_RI _F, _RI _L, _Pr _P, _Ty*)
{
    if (_F != _L)
        for (_RI _M = _F; ++_M != _L; ) {
            _Ty _V = *_M;
            if (!_P(_V, *_F))
                _Unguarded_insert(_M, _V, _P);
            else {
                copy_backward(_F, _M, _M + 1);
                *_F = _V;
            }
        }
}

template<class _RI, class _Pr> inline
void __stdcall _Insertion_sort(_RI _F, _RI _L, _Pr _P)
{
    _Insertion_sort_1(_F, _L, _P, _Val_type(_F));
}

template<class _RI, class _Ty, class _Pr> inline
void __stdcall _Sort_0(_RI _F, _RI _L, _Pr _P, _Ty*)
{
    if (_L - _F <= _SORT_MAX)
        _Insertion_sort(_F, _L, _P);
    else {
        _Sort(_F, _L, _P, (_Ty*)0);
        _Insertion_sort(_F, _F + _SORT_MAX, _P);
        for (_F += _SORT_MAX; _F != _L; ++_F)
            _Unguarded_insert(_F, _Ty(*_F), _P);
    }
}

template<class _RI, class _Pr> inline
void __stdcall sort(_RI _F, _RI _L, _Pr _P)
{
    _Sort_0(_F, _L, _P, _Val_type(_F));
}

}

class Class_00435100 {
public:
    int FUN_00435100();
};

struct Player_004854a0 {
    char unknown_0[4];
    unsigned int key;                   // +0x4
    char unknown_8[0x146 - 8];
    unsigned char field_146;            // +0x146
    char unknown_147[0x14b - 0x147];
};

#pragma pack(push, 1)
struct Game_004854a0 {
    char unknown_0[0x1b63];
    unsigned char players[10 * 0x14b];  // +0x1b63, stride 0x14b
    char unknown_2851[0x1434f - 0x2851];
    unsigned short field_1434f;         // +0x1434f
    unsigned short poolCount;           // +0x14351
    char unknown_14353[0x14357 - 0x14353];
    unsigned char* pool;                // +0x14357
    unsigned char* field_1435b;         // +0x1435b
    void* hotUnits;                     // +0x1435f
    void* hotRadar;                     // +0x14363
    char unknown_14367[0x1436f - 0x14367];
    unsigned short field_1436f;         // +0x1436f
    char unknown_14371[0x14373 - 0x14371];
    unsigned int field_14373;           // +0x14373
    char unknown_14377[0x1439b - 0x14377];
    unsigned int field_1439b;           // +0x1439b
    char unknown_1439f[0x37ee6 - 0x1439f];
    unsigned short unitsPerPlayer;      // +0x37ee6
    char unknown_37ee8[0x391e9 - 0x37ee8];
    Class_00435100* mode;               // +0x391e9
};
#pragma pack(pop)

extern Game_004854a0* g_game;

void* FUN_004d83b0(const char* name, unsigned int size);

int __stdcall FUN_00485940(Player_004854a0* a, Player_004854a0* b)
{
    if (g_game->mode->FUN_00435100() == 3)
        return a->key < b->key;
    return a < b;
}

// FUNCTION: 0x4854a0
void __stdcall FUN_004854a0(void)
{
    g_game->field_1436f = 0;
    g_game->field_14373 &= 0xfffffffd;
    g_game->field_1434f = g_game->unitsPerPlayer;
    g_game->poolCount = (unsigned short)(g_game->unitsPerPlayer * 10 + 1);

    unsigned char* pool = g_game->pool = (unsigned char*)FUN_004d83b0("UNIT MEMORY", g_game->poolCount * 0x118);
    memset(pool, 0, g_game->poolCount * 0x118);

    unsigned int ten = g_game->unitsPerPlayer * 10;
    g_game->hotUnits = FUN_004d83b0("HOT UNITS", ten * 2);
    g_game->hotRadar = FUN_004d83b0("HOT RADAR UNITS", ten * 10);
    g_game->field_1435b = g_game->pool + g_game->poolCount * 0x118 - 0x118;

    unsigned short n;
    for (n = 0; n < g_game->poolCount; n++) {
        *(unsigned short*)(pool + n * 0x118 + 0xa8) = n;
        *(unsigned int*)(pool + n * 0x118 + 0x92) = g_game->field_1439b;
    }

    Player_004854a0* v[10];
    int k;
    for (k = 0; k < 10; k++)
        v[k] = (Player_004854a0*)(g_game->players + k * 0x14b);

    std::sort(v, v + 10, FUN_00485940);

    pool[0xff] = 0xff;
    *(unsigned int*)(pool + 0x96) = 0;
    int i;
    for (i = 0; i < 10; i++) {
        Player_004854a0* item = v[i];
        int c = g_game->unitsPerPlayer * i + 1;
        unsigned char* slot = pool + c * 0x118;
        *(unsigned char**)((char*)item + 0x67) = slot;
        *(unsigned char**)((char*)item + 0x6b) = slot + g_game->unitsPerPlayer * 0x118 - 0x118;
        *(unsigned short*)((char*)item + 0x6f) = *(unsigned short*)(slot + 0xa8);
        *(unsigned short*)((char*)item + 0x71) =
            *(unsigned short*)(*(unsigned char**)((char*)item + 0x6b) + 0xa8);
        for (unsigned char* q = slot; q <= *(unsigned char**)((char*)item + 0x6b); q += 0x118) {
            *(void**)(q + 0x96) = item;
            q[0xff] = *(unsigned char*)((char*)item + 0x146);
            *(unsigned int*)(q + 0xac) = 0xffffffffu;
        }
    }
}
