# Assembly blitters, sprite decoding and indexed pixel operations

[Systems manual](../index.md) | [Roadmap](../../doc-writeup-roadmap.md)

## 1. Scope and ownership

The graphics/blit group contains 28 callable entries in [blit.cpp](../../../src/graphics/blit.cpp), from BlitSurface at `0x4cbbe0` through BlitSpan16 at `0x4cd962`. Six FUNCTION markers alone do not describe its scope: another 22 ENTRY markers identify routines embedded inside the assembly bodies. These are hand-written x86 routines with stack-based, caller-cleaned arguments, including several legacy entries with additional register assumptions.

They operate on indexed, one-byte pixels. They do not allocate images, load palettes, lock display surfaces, decide unit visibility, issue game commands or own a simulation lifetime. [Surface services](graphics-surface.md), [drawing wrappers](graphics-draw.md), [GAF rendering](graphics-gaf.md), [fonts](graphics-font.md) and model rendering establish their inputs. In particular, constructing a ship does not invoke a separate construction-aware blitter. Construction geometry and coloring are chosen by [model rendering](graphics-model-render.md); these routines copy or transform the resulting pixels.

The normal Surface prefix supplies integer width, height and pitch at offsets 0, 4 and 8, followed by its pixel pointer at offset 12. Pitch is the byte distance between rows and need not equal width. Some legacy entries use a different descriptor, detailed below. A local C++ declaration or a source overview comment does not override the fields actually read by assembly.

## 2. Clipping, coordinates and memory prerequisites

Ordinary pixels and rectangles use integer coordinates. Rectangle-copy and fill sizes are inclusive: right-left+1 and bottom-top+1. Source-rectangle copies place that region at the first two coordinates of the destination-position argument; they do not use that argument's right and bottom fields as scaling bounds.

BlitSurface and BlitSurfaceKeyed clip against the complete dimensions of both images. BlitLine clips against the complete destination dimensions. Neither implements the Surface's separate application clip rectangle. Higher wrappers can apply that rectangle first. Most other entries trust coordinates supplied by their callers and perform no destination-bounds check. Compressed decoders can crop an encoded source region, but have no destination dimensions with which to establish safe output bounds.

Valid pointers, positive dimensions where required, compatible tables and adequate pixel/stream storage are caller obligations. These routines have no general malformed-image error result. Many inner loops assume a positive count and can wrap or run excessively for invalid sizes. Forward rectangle copying also has no memmove-style overlap handling: overlapping source and destination regions can overwrite data still needed later in the copy.

Register/flags preservation and direction-flag handling differ by entry. Many copy routines save flags and execute CLD before string instructions. The plain compressed path and aligned solid-fill path do not establish that condition locally. Their string operations require the expected clear direction flag on entry. The file's general overview must not be read as a guarantee that every path preserves every register or establishes every execution flag identically.

## 3. Complete-surface and rectangle copying

| Entry | Inputs and actual pixel operation | Local bounds behavior |
| --- | --- | --- |
| BlitSurface, `0x4cbbe0` | Destination/source Surfaces and destination X/Y; copy source bytes | Clip to both images' dimensions |
| BlitSurfaceKeyed, `0x4cbcd5` | Same, plus transparent color; leave destination unchanged when source equals its low byte | Same dimensional clipping |
| BlitRect, `0x4cbdd1` | Surfaces, inclusive source rectangle and destination position; copy bytes | No clipping |
| BlitRectKeyed, `0x4cbe70` | Same rectangle operation, skipping the low-byte key | No clipping |
| BlitTile32x32, `0x4cbef1` | Destination Surface, integer X, integer Y, raw packed tile bytes; copy 32 rows of 32 bytes | No clipping |

The complete-surface copies reject images wholly outside the destination. Negative placement skips the corresponding leading source columns/rows, positive placement advances the destination origin, and the copied width/height is the remaining intersection. Each image advances by its own pitch. The ordinary complete-surface copy uses dword transfers only when the destination pointer and copied width satisfy its four-byte alignment tests; otherwise it copies bytes. The keyed form examines individual source bytes.

