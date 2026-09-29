// Decompiled by deepseek-v4.1-flash. Names are provisional.
// .TDF record parser, written as the constructor of a tree node: it parses the
// text (arg2) between the enclosing brackets, recursively builds child nodes
// for "name { ... }" sub-records, and stores "key=value;" pairs in the sorted
// entries vector. On a syntax error it appends a message to the error buffer
// and calls FUN_004b6290 (which never returns normally).
//
// PARTIAL: 1152 bytes against 1115, true instruction LCS 166/401 vs 166/420
// (~40.4%). Note: `check.py` reports a lower difflib number (25.6%) than an
// earlier revision (36.3%) even though LCS went up and the two previously
// missing allocator-byte stores are now correct; difflib punishes the reordered
// prologue, which is the downstream effect of the register decision below. The
// whole control flow and every callee are in place. Resolved since the earlier
// revision: the allocator-byte stores, and the whitespace skip (writing it as a
// temporary `char* p = current; ... current = p;` keeps `current` in ecx and
// stores it only once, which is the original's shape). The one remaining cause
// is a single register-allocation decision, and every visible diff follows
// from it:
//  - the original puts `this` in ebp and materialises &children in ebx
//    (`lea ebx,[ebp+4]`), and spills both (`this` to [esp+0x24], &children to
//    [esp+0x38]). That is what makes its frame 0x7fc instead of our 0x7f4 and
//    its error buffer land at [esp+0x3c]. Ours puts `this` in ebx and never
//    keeps &children in a register (it recomputes `lea ecx,[ebx+4]` at the
//    insert call), so nothing needs spilling and the frame is 8 bytes short.
//    Because the original reuses ebp for the binary-search `mid` and ebx for
//    the inlined strcmp's `bl`, it holds this and &children in memory across
//    those blocks. Everything else (whitespace-skip shape, the strcmp's
//    `mov bl,[esi]; cmp dl,bl`, the block order) is downstream of that.
//  - resolved here: the two std::vector default constructors. MSVC 5's
//    vector(const _A& _Al = _A()) copies the allocator from the default-argument
//    temporary, whose storage is an uninitialised stack slot, hence the
//    original's `mov al,[esp+7]` / `mov cl,[esp+0xf]` and the two byte stores.
//    Giving our vector the same defaulted-argument constructor reproduces them.
// Tried and did NOT flip the allocation, so nobody repeats them:
//  - including <vector> and taking &insert: the calls still get inlined
//    (function grows past 2000 bytes) even though the out-of-line instantiation
//    is emitted; dead inline call sites did not consume the budget either.
//    Using the real <vector> DOES give `mov ebp,ecx` / `lea ebx,[ebp+4]`, so
//    the allocation and the inlining are entangled, but the inlining is fatal.
//  - pointer/reference locals to children (`&children`, `children.end()` saved
//    first, an inline `Kids(self)` accessor, calling the insert from an inline
//    `AddChild(this, child)` helper): all fold back to `[ebx+4]`.
// The std::vector below is declared rather than included so that
// vector<T>::insert stays an out-of-line call to 0x4c4d70 / 0x4c51e0.
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// A minimal std::vector whose insert is only declared, so VC5 emits an
// out-of-line call to the real template instantiation (0x4c4d70 / 0x4c51e0)
// instead of inlining it. The layout matches MSVC 5's <vector>: an empty
// allocator byte, then _First/_Last/_End.
namespace std {

template <class T>
class allocator {
};

template <class T, class A = allocator<T> >
class vector {
public:
    typedef T* iterator;
    typedef unsigned int size_type;

    A _A;
    T* _First;
    T* _Last;
    T* _End;

    explicit vector(const A& al = A()) : _A(al), _First(0), _Last(0), _End(0) {}
    iterator begin() { return _First; }
    iterator end() { return _Last; }
    T& operator[](size_type i) { return _First[i]; }
    void insert(iterator where, size_type n, const T& value);
};

}

class Class_004c3e40;

class Class_004c91a0 {
public:
    char* ptr;

    Class_004c91a0() {}
    Class_004c91a0(const Class_004c91a0& other);
};

class Class_004c9180 {
public:
    char* ptr;

    Class_004c9180();
};

class Class_004c54a0 {
public:
    Class_004c91a0 first;
    Class_004c91a0 second;

    Class_004c54a0(const Class_004c54a0& other);
};

class Class_004c54d0 {
public:
    Class_004c91a0 first;
    Class_004c91a0 second;

    Class_004c54d0(const Class_004c91a0& a, const Class_004c91a0& b);
};

class Class_004c4340 {
public:
    Class_004c91a0* FUN_004c4340(Class_004c91a0* out, char* start, char* end);
};

