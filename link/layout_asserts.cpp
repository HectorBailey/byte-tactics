// The layout asserts of docs/linking.md phase 2, in a translation unit that
// holds nothing else. They are declarations, so keeping them here rather than
// in the type's own header leaves the compiler state of every matched file
// untouched (docs/c2-regalloc.md); tools/linkcheck.py compiles this file, so a
// wrong size or offset fails there instead of quietly moving a match.
//
// One size assert per consolidated type whose size the exe gives, and an offset
// assert for each field whose offset was in dispute when its views were merged
// into one.

#include "layout_assert.h"

#include "../src/weapons/projectile.h"
#include "../src/gui/gadget.h"

// Projectile: one weapon in flight, Thaldren's record of 0x6b bytes.
DECOMP_SIZE_ASSERT(Projectile, 0x6b)
DECOMP_OFFSET_ASSERT(Projectile, flags, 0x69)

// Gadget: one GUI list entry, 0x15b bytes.
DECOMP_SIZE_ASSERT(Gadget, 0x15b)
DECOMP_OFFSET_ASSERT(Gadget, status, 0x138)