BlitRect derives its origin from source pixels plus top*pitch+left and its destination from destination pixels plus Y*pitch+X. It selects dword transfers for widths divisible by four, word transfers for other even widths and byte transfers otherwise. Unlike BlitSurface, this choice does not require an aligned destination pointer. None of these transfer widths changes the indexed-byte image format.

BlitTile32x32 advances the source by exactly 32 bytes per row and the destination by its Surface pitch. It expects a packed 1024-byte tile, not a source Surface or source rectangle. The DrawTile wrapper at `0x4c6e70` in [surface.cpp](../../../src/graphics/surface.cpp) declares and passes its arguments as destination, source Surface pointer, rectangle pointer and position pointer. That local interface is incompatible with the assembly's integer-X/integer-Y/raw-tile interpretation. The available source therefore does not justify presenting DrawTile as a working general rectangle-to-tile adapter. The discrepancy is retained and documented; no source correction or runtime claim is made here.

## 4. Rectangle blend, mask and shadow transforms

These three operations use the same unclipped source-rectangle/destination-position geometry. Every source byte equal to the low-byte key leaves the destination untouched.

| Entry | Operation for a non-key source byte S and existing destination D | Expected table |
| --- | --- | --- |
| BlitRectBlended, `0x4cbf2c` | Replace D with table[(S*256)+D] | 256 by 256 indexed blend table |
| BlitRectRemapDest, `0x4cbfc4` | Replace D with table[D]; S supplies coverage only | 256-byte destination remap |
| BlitRectShadow, `0x4cc332` | Replace D with table[((S-79)*256)+D] | Shade rows selected by encoded source index |

Blending is a lookup operation, not a per-channel RGB or floating alpha calculation. Results depend on the installed [palette tables](graphics-palette.md). Destination remapping preserves the source silhouette while applying a uniform transformation to whatever was already behind it. The shadow path instead uses source index 0x4f as its zero row. With the ordinary 32-row light allocation, compatible source indices span 0x4f through 0x6e. The blitter does not clamp a source index or check that a table is present; incompatible indices can address outside the intended table.

These distinctions matter to effects: two source pixels can have identical coverage but produce different blended or shadow outputs, whereas the destination-only mask ignores their different non-key colors. Calling one operation through a declaration suggesting another does not change its assembly lookup formula.

## 5. Compressed sprite row format and cropping

The four compressed entries take raw destination pixels, destination pitch, a destination rectangle, encoded row data and a source rectangle. Three also take a transformation table. Destination left/top locates the output origin. Source left/top supplies cropping; source right/bottom defines the inclusive output size. No resampling is performed.

Each encoded row begins with a little-endian 16-bit byte length. The next row begins after that prefix and the indicated payload. A zero-length row contributes no writes. Within a nonempty payload, a command header selects one of three operations:

| Header condition | Decoded run length | Following bytes and effect |
| --- | --- | --- |
| Low bit set | Header shifted right by one | No color bytes; skip destination pixels |
| Low two bits equal two | (Header shifted right by two)+1 | One color byte, repeated for the run |
| Low two bits equal zero | (Header shifted right by two)+1 | That many literal color bytes |

To reach source top, the decoder walks row-length prefixes without drawing. Within each selected row it consumes commands until the source-left crop is reached. A crop can begin inside a skip, repeat or literal run; the decoder adjusts the remaining portion rather than requiring command-aligned rectangles. It then emits or skips at most the requested width and resumes the next row using the saved encoded-row endpoint. The destination row advances by pitch, independently of how much compressed data the row consumed.

Transparency here is represented by skip commands. There is no independent color-key test on emitted literal/repeat values. A literal byte that happens to equal a common transparent palette index is still processed unless the encoder represented that region as a skip. Empty/transparent areas leave previously drawn terrain, water or other imagery intact.

