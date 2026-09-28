// Decompiled by space-bunny-free. Names are provisional.
#include <windows.h>

extern void (*DAT_005289bc)();

struct Elem_004dd8c0 {
    char data[0x30];
};

class Class_004dd8c0 {
public:
    int field_0;                        // +0x0
    Elem_004dd8c0* first;               // +0x4
    Elem_004dd8c0* last;                // +0x8
    Elem_004dd8c0* end;                 // +0xc
    void FUN_004dd8c0(Elem_004dd8c0* where, unsigned int n, Elem_004dd8c0* val);
};

static void CopyElem_004dd8c0(Elem_004dd8c0* dst, const Elem_004dd8c0* src)
{
    if (dst != 0) {
        *dst = *src;
    }
}

// FUNCTION: 0x4dd8c0
void Class_004dd8c0::FUN_004dd8c0(Elem_004dd8c0* where, unsigned int n, Elem_004dd8c0* val)
{
    Elem_004dd8c0* s;
    Elem_004dd8c0* d;
    unsigned int i;
    if ((unsigned int)((int)(end - last) / 0x30) >= n) {
        unsigned int after = (unsigned int)((int)(last - where) / 0x30);
        if (n > after) {
            for (s = where, d = where + n; s != last; s++, d++)
                CopyElem_004dd8c0(d, s);
            for (d = last, i = n - after; i != 0; i--, d++)
                CopyElem_004dd8c0(d, val);
            for (d = where, s = where + n; d != s; d++)
                *d = *val;
        } else {
            if (n != 0) {
                for (s = last - n, d = last; s != last; s++, d++)
                    CopyElem_004dd8c0(d, s);
                for (s = last - n, d = last; s != where; ) {
                    s--;
                    d--;
                    *d = *s;
                }
                for (d = where, s = where + n; d != s; d++)
                    *d = *val;
            }
        }
        last += n;
        return;
    }

    unsigned int sz = (unsigned int)((int)(last - first) / 0x30);
    unsigned int grow = n;
    if (n < sz) {
        grow = sz;
    }
    unsigned int cap = sz + grow;
    Elem_004dd8c0* newmem;
    do {
        newmem = (Elem_004dd8c0*)GlobalAlloc(0, cap * 0x30);
        if (newmem == 0 && DAT_005289bc != 0)
            DAT_005289bc();
    } while (newmem == 0 && DAT_005289bc != 0);

    for (s = first, d = newmem; s != where; s++, d++)
        CopyElem_004dd8c0(d, s);
    for (d = newmem + (where - first), i = n; i != 0; i--, d++)
        CopyElem_004dd8c0(d, val);
    for (s = where, d = newmem + (where - first) + n; s != last; s++, d++)
        CopyElem_004dd8c0(d, s);

    if (first != 0) {
        GlobalFree(first);
    }
    end = newmem + cap;
    last = newmem + (n + (unsigned int)((int)(last - first) / 0x30)) * 0x30;
    first = newmem;
}
