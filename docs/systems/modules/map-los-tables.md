# Sight ray tables, quadrant expansion and storage support

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `map/los_tables`, 50 annotated entries in the address interval `0x432ba0` to `0x4349f0`. Sources: [table methods and support](../../../src/map/los_tables.cpp), [top-level loading](../../../src/map/los_tables_433130.cpp), [table resizing](../../../src/map/los_tables_433270.cpp), [nested-vector lifetime/copy](../../../src/map/los_tables_433a80.cpp), [outer erase](../../../src/map/los_tables_434360.cpp) and [public declaration](../../../src/map/los_tables.h). The unrelated StringRef::Assign at 0x4c93b0 is physically present in the main file but belongs to its own address-assigned module.

## 1. Data model and responsibility

LosTables owns a vector of tables. Each table owns a vector of lines, and each line owns a vector of four-byte points, two unsigned shorts. One input line expands to four rotated/sign-adjusted lines. These are ordered offset sequences used by terrain-aware visibility traversal, not precomputed lists of currently visible units.

The module does not choose a player's radar range, test target cloak, maintain explored/live sight counts or inspect terrain elevations. [Sight consumers](../../../src/map/line_of_sight.cpp) and their [add/remove paths](../../../src/map/line_of_sight_481d50.cpp) walk the offsets against terrain-height bands and player grids. Loaded table size/ordering therefore matters to consumers even though no visibility mask changes during parsing.

Tables are global shared geometry, not one separate asset per player. Unit sight distance selects a table through caller arithmetic. Actual named-unit sight distance comes from the definition and runtime state, not from these files alone.

## 2. Loading the table set

LoadLosTables (`0x433130`) builds gamedata/los.TDF and loads it through TdfFile. On success it selects TABLEINFO and reads numtables, default zero, into a signed short. It resizes the outer vector and loads indices zero through numtables-minus-one. It unloads the parser after the successful file load, whether TABLEINFO existed or not.

File-load or TABLEINFO-selection failure does not explicitly clear existing tables in that branch. This permits prior state to remain on repeated load. Numtables zero clears/resizes to empty through the valid TABLEINFO path. Negative short counts convert to unsigned for resizing, without a local rejection, so malformed values can request a huge allocation. Valid content and allocation services are prerequisites.

ResizeTables (`0x433270`) exposes ordinary vector resize with a default empty table. Growing preserves old tables and appends empty ones; shrinking destroys removed table/line storage. It is not equivalent to regenerating every retained line from scratch.

## 3. Table numbering and line groups

LoadLosTable (`0x433380`) receives a zero-based table parameter, constructs TABLE followed by parameter-plus-one and resets parser traversal before selection. A missing section leaves that table's existing content unchanged. A selected section reads signed-short numlines, default zero, and sizes the table to four times that value.

For each input index i, it loads the same line field into positions i, numlines+i, twice-numlines+i and three-times-numlines+i, using modes 0,1,2,3 respectively. Storage is therefore four complete directional blocks, not four adjacent points per ray. Short arithmetic for counts/multiples can truncate before loop/access behavior; large malformed counts are not range-checked.

GetLosTable (`0x433500`) interprets its public integer argument as **one-based**, narrows n-minus-one to short and indexes unchecked. GetLosTableCount (`0x433520`) returns outer size. Internal loading compensates by passing table-plus-one. Some live sight consumers pass clamped sight-distance/32 expressions that can reach zero, while this accessor subtracts one. This is a directly observed interface discrepancy; it must not be hidden by rewriting all indexing as conventionally zero-based. The broader sight reference must preserve the caller/accessor relationship when explaining its consequences.

ResizeLines (`0x4335f0`) uses n*4 empty line objects. GetLosLine (`0x4335e0`) indexes a line by a signed-short value without bounds validation; GetLosLineCount (`0x4335c0`) returns total stored lines including all four blocks.

## 4. Input syntax and exact transformations

LoadLosLine (`0x4336f0`) looks up line followed by input-index-plus-one into a 512-byte buffer. It tokenizes on comma or space with strtok. The first token is point count n, parsed by atoi and narrowed to short. It resizes the line, then consumes two tokens per point.

| Expansion mode | First stored component | Second stored component |
| --- | --- | --- |
| 0 | First input value | Negative second input value |
| 1 | Second input value | First input value |
| 2 | Negative first input value | Second input value |
| 3 | Negative second input value | Negative first input value |

Negatives are stored in unsigned-short representation. They remain 16-bit wrapped offsets; interpreting them later as signed displacement depends on the consuming arithmetic/casts. This representation is not a prohibition against rays extending left or upward.

A missing field explicitly resizes the line to zero. A present string with no first token returns without that resize, preserving previous line content. Count zero yields an empty line. Missing coordinate tokens are passed directly from strtok into atoi without a local null check. Negative counts and oversized/truncated input are not rejected by a dedicated schema validator. Unknown mode can leave resized points without coordinate population, although the normal loader uses only 0..3.

