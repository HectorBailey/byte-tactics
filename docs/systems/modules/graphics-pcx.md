# PCX and BMP images, screenshots, 3DO helpers and display exception cleanup

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `graphics/pcx`, all 21 entries `0x4caa40` through `0x4cbab0` in [pcx.cpp](../../../src/graphics/pcx.cpp). The address group contains PCX decoding/writing, palette conversion, screenshot naming, BMP output, shared 3DO loading/geometry utilities and an exception-display cleanup adapter. Describing only the image decoder would leave major model-loading responsibilities uncovered.

## 1. PCX decode contract

DecodePcx (`0x4caa40`) clears its four output fields: pixels, packed RGB palette, width and height. It accepts a complete 128-byte header when manufacturer byte is 10 and version byte is 5. It does not separately require the expected encoding, bit depth, plane count or bytes-per-line fields. Width/height come from unsigned-short maximum minus minimum coordinates plus one; inverted extents, overflow and allocation failure are not locally validated.

It allocates width*height bytes named PCX BODY and 768 bytes named COLOR MAP. It reads the palette from file length minus 768, seeks to byte 128 and decodes width pixels for each row into tightly packed output. The [file-length service](../../../src/util/hpi.cpp), `0x4bbd00`, returns stored/shared length or the underlying file length, not current cursor position. A nearby decoder comment suggests otherwise, but the called implementation establishes the actual end-relative palette seek. The expected trailing 0x0c palette marker is not checked.

The row helper treats bytes with their top two bits set as run headers, with a six-bit count and following repeated value. Other bytes are literals. An overlong run is shortened to the remaining row width; it does not continue into the next output row. Bytes-per-line padding from the header is ignored, so this is the game's packed single-byte image interpretation rather than a general PCX variant implementation.

Only the initial header read is checked. Palette and one-byte body reads ignore short returns, so truncated data can use uninitialized/stale bytes; a zero run does not reduce remaining width. There is no per-row EOF guard or whole-file consistency check. DecodePcx returns 1 after its loops even when a later read was short. On initial header rejection it returns zero with cleared outputs. Successful decode transfers both allocated buffers to the caller to free.

## 2. Loading a Surface or palette

LoadPcx (`0x4caf30`) opens for reading, validates the same header bytes, computes dimensions and allocates a [Surface](graphics-surface.md). Missing file/header rejection closes and returns null; a reported null image allocation also closes and returns null, subject to the allocator's own unchecked initialization behavior. It seeks to zero, invokes DecodePcx and closes the file.

Successful decode copies width*height bytes into the packed image. Optional outPalette expands all 256 RGB triples into four-byte red/green/blue/zero entries. It then frees decoded buffers. If the second decode failed, the code still attempts optional palette expansion, frees the image and returns its old pointer without nulling it. That branch can therefore return dangling storage, and optional output can dereference the cleared null palette first. The first validation usually establishes expected input, but it does not eliminate these source-level second-read failure paths.

LoadPcxPalette (`0x4cb080`) validates the header, resets the file cursor and calls the full decoder even though only colors are wanted. It ignores DecodePcx's return, expands/copies 1024 bytes to the caller, frees palette/pixels and returns 1. It does not install the display palette, avoid pixel-body decoding or provide a robust failure status after that second call. Header acceptance alone is not proof that either loader obtained complete image data.

## 3. PCX output and packed-row assumptions

WritePcx (`0x4cac40`) opens through HAPI_OpenFileAppend, whose [implementation](../../../src/util/hpi.cpp) uses a+b mode. It writes at the append position rather than explicitly truncating an existing path. The header is manufacturer 10/version 5/RLE/8 bits, zero origin, maximum coordinates width-1 and height-1, one plane and bytes-per-line equal to width. It copies the first 48 packed palette bytes into the header's small palette field and emits a trailing marker 12 plus the full 768-byte palette.