## 6. Four compressed output operations

| Entry | Emitted-pixel operation |
| --- | --- |
| BlitCompressedBlended, `0x4cc057` | table[(source*256)+destination] |
| BlitCompressedShadow, `0x4cc1bf` | table[((source-0x4f)*256)+destination] |
| BlitCompressedLit, `0x4cc3d0` | table[source] |
| BlitCompressed, `0x4cc51d` | Source byte directly |

The lit entry receives a one-input remap, for example a previously selected light row. It does not select a two-input shade row using destination pixels. Blend and shadow read the current destination for every emitted pixel, so compositing order affects the result. Plain repeated runs use repeated stores; plain literal runs use word copies plus an odd trailing byte. This optimization does not alter row encoding or pixel depth.

Cropping is not stream validation. The stored row length selects the next row's start; each command is not independently checked against that endpoint. The routines have neither total encoded-buffer length nor destination image bounds. Truncated prefixes, payloads shorter than a run requires, negative source-row counts or incompatible crop sizes can read/write outside intended storage or enter wrapped count loops. After a logical row finishes, the code can fetch the following row prefix before testing the completed row count, so valid backing storage must also tolerate that look-ahead. [GAF loading and clipping](graphics-gaf.md) supplies the ordinary trusted-asset context. This module does not promise safe decoding of arbitrary external bytes.

## 7. Full-surface line clipping and normal lines

IsLineVisible (`0x4cc650`) is more than a yes/no query. It clips the four endpoint values in its own stack argument slots against X=0..width-1 and Y=0..height-1. It first accepts endpoints already inside the full surface, rejects same-side outside cases and otherwise computes integer boundary intersections until acceptance or rejection. Surface.clip is not read. The nonzero return is an acceptance value, not a count of visible pixels; an internal counter is incremented on one clipping branch.

BlitLine (`0x4cc7ab`) pushes copies of its endpoints for that call and recovers the modified copies afterward. Rejection emits no pixels. Accepted endpoints are ordered for the implemented traversal, with horizontal and vertical fast paths and shallow/steep error-accumulation paths for the other octants. Both endpoints are included. A segment whose endpoints coincide therefore writes one pixel. Vertical stepping uses signed pitch, while horizontal movement advances one byte. The color is its low byte, with no blending, depth test or transparent key.

The higher DrawLine wrapper first applies the application clip rectangle. The assembly then applies full-surface bounds again. Direct assembly callers receive only the latter protection. Clipping arithmetic uses bounded integer operations without a general overflow guarantee for extreme coordinates.

## 8. Remapped lines retain distinct pointer behavior

BlitLineRemapped (`0x4cc8df`) does not call IsLineVisible. Its caller must supply clipped endpoints. DrawLitLine and DrawBlendedLine in [draw.cpp](../../../src/graphics/draw.cpp) apply ClipLine before reaching it. Every visited pixel becomes table[(argument*256)+oldPixel]. The table and row argument must be compatible and valid; there is no local palette-availability check.

Its traversal must not be assumed identical to BlitLine. In the vertical path, the initial row offset uses Surface.width rather than pitch, although later row steps use pitch. That changes the origin when rows contain padding. In the shallow general path, a byte write does not advance the pointer, and the loop lacks the ordinary one-byte major-axis increment. Iterations that take no minor step therefore repeatedly remap the same pixel; a minor step advances by signed pitch. In the steep path, the code advances by signed pitch and retains a conditional pointer decrement even though its write did not perform an advancing string store.

These are literal differences in the current assembly, with consequences for line geometry and repeated destination transformation. A remap applied several times to one pixel need not equal one application. This reference does not silently replace those loops with an ideal Bresenham algorithm or infer a corrected visual result from their names. Construction face contours use a different [scanline endpoint renderer](graphics-draw.md), so these remapped-line quirks do not define construction-wireframe geometry.

## 9. Legacy line and rectangle entries

Three embedded entries have no independently established ordinary Surface interface:

