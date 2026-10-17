# COB image relocation, checksum registry and integer tree support

[Systems manual](../index.md) | [Roadmap](../../doc-writeup-roadmap.md)

## 1. Actual module boundary

The util/int_map group has 20 annotated entries from `0x4b2290` to before `0x4b3630`. Nineteen are recorded in [int_map.cpp](../../../src/util/int_map.cpp): two generated static-lifetime routines, three COB image/cache operations and fourteen emitted standard-library members. HapiBank::InitBank at `0x4b3620` is physically in [hapi_bank.cpp](../../../src/util/hapi_bank.cpp) but belongs to this address group.

The cache is a file-local std::map<int,int> mapping a loaded COB image's 32-bit address to a checksum of its original file bytes. It is not the generic pathfinder map, a collection of every game unit, a filename-based shared image cache or a script interpreter. [COB execution](units-cob.md) and [unit-type asset lifetime](units-unit-types.md) consume it.

## 2. Loading and checksum timing

LoadCobScript (`0x4b2450`) asks HAPI_LoadFile for a complete allocation. A null load returns null without insertion. Otherwise it obtains HAPI_FileLengthByName, computes ComputeChecksum over that many bytes, stores the result under the allocation's integer address and only then relocates the image. It returns that same allocation pointer.

Checksum timing matters: pointer fixups change the image bytes, and checksumming afterward would depend on the process address. The external map preserves the raw-image checksum without overwriting a header field. A partial local header view provisionally calls its field at 0x10 checksum, but that field is not where this function stores its computed checksum; the interpreter view identifies 0x10 as static-variable count.

The file length is queried separately after loading. The routine does not compare an allocation's actual loaded length against the second query, validate a minimum header size or guard the checksum walk against changed/malformed asset metadata. Every successful call loads a new image; it does not first find an existing image by path and return a shared reference. Multiple unit instances ordinarily share their definition's script pointer through higher unit-type ownership, rather than this function deduplicating arbitrary filenames.

## 3. In-place relocation

The local Relocate helper converts five header offset fields into pointers by adding the image base: 0x18, 0x1c, 0x20, 0x24 and 0x28. It then adjusts count_1 elements of the table at the relocated 0x1c field, count_2 elements at 0x20, and the offset member of count_3 two-integer records at 0x28.

