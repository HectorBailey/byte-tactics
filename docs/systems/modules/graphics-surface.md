# Surfaces, screen presentation, textured polygons and shared strings

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `graphics/surface`, 51 annotated entries from `0x4c5d90` to before `0x4c9530`. Fifty reside in [surface.cpp](../../../src/graphics/surface.cpp); StringRef::Assign (`0x4c93b0`) is retained in [los_tables.cpp](../../../src/map/los_tables.cpp). The address group combines screen/image services, affine texture rasterization and the adjacent reference-counted StringRef implementation. These shared strings are distinct from the compiler's std::basic_string module.

## 1. Surface storage and ownership

A [Surface](../../../src/graphics/surface.h) is a 0x30-byte descriptor: width, height, byte pitch, pixel pointer, priority, color key, hotspot, inclusive clip rectangle and flags. Pitch can exceed width. The two-argument clip methods copy/set the stored rectangle; they do not constrain a newly assigned rectangle to allocation bounds.

AllocSurface (`0x4c69f0`) allocates one header plus width*height bytes, sets pitch=width, pixels immediately after the header, zero hotspot, priority 10000, key -1, raw/freeable flag0=1, flag1=0 and clip [0,0,width-1,height-1]. It does not clear the pixels or guard a null allocation before initialization. Width/height products and sign are trusted.

InitSurface (`0x4c6a60`) initializes the same descriptor using caller-supplied pitch and pixel pointer, but also sets flag0=1. That flag is therefore not proof that the pixel buffer was allocated immediately after the header. FreeSurface (`0x4c6ac0`) frees the descriptor address only when nonnull and flag0 is set; it does not separately free an arbitrary external pixel pointer. Callers creating descriptor views must preserve the intended lifetime and freeability rules.

ResetClipRect (`0x4c69c0`) resets inclusive full-size bounds. Surface::GetClipRect (`0x4c6ae0`) copies its rectangle to the supplied output and returns that output pointer; SetClipRect (`0x4c6b10`) directly replaces it. SetDisplayFieldE4/GetDisplayFieldE4 (`0x4c6b40`, `0x4c6b60`) store/read an otherwise uninterpreted display field. No additional drawing meaning is inferred from its provisional name.

## 2. Locking a drawing target

LockScreen (`0x4c5e70`) first checks the display override: when enabled it copies the override descriptor to the caller. Otherwise a GDI/offscreen-DC path copies the cached descriptor. Neither path invokes a DirectDraw lock. The DirectDraw path requires a back/drawing surface, sets DDSURFACEDESC size and calls Lock with flags 0x801. On success it builds a nonfreeable Surface using display dimensions, the returned pitch/pixels and display clip rectangle, and records the caller descriptor in the lock-entry array when count<10.

LockPrimary (`0x4c5ff0`) similarly copies the cached GDI descriptor or locks the primary DirectDraw surface with DDLOCK_WAIT. It does not use the override-first selection of LockScreen. HRESULT failure returns zero. These locks expose borrowed buffers; freeing the copied descriptor or retaining its pixels after unlock is not the intended ownership contract.

UnlockScreen (`0x4c5fa0`) ignores the descriptor argument for actual unlocking. It operates on the current display drawing surface only when neither GDI nor override mode is active, then decrements a positive recorded count. It returns zero if that required DirectDraw surface is missing, otherwise one. UnlockPrimary (`0x4c60d0`) unlocks the primary DirectDraw surface, or in the GDI path obtains a window DC, selects/realizes the palette and copies up to two supplied rectangles from the source DC using BitBlt. Those GDI rectangle widths/heights use right-left and bottom-top, unlike inclusive rasterization bounds elsewhere.

SetLockEntry (`0x4c5d90`) writes a surface pointer and primary/drawing flag at the current count when below ten. It does not increment the count. PopScreenLock (`0x4c5dc0`) only decrements a positive count; GetScreenLockCount (`0x4c5de0`) reads it. The DirectDraw LockScreen body also records at the current count without incrementing it. These bodies must not be described as a conventional automatically balanced push/pop stack.