class Class_004c9390 {
public:
    char* data;

    void FUN_004c9390();
};

class Class_004c93b0 {
public:
    char* ptr;

    Class_004c93b0* FUN_004c93b0(Class_004c91a0* param_1);
};

#pragma pack(push, 1)
class Class_004c3e40 {
public:
    char* name;                               // +0x0
    std::vector<Class_004c3e40*> children;    // +0x4
    char unknown_14;                          // +0x14
    std::vector<Class_004c54a0> entries;      // +0x15
    char* unknown_25;                         // +0x25

    Class_004c3e40(char* name, char* text, int* nextblock, char* filename);
};
#pragma pack(pop)

char* FUN_004d8610(char* text);
char* FUN_004b6ba0(char* text, int len);
void FUN_004b6290(char* text);

// FUNCTION: 0x4c3e40
Class_004c3e40::Class_004c3e40(char* name, char* text, int* nextblock, char* filename)
{
    char error[0x7d0] = "Parse error in .TDF File! ";
    char* current = text;

    this->name = FUN_004d8610(name);

    while (1) {
        {
            char* p = current;
            while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'))
                p++;
            current = p;
        }

        if (*current == 0) {
            if (nextblock == 0) {
                if (text > current - 1) {
                    unknown_25 = 0;
                    return;
                }
                unknown_25 = FUN_004b6ba0(text, (int)(current - 1) - (int)text - 1);
                return;
            }
            goto eof_error;
        }
        if (*current == '[') {
            char* close = strchr(current, ']');
            if (close == 0)
                goto close_error;
            Class_004c91a0 subname;
            ((Class_004c4340*)this)->FUN_004c4340(&subname, current + 1, close);
            current = close + 1;
            {
                char* p = current;
                while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'))
                    p++;
                current = p;
            }
            if (*current != '{') {
                strcat(error, "Sub-record - opening '{' not found");
                ((Class_004c9390*)&subname)->FUN_004c9390();
                goto report;
            }
            Class_004c3e40* child =
                new Class_004c3e40(subname.ptr, current + 1, (int*)&current, filename);
            children.insert(children.end(), 1, child);
            ((Class_004c9390*)&subname)->FUN_004c9390();
            continue;
        }
        if (*current == '}') {
            if (nextblock)
                *nextblock = (int)(current + 1);
            if (text > current - 1) {
                unknown_25 = 0;
                return;
            }
            unknown_25 = FUN_004b6ba0(text, (int)(current - 1) - (int)text - 1);
            return;
        }
        {
            char* eq = strchr(current, '=');
            if (eq == 0) {
                strcat(error, "Data field - '=' not found");
                goto report;
            }
            Class_004c91a0 key;
            ((Class_004c4340*)this)->FUN_004c4340(&key, current, eq);
            current = eq + 1;
            char* semi = strchr(current, ';');
            if (semi == 0) {
                strcat(error, "Data field - ';' not found");
                ((Class_004c9390*)&key)->FUN_004c9390();
                goto report;
            }
            Class_004c91a0 value;
            ((Class_004c4340*)this)->FUN_004c4340(&value, current, semi);
            current = semi + 1;

            Class_004c54a0* lo = entries.begin();
            Class_004c54a0* hi = entries.end();
            while (lo != hi) {
                Class_004c54a0* mid = lo + (hi - lo) / 2;
                bool less = _strcmpi(key.ptr, mid->first.ptr) < 0;
                if (less)
                    lo = mid + 1;
                else
                    hi = mid;
            }
            Class_004c91a0* dst;
            if (lo != entries.end() && strcmp(lo->first.ptr, key.ptr) == 0) {
                dst = &lo->second;
            } else {
                Class_004c9180 empty;
                Class_004c54d0 pair(key, *(Class_004c91a0*)&empty);
                unsigned int idx = (unsigned int)(lo - entries.begin());
                entries.insert(lo, 1, *(Class_004c54a0*)&pair);
                dst = &entries[idx].second;
                ((Class_004c9390*)((char*)&pair + 4))->FUN_004c9390();
                ((Class_004c9390*)&pair)->FUN_004c9390();
                ((Class_004c9390*)&empty)->FUN_004c9390();
            }
            ((Class_004c93b0*)dst)->FUN_004c93b0(&value);
            ((Class_004c9390*)&value)->FUN_004c9390();
            ((Class_004c9390*)&key)->FUN_004c9390();
        }
        continue;

    close_error:
        strcat(error, "Sub-record - closing ']' not found");
        goto report;
    eof_error:
        strcat(error, "End of file - nextblock not zero");
    report:
        if (name)
            sprintf(error + strlen(error), " - name = '%s' from file %s", name, filename);
        FUN_004b6290(error);
        return;
    }
}
