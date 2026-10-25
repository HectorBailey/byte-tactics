// TdfField, one `key = value;` entry of a TDF section or of the translation
// tables: two reference-counted string handles. The constructors (0x4c54a0,
// 0x4c54d0) and the destructor (0x4c5190) are defined in tdf.cpp, where the
// pair is used; a file that only keeps entries in a vector needs the layout
// and the declarations.
//
// A header of its own rather than util/tdf.h: tdf.h reaches
// orders/vtol_orders.cpp through map/mission.h, and the declarations there
// move its VtolEvadeOrder (0x413bc0), a handler with about 14 symbol ids of
// headroom (docs/c2-regalloc.md).
#ifndef TDF_FIELD_H
#define TDF_FIELD_H

#include "string_ref.h"

struct TdfField {
    StringRef key;                  // +0x0
    StringRef value;                // +0x4

    TdfField(const StringRef& a, const StringRef& b);
    ~TdfField();
};

#endif