UnlockAllScreens (`0x4c5df0`) loops while count>0. It reads the selector flag at index count but passes the pointer at index count-1 to its chosen unlock routine. If the unlock did not change count, it explicitly decrements to ensure progress. That flag/pointer index mismatch, plus the absence of a push increment in the observed helpers, are literal source contracts. The routine does not validate the recorded pointer or dynamically grow its ten-entry tracking scheme.

## 3. Override, restoration and page flipping

SetOffscreenSurface (`0x4c69a0`) enables override mode and retains the supplied pointer without allocating or validating it. SetRestoreSurface (`0x4c61f0`) retains a separate image used to repaint restored DirectDraw storage. SetPageFlipping (`0x4c61b0`) writes the requested option bit before checking whether a back buffer exists. It can return failure while leaving that bit enabled.

RestoreSurfaces (`0x4c6210`) returns zero for the GDI path. Otherwise it restores supplied primary/back interfaces in order, then locks the drawing target, copies the retained restore image and unlocks it. The local restore-copy path does not check LockScreen's result before drawing. RestoreScreen (`0x4c62c0`) checks display DirectDraw flag bit 1 and primary IsLost specifically against DDERR_SURFACELOST; on successful restores it copies the retained image and records the current back surface. It clears override mode unconditionally at the end.

FlipScreen (`0x4c63a0`) has four presentation paths:

| Conditions | Presentation |
| --- | --- |
| DirectDraw flag bit 1 clear | Under the MAIN graphics lock, copy override image to cached GDI surface, draw cursor, realize palette and BitBlt to the window |
| DirectDraw plus override mode | Require override dimensions equal screen dimensions, lock primary, draw cursor into override, copy it to primary, optionally restore saved cursor background, then unlock |
| Back buffer and flip option enabled | Invoke primary Flip with wait flag and return without checking its result |
| Other DirectDraw case | Translate client rectangle to screen coordinates and repeatedly Blt the drawing surface to primary |

The final path retries indefinitely for errors other than the recognized surface-lost code. On surface loss it attempts restoration, retrying when restoration fails and returning when it succeeds. It has no timeout or sleep here. Override-primary locking failure only triggers restoration for the recognized lost-surface HRESULT; other failures omit the presentation and release the MAIN lock.

The inline MAIN lock uses the shared atomic owner tag and event described in [palette services](graphics-palette.md). Tag equality allows nested entry; it is not a thread-ID-based recursive lock implemented in this module.

## 4. Cursor overlay and filling

DrawCursor (`0x4c67c0`) requires cursor-thread enabled, overlay enabled and a nonnull frame. It sizes the saved-background descriptor to the cursor frame, computes top-left from hotspot, copies background into that saved image using negative offsets, then draws the cursor frame. It does not choose cursor artwork or update pointer coordinates. FlipScreen controls where that overlay is applied and when saved pixels are restored.

FillSurface (`0x4c6890`) fills complete raw storage as height*pitch bytes, including padding, for a supplied flag0 surface, override surface or cached GDI target. Other cases invoke the display's DirectDraw drawing-surface color-fill operation. A supplied descriptor with flag0 clear does not redirect that driver operation to a surface-specific COM pointer; the global drawing surface is still used. It returns one unless that driver call reports failure. This fills the whole target rather than honoring its current clip rectangle.

## 5. Image copying and persistence

DrawSurface (`0x4c6b70`) handles three useful directions. Null destination plus valid source locks the screen and draws with the source hotspot subtracted. Supplied destination plus null source captures the locked screen using the requested offsets. Two supplied surfaces delegate directly with source hotspot adjustment. Two null pointers do nothing. DrawSurfaceKeyed (`0x4c6c50`) supplies a transparent index to the keyed blitter; it assumes a valid source, including before hotspot reads.

