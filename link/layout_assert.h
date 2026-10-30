// The layout markers of docs/linking.md phase 2. VC++ 5.0 has no
// static_assert, so a negative array size is the failure: the translation unit
// stops compiling at the type or field whose layout is wrong.
//
// An assert is a declaration, and extra declarations move the compiler's symbol
// counter (docs/c2-regalloc.md), which moves matched functions in every file
// that includes the type's header. The asserts therefore do not sit beside the
// types: they are written in link/layout_asserts.cpp, a translation unit of its
// own, which tools/linkcheck.py compiles.
#ifndef LAYOUT_ASSERT_H
#define LAYOUT_ASSERT_H

#include <stddef.h>

#define DECOMP_PASTE(a, b) a##b
#define DECOMP_PASTE2(a, b) DECOMP_PASTE(a, b)

#define DECOMP_STATIC_ASSERT_(LINE, V) \
    namespace { typedef int DECOMP_PASTE2(layout_assert_, LINE)[(V) ? 1 : -1]; }

#define DECOMP_STATIC_ASSERT(V) DECOMP_STATIC_ASSERT_(__LINE__, V)
#define DECOMP_SIZE_ASSERT(T, S) DECOMP_STATIC_ASSERT(sizeof(T) == (S))
#define DECOMP_OFFSET_ASSERT(T, F, O) DECOMP_STATIC_ASSERT(offsetof(T, F) == (O))

#endif
