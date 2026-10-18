# Extraction callbacks, wind callbacks and bitmap-cache allocation

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `units/unit_motion`, `0x437840` through the boundary before `0x437cd0`. All 12 annotated entry points in this range are in [unit_motion.cpp](../../../src/units/unit_motion.cpp). The module label is provisional: these functions update extraction/wind script inputs and implement an evicting memory arena. They do not implement a unit's complete steering or pathfinding loop.

The same source file physically contains later object-picture/shadow functions at 0x458810, 0x459170 and 0x45a790. Those belong to `graphics/model_render`, whose address range starts at 0x458160. Their use of this allocator explains the connection, but their presence in this file does not enlarge the current module's completion scope.

## 1. Metal extraction input

`UpdateMetalExtraction`, `0x437840`, does work only for a positive definition extractsMetal value. It walks the unit's footprint rectangle at its stored cell origin, calls GetMapCell for every cell and sums each existing cell's metal byte plus one. Missing cells contribute nothing.

The sum is accumulated in the signed whole half of a 16.16 value. The final rate multiplies that value by 1/65536 and the definition's extractsMetal, canceling the fixed-point scaling. This is the raw sum multiplied by the definition coefficient, subject to the narrow accumulator's range. The added one means a valid zero-metal cell still contributes one before the coefficient; an off-map missing cell does not.

It stores the resulting float at the unit's extraction field, then, if a script exists, starts SetSpeed with the unscaled summed whole value. Script animation speed input and the stored economic extraction rate are related but different values. The economic account update is documented in [economy](../03-economy.md).

For a nonpositive extraction definition the function returns without explicitly zeroing a previously stored rate. Correct type initialization and update calls must establish the appropriate state. This body does not independently test activation, construction completion, energy availability or competing extractors; those gates belong to downstream economy and callers.

## 2. Wind-driven animation inputs

`UpdateWindGenerator`, `0x437910`, requires a positive definition windGenerator value and global windEnabled. It starts SetDirection with the stored wind direction and SetSpeed with windSpeed shifted left four bits.

This body communicates environment values to COB. It does not add energy to a player account or multiply the speed argument by generator rating. The rating is a qualification here; its economic use belongs to the resource system. Unlike the extraction callback, this branch does not guard a null script pointer. The participating wind-unit definition/runtime combination must supply the expected interface.

## 3. Arena state and chunk ownership

CMemoryCache stores capacity, arena base, current allocation cursor, an internal handle and a separate scratch-image pointer. Arena records begin with an eight-byte header containing the address of an owning pointer variable and total chunk size. Returned data begins immediately after that header.

The owner is therefore a pointer to a handle location, not the unit/model itself. When the arena reuses a chunk, it can clear that handle if it still points to the chunk's data. Rendering code detects a cleared cached-picture handle and rebuilds the image. Consumers must not retain an untracked raw data pointer across operations that may evict its chunk.

The owning pointer's memory must remain valid until its chunk association is released or invalidated. Destroying a model/state object while leaving its handle address registered would let later cache eviction write through a stale address.

## 4. Initialization and cleanup contracts

`ClearPointers`, `0x4379a0`, clears base and the internal handle, returning the cache object. It does not initialize every member, allocate scratch images or notify old chunk owners. It is an initialization helper, not a safe substitute for all teardown steps on a populated cache.

`InitCache`, `0x4379b0`, frees an existing arena, allocates the requested byte size, records capacity, points the cursor at the base and writes one unowned chunk spanning the arena. It returns 1 without an independent allocation-failure recovery branch or minimum-size check. Its caller must provide enough space for a chunk header and follow the allocator's allocation contract.

`FreeCache`, `0x437a00`, frees an existing arena and clears base. `FreeBuffer`, `0x437a20`, frees base without clearing it or testing it locally. These are distinct lifetime operations; invoking the latter and then using a still-nonzero base as proof of valid storage would be incorrect. Neither walks the owner registry to clear every external cached pointer in these bodies.