Each source row is exactly width bytes, with no input pitch parameter or even-byte padding adjustment. Equal-byte runs split into chunks of at most 63. A one-byte value whose top two bits are not both set can be literal; other values use an explicit run header/value pair, including high-valued single pixels. Runs never span output rows.

Open failure, short header write or short final palette write returns zero after closing any opened handle. Per-run writes and the marker write are unchecked, so those failures can still lead to a successful final return. Width/height must describe readable source rows; width zero with positive height still reaches the initial source-pixel read. The routine does not delete partial files on failure.

SaveSurfacePcx (`0x4caec0`) converts the display's retained 256 RGB0 entries to packed RGB triples and invokes WritePcx with Surface width/height/pixels. It ignores the writer result and the Surface pitch. A padded or external surface must therefore be packed compatibly or it can export incorrect row boundaries. The saved colors come from retained palette entries, not a readback of brightness-scaled hardware colors.

## 4. Screenshot selection and naming

SaveScreenshot (`0x4cb170`) requires display override mode, otherwise returning zero before enumeration. It uses the override image pointer; it does not independently lock/capture a DirectDraw surface. It inserts a backslash only when the nonempty directory lacks one, enumerates prefix*.pcx through HAPI and parses each matching filename suffix with atoi. The highest positive parsed number, starting from zero, determines the next name with minimum four-digit numeric formatting.

The routine then converts retained display palette entries and returns WritePcx's result. It does not fill numbering holes, atomically reserve the filename, create directories or prevent another writer from selecting the same next number. Since WritePcx opens in append mode, reusing an existing destination has meaningful consequences. Directory/prefix pointers and lengths are trusted; sprintf into a 260-byte buffer has no local bounds limit. Forward slashes are not used for the trailing-separator test.

## 5. Loading and rebasing shared 3DO geometry

Load3do (`0x4cb560`) loads one entire file block, returning null on load failure, then calls RelocateObject (`0x4cb4c0`) with the block base as the relocation delta. Nodes contain vertex/primitive counts, a special primitive index, XYZ origin, name/other offsets, vertex/primitive arrays and child/sibling links. Primitive records contain vertex indices and optional picture/name-related offsets.

RelocateObject adds the base to node pointers and primitive index-list offsets, conditionally rebases optional fields, recursively visits relocated siblings/children and sorts primitives at each node. The vertex and primitive-array fields and each primitive's index-list field are rebased without a null-offset test. This is an in-place conversion of trusted serialized offsets, not a copy into a validated geometry object graph. Counts, cycles, file bounds, indices and format version are not checked here. Returned geometry pointers remain tied to the caller-owned loaded block.

SortPrimitives (`0x4cb370`) moves the designated special face to entry zero when its index is not -1 and count is positive, then writes that index as zero. It bubble-sorts entries 1 through count-1 by ascending integer average vertex Y, leaving entry zero untouched even when no special index was declared. Primitive count zero divides by zero when an average is requested; index bounds are trusted. AveragePrimitiveY (`0x4cb2f0`) exposes the same average calculation independently. SwapPrimitives (`0x4cb330`) swaps the complete primitive records without changing vertex data.

This preparation explains [model rendering](graphics-model-render.md)'s face-zero convention and stable material/geometry references. It does not calculate per-frame piece animation or choose which COB piece is visible. Model assets loaded by [data-file services](../../../src/game/data_files.cpp) and [unit definitions](units-unit-types.md) pass through these helpers before instance state is built.

## 6. Coordinate conversion, height and bounds

MirrorObject (`0x4cb590`) negates every vertex X and Z and each node's X/Z origin, recursively including siblings and children. Negating both horizontal axes is the literal operation; treating its provisional mirror name as a reflection in only one plane would give the wrong orientation. Y is preserved. Shared loaded geometry is modified in place.

