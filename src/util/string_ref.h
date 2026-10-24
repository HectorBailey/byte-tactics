// StringRef, the reference-counted string handle: a pointer to the characters
// with the reference count in the int just before them. The constructors are
// 0x4c9180 (empty), 0x4c91a0 (copy) and 0x4c91b0 (from text); copying and
// assignment bump the count and ReleaseRef decrements it and frees the block
// at zero. 0x4c9230 builds from a length, 0x4c9290 and 0x4c9310 change the
// case, 0x4c93f0 assigns text and 0x4c9490 takes a substring. The methods are
// defined in graphics/surface.cpp.
#ifndef STRING_REF_H
#define STRING_REF_H

#include <stdlib.h>
#include <string.h>

class StringRef {
public:
    char* ptr;               // refcount lives in the dword before ptr

    StringRef();
    StringRef(const StringRef& other);
    StringRef(const char* text);
    StringRef(const char* text, int len);
    ~StringRef() { ReleaseRef(); }
    StringRef& operator=(const StringRef& other)
    {
        Assign(&other);
        return *this;
    }
    StringRef* Assign(const StringRef* other);
    void ReleaseRef();
    StringRef* Append(const StringRef& other);
    StringRef* MakeLower();
    StringRef* MakeUpper();
    StringRef* AssignText(const char* text);
    // Copies the characters into a block of their own unless the count is 1.
    char* GetUnique()
    {
        int len = (int)strlen(ptr);
        if (*(int*)(ptr - 4) != 1) {
            int* block = (int*)malloc(len + 5);
            *block = 1;
            char* copy = (char*)(block + 1);
            strcpy(copy, ptr);
            (*(int*)(ptr - 4))--;
            if (*(int*)(ptr - 4) == 0) {
                free(ptr - 4);
            }
            ptr = copy;
            return copy;
        }
        return ptr;
    }
    bool IsEmpty() const { return *ptr == 0; }
    StringRef SubString(int start, int end) const;
};

#endif