The routine preserves point order exactly. It does not sort steps by distance, deduplicate offsets, insert an origin, construct a circular disk or verify that successive points form a continuous ray. Those properties must come from los.tdf content.

## 5. Traversal interface and visibility interaction

GetLosLineStepCount (`0x4339c0`) returns vector size. GetLosLineStep (`0x4339e0`) takes an unchecked short index and writes two unsigned-short values to its output pointers. It does not provide elevation, distance or visibility status with the point. Caller declarations sometimes view output storage through wider pointers, so the actual two-byte writes and subsequent short-coordinate casts matter.

Terrain-aware consumers select lines, add offsets to a source cell and compare lower/upper terrain bands relative to source elevation. They use step ordinal in slope/horizon comparisons, so ordering is semantically important. The table loader supplies geometry only; a blocker/visible decision is a consumer result.

Other sight paths use GAF masks rather than these ray tables. Having a LosTables object loaded does not mean every visibility update uses it. The live map mode and caller branch determine which implementation runs. Sonar/radar/contact rendering likewise cannot be inferred solely from the existence of this TDF asset.

## 6. Nested-vector ownership and teardown

FreeTables (`0x4330b0`) explicitly destroys the outer vector, recursively destroying table/line storage. FreeLines (`0x433540`) destroys one table's line vector. These are destructor-shaped services, not guarded idempotent clear calls that leave a fully constructed container contract for arbitrary future use.

The module includes point-vector construction/destruction, size, insert, erase, copy, assignment, deallocation and trivial destruction operations. A point is four trivial bytes. A line owns its point allocation; copying a line allocates/copies its points rather than sharing one raw buffer. A table owns its line vector, and copying/erasing at outer levels invokes corresponding inner assignments/destructors.

Separate [nested-vector source](../../../src/map/los_tables_433a80.cpp) includes destructor 0x433a80, erase 0x434020 and copy constructor 0x4344e0. [Outer erase 0x434360](../../../src/map/los_tables_434360.cpp) shifts retained tables and destroys the removed tail. DestroyPoint (`0x434430`) does no work. DestroyLine (`0x434440`) destroys the owned inner vector. StoreDwordIfDst (`0x4345c0`) copies one four-byte point only for nonnull destination. Scalar deleting destructor 0x4349f0 handles a line-owning element with the usual conditional deallocation shape.

Size/insert/erase/_Destroy/_Ucopy/allocator entries throughout 0x433a10..0x4349c0 support these ownership operations. They do not independently cast sight rays, apply player visibility or impose a maximum sensor distance. Empty/trivial destructors belong to storage support, not missing simulation logic.

## 7. Adjacent string-pair support

The beginning of this module also contains support for reference-counted string vectors and string-plus-int pairs. String-vector destructor/size are 0x432ba0/0x432be0. Deleting wrappers 0x432c00/0x432c20 release a StringRef then optionally free its wrapper. Pair uninitialized copy/construct at 0x432c40/0x432cf0 use string copy construction, preserving reference counts.

AssignRange (`0x432c80`) assigns each destination's string handle and int. AssignRangeBack (`0x432cb0`) performs the same operation backward for overlapping-tail movement. AssignPair (`0x432d20`) assigns one pair. These support name-keyed tables elsewhere, including damage overrides; they are not string-based sight offsets.

StringRef::Assign is a dependency at 0x4c93b0 and is deliberately excluded from this group's count despite sharing the physical file. It increments the incoming reference, decrements/releases the previous allocation, then stores the new pointer. Its separate module reference owns the complete string contract.

## 8. Unit-definition sorting support

SortUnitTypes (`0x432d40`) is an STL quicksort partition phase over packed UnitDef records using a caller-supplied comparator. Above the STL threshold it chooses a median of first/middle/last, partitions, recursively processes one side and loops on the other. It stops when partitions are small; it does not finish each small segment with insertion sort in this body.

InsertionSortUnitTypes (`0x432fb0`) separately performs insertion sorting, copying whole definition records and using the same comparator convention. These two entries are distinct phases of a larger sort workflow. Neither assigns new unit IDs, derives sensor ranges nor sorts sight-ray points. Caller comparator and phase order establish the final ordering.

## 9. Persistence, failure and completeness

This group has no independent disk-save or network serializer. It rebuilds shared geometry from gamedata and frees it through lifecycle teardown. Per-player sight counts, explored bits, active sensor contacts and saved Mapping/Data belong elsewhere.

All 50 address-assigned entries are accounted for: table loading/access, four-way line transforms, nested container operations, string-pair helpers and unit-definition sort phases. Explicit limits include unchecked/one-based access, short count narrowing, malformed token handling, stale content on selected failure paths and reliance on ordered asset rays. This is static source documentation without builds or game execution.