GetObjectHeight (`0x4cb5f0`) starts with best zero, checks each sibling's vertex Y plus node Y offset and combines each child's recursively measured height plus the current node Y. This accumulates parent translation through the hierarchy without runtime rotations or COB translations. Entirely negative geometry cannot lower the result below zero; a child subtree's zero result plus positive parent offset can contribute even without a higher vertex.

GetObjectBounds (`0x4cb650`) initializes both XYZ corners and starting offset to zero, then calls AddObjectBounds (`0x4cb6a0`). Each node adds its origin to the supplied ancestor offset and expands bounds with vertices only when vertexCount>2. A nonzero recursive argument visits the child with the accumulated offset and the sibling with the original parent offset. A zero argument measures only the supplied node. Starting corners at zero keeps the origin included even when all vertices lie on one side. This is static translated geometry, distinct from posed/projected screen bounds in the model renderer.

## 7. BMP writer lifecycle and headers

BmpWriter::Init (`0x4cb7c0`) nulls its FILE pointer and returns the object. Close (`0x4cb7d0`) closes a nonnull file but leaves the pointer nonnull; repeated closing without reinitialization is not safe. Open (`0x4cb7f0`) stores dimensions, opens wb and writes a 14-byte BMP file header, 40-byte information header and 256 four-byte palette entries.

Header fields specify signature BM, data offset 0x436, positive height, one plane, 8-bit uncompressed pixels, zero file/image-size fields and 3000 pixels-per-meter resolutions. Palette bytes are converted from retained RGB to BMP BGR. The fourth/reserved palette bytes are not explicitly initialized before the whole information block is written. No later finalization rewrites file/image size in these methods.

Open returns false for file-open or short-header writes, but a header failure leaves the open FILE retained for caller cleanup. It records dataOffset from ftell after successful header output. It does not allocate pixel storage or install a palette.

## 8. BMP row output and source padding

WriteRows (`0x4cb940`) derives stride by rounding the **source image width** to a multiple of four. It seeks to dataOffset+(writerHeight-rows-y)*stride and writes the selected source rows from last to first, using source pitch for row starts and srcY for selection. X and two other arguments are unused. It writes stride bytes directly from each source row instead of generating zero padding; readable source storage must include those bytes. It does not check consistency between source width and the width stored by Open. A failed seek/write returns false.

SaveBmp (`0x4cb9e0`) initializes/open/closes a writer around full-image output. It seeks to dataOffset because writerHeight equals the full image row count, then writes all rows bottom-up with the same rounded stride/source-pitch distinction. This zero additional seek is expected for its full-image call, not by itself evidence of a misplaced partial block despite a nearby suspected-bug comment. It closes the file on both ordinary success and failure and returns 1 or 0. A partial output file is retained on disk.

## 9. Exception cleanup adapter

ExceptionFilter (`0x4cbab0`) first calls ReportException. If a display exists, it acquires the shared MAIN graphics lock, clears the cursor-frame field, invokes [UnlockAllScreens](graphics-surface.md), releases that lock and calls ReleaseDirectDraw. A retained window is moved to non-topmost using SetWindowPos with no-move/no-size/no-activate flags. The function returns zero.

It does not save a game, reconstruct lost geometry, resume a faulted operation or guarantee every earlier allocation has been released. Its role in this module is making the display/cursor less obstructive around exception reporting and teardown. Actual exception policy and diagnostic output belong to the caller/reporting implementation.

## 10. Coverage and source limits

All 21 address-assigned entries are covered: six PCX/image/screenshot functions, nine model utility functions, five BMP writer/output methods and the exception adapter. Source-side file-service bodies were checked where a local comment or declaration could mislead the contract, including true file length and append mode. No image, screenshot, game or build was executed for this documentation.

The module transforms presentation assets and writes image files. It supplies static model geometry used downstream by unit instances, but does not implement combat, construction payments, dynamic piece posing, network transfer or gameplay saves. Exact hull dimensions and screenshot colors depend on loaded model/palette bytes. The behavioral limits above follow existing source without copying source listings into this writeup.