CopySurfaceRect (`0x4c6d20`), CopySurfaceRectKeyed (`0x4c6dc0`) and DrawTile (`0x4c6e70`) lock only when destination is null and delegate rectangle/tile memory work to [blitters](../../../src/graphics/blit.cpp). They do not add a general source/destination clipping pass. DrawTile passes source/rectangle/position pointers, but the [actual tile entry](graphics-blit.md) interprets those stack positions as integer X, integer Y and raw packed tile bytes. That incompatible interface remains unresolved in source; this wrapper is not evidence of a working general tile adapter. Rectangle interpretation for the other entries follows the called assembly, without inferred clipping.

SaveSurface (`0x4c6f10`) seeks box position zero, writes two 32-bit dimensions and then width bytes per row using the original pitch. It ignores write return values. Padding, palette, hotspot, clip, flags and priority are not serialized. LoadSurface (`0x4c6f80`) requires the eight-byte header, allocates a fresh packed raw surface and reads each row. A short row frees that allocation and returns null. It trusts dimensions/allocation and does not validate total box length before allocating. Restored descriptor defaults come from allocation, not saved presentation metadata.

## 6. Edge-index and interpolation helpers

InitSurfaceIndexFreeList (`0x4c7080`) initializes global previous/next arrays as a circular index list of the requested count. It does not allocate those arrays, validate their capacities or maintain a dynamic free count despite its provisional name. Two interpolation helpers use those rings, a shared table and separate A/B cursors, making them stateful and nonreentrant.

InterpEdgeUvFromScanline (`0x4c70d0`) follows previous links while the scan value is beyond the next table boundary. The B variant (`0x4c71f0`) follows next links. Each updates global low/high/span state and leaves output unchanged for a zero-height interval. For index cases 0..3 it assigns one output coordinate to zero/full extent and interpolates the other through [the integer multiply/divide helper](../../../src/util/math_4b70a0.cpp). The two variants have mirrored edge/corner mappings. An out-of-range index has no default output assignment; malformed rings or boundary order have no bounded iteration safeguard here.

## 7. Surface quadrilateral texture mapping

DrawFrameQuad (`0x4c7580`) requires a nonnull frame and four-point destination. A null Surface requests a screen lock and safely returns on lock failure. Optional source coordinates default to the entire raw frame rectangle. It finds top/bottom/extents, clips against the Surface clip rectangle, then walks the two ordered quad edges to fill a local table of 800 scanlines.

Each edge interpolates X and texture U/V in 16.16 with the 0xffff X-rounding bias used by [polygon fills](graphics-draw.md). Rows run from minimum Y up to but excluding maximum Y. DrawQuadRow (`0x4c7310`) interpolates between left/right texture coordinates across a positive-width span, advances them for clipped left pixels and clamps right X. The emitted count is right-left, excluding the endpoint. Winding/monotone convex geometry, bounded height and readable UV coordinates are preconditions; no perspective-correct division, source-UV clamp or transparent-color test is performed.

Widths 128/64/32/16 select specialized sampling blitters. Width 8 uses equivalent shift/mask row-address calculation; other widths multiply the sampled row by texture width. These are byte-index sampling operations. Compression and frame-child metadata are not decoded on this path, so input must already be an appropriate raw leaf texture.

## 8. Textured model polygons, depth and light

DrawTexturedPolygon (`0x4c8760`) consumes four XYZ vertices; DrawLitTexturedPolygon (`0x4c8bb0`) consumes four XYZ/shade vertices. Both accept optional UV coordinates or default to the whole texture. They test required pointers and projected bounds, clip Y to raw frame dimensions and produce up to 800 ten-int spans through paired rising-edge walks. UV, depth and optional light values interpolate affinely along edges and spans. They are not a general triangulator, perspective rasterizer or arbitrary concave polygon filler.

