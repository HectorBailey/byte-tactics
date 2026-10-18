# GAF frames, animation references, sprite drawing and compression

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `graphics/gaf`, all 38 entries from `0x4b7e60` to `0x4ba1b0` in [gaf.cpp](../../../src/graphics/gaf.cpp). These services load/rebase animation assets, maintain sequence references, draw sprite trees, manipulate model depth images, provide lens effects and encode compressed sprite rows. Model construction rendering consumes these primitives but remains separately documented in [model_render](graphics-model-render.md).

## 1. Frame representation and loaded-file ownership

The [shared frame header](../../../src/graphics/gaf_frame.h) is 0x18 bytes: unsigned-short width/height, signed-short hotspot offsets, transparent color index, compression byte, layer count, blend-kind byte, reserved word and two pointers. For a leaf frame, plane0 holds indexed pixels or compressed runs. With a nonzero layer count it instead points to an array of child-frame pointers. Plane1 is optional scratch/depth data; specialized lens frames use their storage differently. A count-bearing frame is a draw tree, distinct from a timed sequence of frames.

LoadGaf (`0x4b8c60`) loads the whole asset with HAPI_LoadFile. It converts entry offsets to pointers in place, then each sequence frame offset, its plane0 offset, and nested child-frame and child-plane0 offsets. The supported nested-layer relocation is explicit, not an arbitrary recursive validator. It returns the allocated file block, or null when loading failed. Entry/frame pointers refer inside that block and remain valid only while it is retained. The [asset owner](../../../src/game/data_files.cpp) retains loaded GAF blocks and later frees them; this module does not separately allocate or free each loaded leaf.

No file signature, byte length, count/offset bounds or cycle validation is performed before these fixups. The entry count is read as a signed short for iteration. Calling LoadGaf fixups twice on an already rebased buffer would not be a valid operation. FindGafEntry (`0x4b8d40`) scans the loaded entry pointers case-insensitively by their names, returning null for absent file/name; it does not copy the result.

## 2. Sequence lookup and timing

A sequence contains an unsigned-short frame count and eight-byte entries whose first word-sized pointer field identifies a frame and whose duration is read at the corresponding +0x2c-based offset. A live reference stores current unsigned-short index, countdown, loop flag and sequence pointer.

| Entry | Contract |
| --- | --- |
| GetGafSequenceFrame, `0x4b7ee0` | Null sequence returns zero; otherwise indexes directly with the retained reference index |
| SetGafSequenceFrame, `0x4b7f00` | Sets index if sequence exists and count exceeds the supplied signed integer; negative integers are not rejected before narrowing |
| GetGafFrame, `0x4b7f30` | Rejects negative/out-of-count frame indices, but reads the count before its later null-pointer test |
| GetGafFrameCount, `0x4b7f60` | Reads the count directly, without a null guard |
| GetGafFrameDuration, `0x4b7f70` | Returns current duration, or 0xffff for a null sequence; no current-index validation |

InitGafSequence (`0x4b8b30`) uses the requested index when it compares below count, otherwise zero, copies the sequence loop byte and loads the duration. It requires a nonnull sequence; the signed-index comparison also permits a negative request before conversion to the unsigned-short field.

StepGafSequence (`0x4b8b90`) advances one unit of time. A countdown below two causes a frame advance; reaching the end wraps when looping, otherwise clears the sequence pointer and returns 1. It loads the new duration and returns 1 whenever it changes frames. A larger countdown is decremented and returns 0. A null sequence returns 0. A one-frame sequence can therefore loop or expire through this function.

AdvanceGafSequence (`0x4b8bf0`) uses a signed-short countdown/step interpretation. It does nothing for null sequences or counts at most one. Otherwise it subtracts the step, repeatedly advances while the countdown is nonpositive, and adds each new duration to preserve leftover time. Nonlooping exhaustion clears the pointer. Negative steps can increase remaining time; signed-short narrowing can wrap. A looping sequence with all zero durations cannot make that while loop progress to a positive countdown. Neither stepping API itself schedules global animation updates or draws a sprite.

## 3. Rectangles, hotspots and screen ownership

ClipRects (`0x4b7e60`) clips one inclusive rectangle to bounds while moving the corresponding source rectangle edges by the same amounts. Empty results are recognized by drawing callers through right>=left and bottom>=top checks. Ordinary leaf placement begins at requested X/Y minus frame hotspot offsets, with width/height minus one defining inclusive right/bottom edges.

Most draw wrappers accept a destination surface or null. A null destination asks LockScreen to populate a temporary surface, and a final pointer comparison unlocks only that temporary. A failed lock generally leaves the destination null while later draw code can still use it; these functions do not all turn lock failure into a safe no-op. Their wrappers rely on normal display lifetime and screen-lock contracts. They do not acquire unit visibility, choose world coordinates or synchronize animation time themselves.

SurfaceFromFrame (`0x4b8a80`) creates a surface view over plane0 with pitch equal to width, zPriority 10000, colorKey -1, frame offsets and initialized surface flags/clip bounds. FrameFromSurface (`0x4b8ae0`) creates the inverse nonowning frame view, but overwrites width with surface pitch after first copying surface width. It sets compression/layers/kind zero and color key 255, without clearing every remaining header field such as plane1. Padded surface rows therefore change the frame width through this adapter. These conversions do not allocate or transfer ownership.