The [interpreter's ScriptTable view](../../../src/units/cob.cpp) establishes script-entry table at 0x18, script names at 0x1c, piece count at 0x08 and instruction data at 0x24. Script-entry values are code offsets used by the interpreter; this helper relocates the entry-table pointer without adding the base to every code-offset value. Names/piece-name pointers and the tail records have the distinct per-element fixups described above. The remaining tail record's non-offset member is left unchanged; no unsupported semantic name is invented for it here.

The operation is destructive and intended once per freshly loaded image. Repeating it would add the base a second time. Zero-valued offsets are also adjusted rather than treated as absent pointers. Counts/offsets are trusted, with no total-buffer range validation or allocation rollback after an invalid fixup. A checksum entry is already inserted before those fixups begin.

## 4. Query and free behavior

GetCobChecksum (`0x4b26f0`) returns s_cobScriptCache[key] using map subscript. A missing key therefore inserts a new entry with integer value zero. This is a mutating lookup, not find-plus-absent status. Zero can mean an actual zero checksum or a synthesized value for an unregistered pointer. It does not recompute the relocated image or verify the script remains allocated.

FreeCobScript (`0x4b2540`) ignores key zero. For a nonzero key it erases that key from the map, then frees the address through GameFreeThunk. It does not require that erase found an entry. Repeated frees or a foreign integer address are not made safe by the map lookup. The map has no reference count, automatic size limit, least-recently-used eviction or protection from a raw address being reused after ownership mistakes.

[Unit definitions](../../../src/units/unit_types.cpp) load/free these images when binding or releasing script assets. CobScript::SetCob obtains GetCobChecksum and records it in interpreter state. [Script-state loading](units-cob.md) compares the persisted checksum against the currently bound script to reject incompatible state. The numerical pointer key itself is not the persistent identity written by that script save contract.

## 5. Static tree lifetime

The generated initializer (`0x4b2290`) constructs s_cobScriptCache before ordinary use and registers its destruction. Its generated destructor (`0x4b2340`) destroys map nodes at process shutdown. Keys/values are integers, so destroying a cache node does not free its referenced COB image. FreeCobScript and unit-definition cleanup remain responsible for those allocations.

The tree uses a header node storing root/leftmost/rightmost relationships and a shared black nil sentinel with reference counting for that template instantiation. Initialization installs empty header endpoints and size zero. Destruction erases nodes, frees the header and releases the shared sentinel when the last corresponding tree goes away. The map enforces unique keys and orders them with integer less-than. Ordering is for lookup/tree structure, not a gameplay priority or script execution sequence.

The emitted algorithms are supplied by the existing MSVC 5 SP3 MAP and XTREE headers under the main checkout's toolchain/msvc5-sp3/INCLUDE directory. Those existing files were read, without compiling or installing anything. int_map.cpp identifies the concrete emitted specialization and entry addresses; it intentionally does not duplicate the standard-library bodies as handwritten game source.

## 6. All fourteen emitted member responsibilities

| Address | Member | Concrete responsibility |
| --- | --- | --- |
| `0x4b2840` | begin | Return the header's leftmost node iterator, equal to end for an empty tree |
| `0x4b2850` | insert | Search by key, return existing iterator/false for a duplicate or insert/true for a new key |
| `0x4b2ac0` | erase(iterator) | Remove one node, repair tree/endpoints and return its successor |
| `0x4b2fb0` | _Erase | Destroy an entire subtree for bulk cleanup |
| `0x4b3000` | iterator/bool pair constructor | Copy the insertion iterator and success flag into a returned pair |
| `0x4b3020` | _Insert | Allocate/link a red node, construct its key/value and rebalance |
| `0x4b3310` | _Lrotate | Rewire a subtree around its right child, updating root/parents |
| `0x4b3370` | _Min | Follow left links to the minimum node |
| `0x4b33b0` | _Rrotate | Symmetric rewire around the left child |
| `0x4b3410` | _Buynode | Allocate a node and initialize parent/color fields |
| `0x4b3430` | _Lbound | First node with key at least the requested integer, otherwise header/end |
| `0x4b3490` | _Ubound | First node with key greater than the requested integer, otherwise header/end |
| `0x4b34f0` | iterator decrement | Move to predecessor; recognize end/header and select rightmost |
| `0x4b3590` | iterator increment | Move to right-subtree minimum or climb parents to successor/end |

Subscript in MAP inserts a default-initialized mapped integer and returns its value reference. LoadCobScript's assignment then overwrites that value, whereas GetCobChecksum can leave the new zero intact. Key erase uses lower/upper bounds to identify its range; unique-key mode means at most one matching cache node is removed.

## 7. Rebalancing, iteration and allocation assumptions

Insertion descends from root, checks the predecessor when needed to detect an equal key, and links a new red leaf only for a genuinely absent key. It updates leftmost/rightmost as appropriate. While a red parent violates the red-black invariant, it recolors a red uncle or rotates/recolors the relevant inner/outer configuration. The root is black on completion, maintaining bounded search height for valid operations.

Erasing a node with two children substitutes its in-order successor structurally rather than changing the ordering keys of unrelated surviving nodes. It repairs root/extreme endpoints and, when removing a black node, uses sibling colors, rotations and recoloring to restore black-path balance. It destroys/frees the removed value/node and decrements size. Bulk _Erase recursively clears right subtrees while walking down left links. No script instructions run during any of these operations.

Iterator movement assumes a valid node belonging to this tree or a permitted header transition. Erasing end, decrementing before begin or retaining an erased node's iterator is not checked by a game-level error return. Node allocation is delegated to the standard allocator and then used; the cache wrapper has no explicit allocation-failure rollback transaction around file loading plus insertion. Library _Lockit objects protect internal operations where present, but the surrounding HAPI load/checksum/fixup/free sequence is not enclosed in one cache-level lifetime lock. This reference does not infer general concurrent script-image ownership from those internal guards.

## 8. HapiBank initialization tail

HapiBank::InitBank (`0x4b3620`) writes its bank pointer null and returns this. It does not allocate an account list, open a file or release a previous bank. Calling it on an already owning object would discard the old pointer without cleanup. [Save-bank adapters](game-settings-campaign.md) call it on fresh objects; NewBank/OpenBank and CloseBank belong to the following HapiBank module.

## 9. Coverage and limits

All 20 owned entries were accounted for through game bodies, generated-lifetime source, the existing concrete library headers and the adjacent initialization body. COB image mutation, external checksum identity, missing-key insertion and ownership are covered independently of the interpreter itself. There is no cache save file, replicated pointer map, construction-progress renderer or unit-target rule in this group.

The reference uses static source tracing without builds, installs, matching checkers, game execution or copied source listings. It does not claim that a generic map index entry alone documents the cache's game behavior.