DrawTexturedSpan (`0x4c7a20`) and DrawLitTexturedSpan (`0x4c8020`) calculate interpolation increments by dividing by initial span width, then adjust starts for negative X and clamp the right edge. Polygon callers ensure a positive initial width. With a depth plane, a pixel draws when stored depth is at most incoming interpolated depth converted to a byte, then stores that byte. Equal-depth later work wins. Ordinary texture color is copied directly; lit texture color goes through shadeTable[(lightWhole*256)+sample]. No local light-row clamp or transparent-source-key exclusion is added.

Without a depth plane, both routines use raw sampling blitters or their equivalent loops. The lit routine's no-depth path does not apply its interpolated light at all. In both routines, texture width 128 falls through to the width-64 sampling call after the 128 call, overwriting the same destination with the second sampling interpretation. The source explicitly retains that fallthrough. Other width-specialized cases return normally. Depth-plane presence therefore changes more than occlusion: it also determines lighting use and avoids that no-depth fallthrough.

The polygon builders exclude their maximum Y and their spans exclude the right endpoint; their Y clipping to height-1 likewise leaves the last clipped scanline unfilled. Arrays, texture coordinates and fixed integer products are not protected against oversized/malformed inputs. These routines supply the actual hull texture pixels later subjected to [construction recoloring and attachment composition](graphics-model-render.md).

## 9. Reference-counted StringRef entries

The module's remaining eleven entries implement a shared byte-string handle with an integer reference count immediately before its characters. The empty singleton's count starts at one in [global data](../../../data/globals.csv), providing a retained base reference. This implementation is not std::string and has no atomic reference counting.

Default construction (`0x4c9180`) shares/increments the empty singleton. Copy construction (`0x4c91a0`) shares/increments the incoming block. Text construction (`0x4c91b0`) uses the singleton for null/empty input, otherwise mallocs count+characters+terminator and copies. Length construction (`0x4c9230`) only uses the singleton for null input; otherwise it copies up to the supplied length with strncpy and explicitly terminates. Negative/oversized lengths and malloc failure are not normalized or handled transactionally.

[Assign](../../../src/map/los_tables.cpp) (`0x4c93b0`) increments the incoming block before decrementing/freeing the old block, then stores the incoming pointer, which handles assignment between aliases including self-assignment. ReleaseRef (`0x4c9390`) decrements and frees when zero, without nulling the handle. Calling it repeatedly on the same released handle is not valid.

MakeLower/MakeUpper (`0x4c9290`, `0x4c9310`) call a copy-on-write uniqueness helper before the byte-string case conversion. Shared storage is copied to a new count-one allocation first; unique storage changes in place. AssignText (`0x4c93f0`) releases old storage before inspecting/copying the new C-string. Passing a pointer into that last old allocation can therefore read freed memory, unlike the refcount-safe handle assignment.

Append (`0x4c90b0`) returns unchanged for an empty other string. Otherwise it builds a fresh concatenation before releasing the old buffer, then stores the new pointer. Its allocation reserves the two lengths plus six bytes in the retained implementation, slightly more than the four-byte count plus terminator requires. SubString (`0x4c9490`) clamps start below zero and end above current length, returns an empty reference when start>=end, otherwise constructs an independent end-exclusive substring. The methods do not validate Unicode, locale-independent case rules, allocation failures or thread-safe sharing.

## 10. Coverage and integration

All 51 address-assigned entries were traced, including the shared-file StringRef assignment. Surface storage/persistence, screen lock/present/restore, cursor/fill/copies, global edge helpers, quad/depth/light mapping and adjacent strings are covered. StringRef has runtime clients such as [LOS table support](map-los-tables.md); that usage does not make its implementation part of the LOS module's count.

These services define image/save-box bytes and presentation ownership, not unit damage, construction payments or multiplayer authority. Display initialization provides usable COM/DC objects and palette resources, asset/model code provides valid raw frames/UVs, and higher drawing calls provide clipping/geometry assumptions. No builds, installs, source listings or runtime rendering tests were used for this documentation.
