# Runtime string buffer copying and shared-storage detachment

[Systems manual](../index.md) | [Roadmap](../../doc-writeup-roadmap.md)

The runtime/basic_string module has two owned entries between `0x4c4fa0` and `0x4c5170`: std::basic_string<char>::_Copy in [basic_string_4c4fa0.cpp](../../../src/runtime/basic_string_4c4fa0.cpp) and _Split in [basic_string.cpp](../../../src/runtime/basic_string.cpp). Other standard-string functions in those physical files have different address ownership; they are not counted again here. The existing MSVC 5 SP3 XSTRING implementation was read to trace both emitted bodies.

## 1. Library string representation

This standard-library string stores an allocator, character pointer, length and capacity. An unsigned byte immediately before the characters records additional shared references. Zero means uniquely owned storage and 255 marks frozen storage. This is distinct from the game's [StringRef](graphics-surface.md), whose reference count is an integer before its text pointer.

Compatible ordinary copies can share a library-string buffer. Mutation helpers detach when necessary so another string's characters do not change. c_str is NUL-terminated storage; these routines do not implement Unicode conversion, TDF parsing or the gameplay meaning of a string.

## 2. _Copy allocates replacement storage

_Copy (`0x4c4fa0`) receives a requested capacity and first tries a rounded capacity formed by setting its low five bits. For char this produces a capacity such as 31, 63 or 95, reducing frequent small reallocations. If that rounded capacity would exceed max_size, it uses the exact requested capacity instead.

It allocates capacity+2 characters: one leading reference-count byte, the text capacity and a trailing terminator slot. A catch-all allocation exception handler retries with exact requested capacity+2 rather than the rounded amount. If that retry throws, the exception is not converted to a boolean failure by this void helper. The exception fallback is triggered by throwing, not by an explicit returned-null test; the replacement game's allocation family can expose different failure behavior, and a null allocation is not separately checked before later use.

After a successful allocation it copies the current length's character bytes, remembers that old length, releases/decrements old storage through _Tidy(true), points to the new allocation after its leading byte, sets reference count zero, installs capacity and terminates at the old length through _Eos. It retains the string's existing contents and logical length rather than resizing to the requested capacity.

The current length must fit the new storage under the calling operation's preconditions. This helper does not independently truncate text or validate arbitrary direct requests smaller than existing contents. Its ordinary callers, including _Grow, determine whether old contents have already been reset before requesting a smaller capacity.

## 3. _Split detaches shared text

_Split (`0x4c50a0`) returns without action for a null pointer, a unique reference count zero or frozen count 255. For other shared counts it remembers the existing character pointer, calls _Tidy(true) to release this string's share/reset its fields, then assigns from the remembered C string into the now-empty object.

The prior shared buffer remains alive because at least another owner existed. Assignment creates this string's independent storage as needed, enabling operations such as erase to mutate without changing the other owner. The routine does not reset the source owners, copy their object metadata or release every reference globally. It performs no outer lock making simultaneous unsynchronized string changes safe.

Frozen storage follows the library's separate mutation/exposure convention and is deliberately excluded from this shared-detachment test. _Split is therefore not equivalent to unconditionally cloning any present buffer.

## 4. Interactions, boundaries and completion

[TDF support](util-tdf.md) owns the neighboring copy-constructor, _Tidy, erase, _Eos and _Grow entries. _Grow calls _Copy for capacity changes/shared storage; erase calls _Split before modifying characters. The exception types retaining message strings use the same library implementation. The farther assign entry in basic_string.cpp is outside this module's interval and is not promoted by this two-entry reference.

Both owned entries and their exact existing header bodies were inspected, including reference-count layout, rounded/exact allocation retry, content preservation and detachment. These helpers have no file format, save record, network packet, unit state or construction-rendering decision. No compiler, allocation test or game was run, no dependencies were installed, and no source listings were copied into this reference.