## 5. Allocation, reuse, wrapping and eviction

`AllocHandle`, `0x437a30`, accepts the address of a handle and requested payload bytes. Nonpositive requests clear the handle and return zero. It adds eight bytes for the header. An existing allocation whose stored chunk size already satisfies that total returns success without moving it.

If the requested total exceeds capacity, it clears the handle and returns zero. Otherwise it allocates at the cursor, wrapping to the arena base when the requested extent would pass the end. On wrapping, it first traverses remaining tail chunks and clears matching live owner handles.

Starting at the chosen allocation point, it consumes adjacent chunks until their combined size satisfies the request. For each consumed header, a nonnull owner pointer is cleared only if that owner still points to this chunk's data. This comparison avoids erasing a handle that has since been redirected elsewhere.

When the unused remainder exceeds eight bytes, it creates a free remainder header and leaves the cursor there. A remainder of eight bytes or less is absorbed into the allocation; the cursor advances to the following chunk or wraps to base. Finally it records the supplied handle address, total allocated size and returned data pointer.

This is cursor-driven eviction, not an LRU cache that ranks pictures by recent use. It relies on a valid, traversable partition of positive-sized chunks. It does not compact live payloads or return permanent object storage.

`GetHandleSize`, `0x437b30`, returns zero for a null payload pointer and otherwise reads the preceding size field. That stored size includes the eight-byte chunk header and possibly an absorbed small remainder. It is not exactly the caller's original payload request.

## 6. Bitmap wrappers

`AllocBitmap`, `0x437b50`, requests a 0x18-byte GAF-frame header plus width times height pixel bytes. It initializes dimensions, offsets, transparency value 1, uncompressed state and the payload pointer; the pixels start filled with 1. Its scratch-plane pointer is zero.

`AllocTwoPlaneBitmap`, `0x437be0`, requests the same header followed by two width-times-height planes. The first plane is initialized to transparency value 1, and the scratch plane to zero. Both wrappers return zero when handle allocation fails or leaves a null result, otherwise 1.

The wrappers can reuse a sufficiently large existing chunk and reinitialize its frame/pixels. They do not independently check width/height multiplication overflow or whether dimensions fit every narrowed frame-header field. Callers supply measured, valid image bounds.

One-plane versus two-plane allocation matters to rendering consumers because the presence of scratch data affects available compositing/depth behavior. It does not change a unit's build percentage. The [naval construction rendering reference](../../naval-systems.md) describes how the later model renderer interprets construction and cached-image state.

## 7. Flush and owner-address release

`FlushCache`, `0x437c80`, requests capacity minus eight bytes through the cache's internal handle. This consumes the arena as one allocation, evicting ordinary matching owners through the allocator's existing machinery. It does not free the backing memory.

`ReleaseHandle`, `0x437c90`, walks every chunk and clears its owner field when that field equals the supplied handle-address value. It does not clear the caller's pointer or immediately combine/free that chunk. This unregisters the external pointer location so later eviction will not write through it.

The difference is essential: flushing invalidates tracked pictures while preserving cache storage; releasing an owner address protects an external object's lifetime; freeing destroys the arena itself. They are not interchangeable forms of deletion.

## 8. Dependencies and completeness

These callbacks depend on map cell metal, definition coefficients, global wind and COB execution. The arena depends on the allocation layer and GAF frame layout. Later model-picture and shadow code builds images in its storage, tests evicted handles and draws/rebuilds accordingly. Cache bytes are transient presentation state, with no save-bank or network-packet format in this module.

All 12 entry points are covered. For extraction animation mismatch, compare the raw SetSpeed sum with the coefficient-scaled economic rate. For wind animation mismatch, inspect windEnabled and COB callback handling. For a disappearing cached image, inspect handle invalidation and renderer rebuild conditions; for a stale pointer, inspect owner-address release and arena teardown ordering. Movement steering remains a separate system despite this module's provisional name.