- `0x4cca33` draws bytewise XOR line pixels using descriptor state expected in EDI. It reads a word-sized width and a pixel pointer at descriptor offset 4. It does not establish EDI from a normal surface argument, and pushes four coordinates to the five-argument IsLineVisible interface without supplying its surface argument. The nearby label describing a 16-bit line must not be mistaken for 16-bit output pixels.
- `0x4ccb65` expects a descriptor in ESI, with word-sized width/height at offsets 0/2 and an endpoint-rectangle pointer supplied on the stack. It clips by mutating that rectangle. Its branches include zero-divisor checks and a word-sized signed Y test. It is not an alternative C++ Surface clipping entry with the same argument contract as IsLineVisible.
- `0x4ccd85` draws an XOR outline through `0x4cca33`, taking a rectangle and color while inheriting the unresolved descriptor context. It covers top/bottom rows and shortens the two vertical sides to exclude their corners, avoiding a second XOR at ordinary corners. That does not establish a safe standalone interface for degenerate rectangles.

Source searches find the internal outline-to-line calls, but do not establish active external call sites for these legacy interfaces. Their retained implementation is accounted for without claiming they run in the current battle rendering flow.

The ordinary outline entry at `0x4ccd1c` takes a Surface, rectangle and color and calls BlitLine for each of four sides. Corners are visited twice, which is harmless for a plain replacement color. Each line receives full-surface clipping. This entry also uses word-sized SP adjustments while retaining a 32-bit frame; it is an assembly-specific stack sequence rather than a conventional generated C++ wrapper.

## 10. Solid fills, XOR and destination remapping

FillSolidRect (`0x4ccdea`) uses the ordinary Surface pitch/pixel fields and an inclusive rectangle. It performs no clipping. If left and right+1 are four-byte aligned, it replicates the low color byte across a dword and fills dwords; otherwise it fills bytes. Row progression uses pitch. The aligned path does not locally clear the direction flag, unlike the byte path that saves/restores flags around CLD. Positive, valid geometry and sufficient destination storage are prerequisites.

XorRect (`0x4cce87`) is a significant interface discrepancy. Its assembly reads a word-sized row width at descriptor offset 0 and pixels at offset 4, with a 16-bit MUL used in the initial row-offset calculation. It does not read ordinary Surface.pitch at 8 or Surface.pixels at 12. Consequently it is not a normal pitched-Surface XOR rectangle despite the C++ declaration in draw.cpp. XorRectangle actually passes a normal Surface pointer through that declaration. Its clipping wrapper does not resolve the incompatible descriptor interpretation. For compatible legacy storage, the inner operation XORs each byte with the low value byte; the ordinary mathematical property that identical XOR operations undo each other is not evidence that this mismatched wrapper safely updates a current Surface.

RemapRect (`0x4cced5`) instead accepts raw pixels, pitch, width, height and a 256-entry table. It replaces each destination byte with table[oldByte]. The loop processes bytes backward within each row; an alignment path groups four remapped bytes but still performs four indexed lookups. Rows advance by pitch. It does not clip, verify table flags or handle arbitrary nonpositive sizes as a documented empty operation. GrayRectangle establishes the ordinary clip/table prerequisites above this level.

## 11. Packed glyph rasterization

BlitText (`0x4ccf60`) receives raw destination pixels/pitch, font bytes, text, X/Y and foreground/background/key colors. Font byte 0 supplies height, byte 3 the first supported character, and signed byte 2 a vertical offset subtracted from Y. A word-offset table beginning at byte 4 locates each character's glyph. Characters below the first supported byte or with a missing offset are skipped without an X advance.

A glyph begins with its width followed by packed bits. Bits are consumed most-significant first, with the bit position continuing across rows rather than restarting on a byte boundary for each row. A set bit selects foreground and a clear bit background. If that selected color equals the key, the destination is unchanged; otherwise its byte is written. Each glyph advances the horizontal origin by its width. NUL or newline terminates the operation.