## 4. Ordinary, opaque and blended drawing

DrawFrame (`0x4b7f90`) visits tree children in stored order. A child's nonzero blend-kind selects DrawFrameBlended; other children recurse normally. Uncompressed leaves use a color-keyed rectangle blit after clipping; compressed leaves invoke the run decoder. Transparent source indices leave the destination unchanged in the keyed path.

DrawFrameOpaque (`0x4b8150`) copies all pixels of an uncompressed leaf with the ordinary rectangle blitter. Its tree branch still dispatches normal/keyed or blended child drawing, and its compressed branch still uses the run decoder. Opaque in the function name does not remove transparent skip commands already encoded in compressed data or force every child through an opaque leaf path.

DrawFrameBlended (`0x4b8500`) operates only when display flag bit 5 indicates the alpha table exists. Trees recurse through blended drawing for every child. Leaves invoke uncompressed or compressed blend blitters using the [256 by 256 alpha table](graphics-palette.md). Source and existing destination indices jointly determine the output, normally an approximately 50/50 palette blend; there is no per-pixel RGBA channel in the frame representation.

DrawFrameLit (`0x4b8310`) requires display flag bit 7, the light table. Its tree branch calls DrawFrameBlended for each child rather than propagating the requested light row. For a compressed leaf it passes lightTable+(paletteRow<<8) to the one-input remap decoder. The uncompressed branch instead calls the generic two-input BlitRectBlended with paletteRow in the color-key argument and the light table as its table pointer. The [blitter implementation](../../../src/graphics/blit.cpp) uses source and destination indices for that operation. Thus this branch is not equivalent to simply remapping source colors through one chosen 256-byte row, and input indices can exceed the ordinary 32-row light-table assumptions. The reference retains the actual call distinction rather than imposing an idealized common lighting contract.

DrawFrameShadow (`0x4b8ec0`) also requires bit 7. It recurses through shadow drawing, skips null leaf data and selects compressed/uncompressed shadow blitters with the light table. Those blitters use sprite values as remap-row controls for the destination, not as ordinary visible source colors. A shadow frame therefore acts on pixels already behind its mask.

## 5. Gray masks and checkerboard erasure

DrawFrameGray (`0x4b86e0`) requires display flag 0x100 and an uncompressed frame. It creates/clips a surface view and calls BlitRectRemapDest with the gray table. The [blitter](../../../src/graphics/blit.cpp) maps the **existing destination** through grayTable wherever the source is non-key. It does not necessarily draw gray versions of the source sprite's own colors. Tree children recurse through the same effect.

EraseFrameDithered (`0x4b88d0`) similarly accepts uncompressed frames and recurses over children. At a leaf it clears destination pixels to index zero wherever the source is non-key and (destination row+column+parity) selects the requested checkerboard half. Other pixels survive. This is a masked eraser, not alpha blending or an animated visibility detector.

Both wrappers test frame->flag9 before their later null check. A null frame can therefore fault even though another branch appears to guard it. Compressed input is skipped rather than decompressed into scratch for these effects.

## 6. Scaling and supersampling

DrawFrameScaled (`0x4b9a50`) multiplies dimensions and hotspot offsets by supplied double X/Y scales, truncates to integers and skips nonpositive output width/height. It clips the scaled destination, converts clipped offsets back to source positions and advances source X/Y with 16.16 increments. The nearest sampled non-key pixel is copied directly. Children with blend-kind use DrawFrameScaledBlended, others recurse normally.

DrawFrameScaledBlended (`0x4b9740`) uses the same size, hotspot, clipping and nearest-source sampling, but alpha-table blends every non-key sample; all children use its blended path. Unlike DrawFrameBlended, this wrapper does not locally test the alpha-table availability flag. Both scaling wrappers interpret plane0 as raw pixels without branching on compression and require valid nonnull frame input. Negative scales do not implement mirroring through these loops.

DownsampleFrame (`0x4b95a0`) combines each source 2 by 2 block into one destination pixel using three alpha-table lookups: blend each horizontal pair, then blend those two results. It assumes compatible doubled source dimensions and a valid table; it does not clip or skip key pixels specially. [Model supersampling](graphics-model-render.md) invokes it for color while separately selecting secondary-plane samples. The visible result is palette-mediated blending, not an arithmetic average of four stored index numbers.

## 7. Allocated frames, clearing and depth composition

AllocFrame (`0x4b8da0`) allocates header plus width*height bytes and returns null on failure. It sets dimensions, raw plane0 pointer, null plane1, zero offsets/compression/layers/kind, but leaves pixel contents, transparent key and reserved field uninitialized. AllocDepthFrame (`0x4b8e00`) allocates two width*height byte planes after the header and sets similar metadata. It dereferences its allocation without a null check and does not clear either plane or initialize the key. Callers must establish those values before meaningful transparent/depth drawing.

ClearFrame (`0x4b8e50`) only acts on uncompressed frames, filling plane0 with the low byte of its color argument and zeroing plane1 when present. It does not change the transparent-key field and does not traverse layer trees. ZeroFramePixels (`0x4b96a0`) instead changes only non-key color pixels to zero, preserving the silhouette's transparency.

DrawFrameDepth (`0x4b90a0`) aligns source/destination hotspots plus supplied offsets. Negative starting X/Y offsets cause an immediate return. For each source pixel, a non-key color replaces the destination when destination depth is at most source depth plus supplied level. It stores that adjusted depth as a byte. Source pixels and depth advance together. It does not clip the right/bottom edge, validate plane1 or clamp level arithmetic, so callers must provide sufficiently sized raw two-plane frames. This is the parent/child occlusion primitive used for unfinished products inside factories.

TintFrameBelow (`0x4b96e0`) maps non-key pixels through blueTable when their plane1 value is **at most** the supplied byte threshold. CutFrameBelow (`0x4ba1b0`) instead replaces pixels at or below that threshold with the frame's key. Both walk raw planes without compression/layer/null guards and do not modify the depth values. [Model rendering](graphics-model-render.md) chooses tint versus cutoff according to ownership, contact and sea-level state; these helpers themselves know nothing about ships or sonar.

CutOutFrame (`0x4b9d70`) makes destination pixels transparent wherever an offset source has a non-key pixel. It accounts for hotspots and vertical overlap but clamps copy width to destination width without subtracting a positive destination column. That can overrun a destination row. A source start beyond its width can produce negative copy count, and the loop tests count!=0 rather than count>0. The routine therefore has stronger valid-overlap preconditions than a fully clipped sprite compositor. Its shadow-building use must be understood with these literal limits.

## 8. Lens frames and saved background

BuildLensFrame (`0x4b91b0`) allocates specialized storage by asking the depth allocator for double width, then halves the visible header width. Plane0 becomes width*height 16-bit displacement entries; the second allocation region supplies background and output scratch bytes. It sets hotspot to half width/height and computes radial displacement inside radius width/4. Outside that radius it stores sentinel 32000. The lens argument affects magnification through its denominator arithmetic. Invalid zero/small parameters are not locally normalized; the lower allocator can dereference null before this function's later allocation check.

DrawLens (`0x4b9360`) temporarily swaps planes to capture the destination rectangle into scratch, restores them, and walks the signed-short displacement map. For each cell it samples captured background at that cell's relative displacement, or writes the frame key for sentinel 32000. It temporarily points plane0 at the constructed output and calls DrawFrame, then restores the original map pointer. It does not persistently distort the source geometry or alter unit position.

GrabBackground (`0x4b94c0`) builds a raw surface view over a supplied frame's plane0 and copies the requested background rectangle into it. The frame is caller-owned capture storage. Rectangle copying supplies applicable screen clipping; the lens displacement sampling itself does not check every resulting index.

## 9. Compressed rows and transparency runs

CompressFrame (`0x4b9e60`) processes raw source rows through CompressRow, prefixing each encoded row with its unsigned-short byte length. With null output it only measures total bytes; the same call with allocated output emits the representation. Width/height, key and raw plane0 are required. Row lengths narrow to 16 bits, and the interface receives no output capacity.

CompressRow (`0x4ba000`) first detects an entirely key-colored row and returns length zero for it. Otherwise it rereads the row and uses literal/repeat state, a global literal history buffer and a global output-byte counter. EmitCopyRun (`0x4b9ed0`) emits literal chunks of at most 64 pixels. EmitRepeatRun (`0x4b9f50`) emits key skips of at most 127 pixels without a value byte, or non-key repeat chunks of at most 64 with one value byte. Command low bits distinguish these forms; the high bits encode their count. The decoder's row-length prefix permits a fully transparent zero-byte row without an end-marker pixel stream.

Run selection detects equal colors and flushes accumulated literals around repeats/key spans. Null output still updates measured size. These helpers share globals and provide no local reentrancy/thread-safety mechanism; interleaved compression could corrupt shared history/count state. They do not compress plane1, install the compressed bytes into a frame, change frame->flag9 or allocate destination memory. Shadow building performs those ownership/format steps separately.

## 10. Coverage and interactions

All 38 address-assigned entries reside in gaf.cpp, including DrawFrameLit, DrawFrameDepth and DownsampleFrame placed out of address order. Coverage includes paired clipping, five sequence lookup/accessors, three timing methods, loading/search, surface adapters, ordinary/effect draw wrappers, two scaling wrappers, frame allocation/clearing, depth/water/cutout helpers, background/lens operations, downsampling and four compression methods. These are functional areas rather than additional counted modules.

The module does not select combat targets, pay construction costs, advance orders or serialize a game. [Asset loading](../../../src/game/data_files.cpp), [model rendering](graphics-model-render.md), [palette services](graphics-palette.md), [surface infrastructure](../../../src/graphics/surface.cpp) and [blitters](../../../src/graphics/blit.cpp) supply or consume its data. Loaded assets determine frames, hotspots, durations and layer ordering; source fixes their interpretation and the limits above. This writeup uses source inspection only, without copied source listings, builds or gameplay execution.