No destination dimensions, clipping rectangle, font allocation length or general malformed-glyph guard is supplied. In particular, invalid zero dimensions are not a supported safe empty glyph. [Font wrappers](graphics-font.md) measure and test containment before ordinary calls, but the assembly itself does not partially clip glyphs. It does not select a font, change global style, interpret multibyte Unicode, kern pairs or antialias coverage.

## 12. Sampling an arbitrary image line

SampleBresenhamLine (`0x4cd010`) writes a contiguous byte buffer from samples along a line in a source Surface. Its last parameter is provisionally named step, but its use is an output sample count. It reads source pitch/pixels, derives inclusive spans abs(delta)+1, selects an octant/major axis, and handles equal, shorter or longer output lengths using whole advances and fractional carry/borrow accumulation.

When output length exceeds the sampled span, repeated output pixels can reuse the last fetched source value while the source position has not advanced. A shorter output skips source positions. Axis-aligned equal-length cases reduce to ordinary traversal. Other octants change the signs of source X/row steps. Destination always advances through consecutive bytes; it has no destination pitch or rectangle argument.

This entry neither clips the source line nor applies transparency, lighting, depth or palette conversion. A count of zero is not guarded as a valid empty request. Some stretch paths use word-sized divisors and therefore impose additional arithmetic range constraints; invalid counts can divide by zero or wrap loop state. No active source caller was found, so the presence of this sampler does not establish that the live terrain/model renderer uses it for scaling.

## 13. Fixed-width texture span samplers

| Entry | Source texture width | Row-coordinate shift | Row mask |
| --- | ---: | ---: | --- |
| BlitSpan128, `0x4cd896` | 128 | 7 | 0xff800000 |
| BlitSpan64, `0x4cd8da` | 64 | 6 | 0xffc00000 |
| BlitSpan32, `0x4cd91e` | 32 | 5 | 0xffe00000 |
| BlitSpan16, `0x4cd962` | 16 | 4 | 0xfff00000 |

Each takes raw destination/source pointers, output count, two fixed-point coordinates and their increments. In actual stack order the fourth/fifth arguments are the horizontal/vertical fixed coordinates used by the calculation; source overview parameter names should not be treated as an independent semantic contract. The 32-bit address calculation shifts the vertical coordinate by the width-specific shift, applies the row mask, adds the horizontal coordinate, then logically shifts right by 16. For valid nonnegative coordinates this selects the expected row*width+column byte. The mask is not a general texture-bounds or repeat-wrap guarantee for arbitrary coordinates.

For every output byte it samples once, stores once and adds both increments. These are nearest indexed samples with no color key, depth test or lighting lookup. A zero count produces no output because the count is checked before the first store; negative counts are not a valid bounded request. The destination is contiguous and the texture must have the assumed packed width.

[Surface polygon rasterization](graphics-surface.md) calls these entries on no-depth paths. The caller supplies span geometry, texture coordinates and clipping. That caller's retained width-128 fallthrough invokes the width-64 interpretation afterward, overwriting its first result. The blitter does not compensate for the caller's dispatch behavior. Depth-enabled paths perform their own sampling/depth/light operations and must be read separately.

## 14. Interactions, lifetime and coverage limits

All 28 entry bodies, including legacy routines and the long octant-specific sampler, were traced. Tables are borrowed from palette/display state; Surfaces, raw buffers, font data and encoded sprite rows remain owned by higher systems. The blitters retain no object references or per-unit state and have no save record, network packet or construction-progress counter. Particle and weapon renderers reach them through surface/GAF/drawing services rather than granting a projectile permission to hit a unit.

Visible output depends on existing destination contents for keyed, skipped, blended, shadowed, remapped and XOR operations. Presentation order, correct asset encoding, pitch and caller clipping are therefore part of the behavior. This account records current source inconsistencies explicitly. It is based on static inspection, not a build, game execution or a corrected replacement implementation, and contains no copied source listings.
