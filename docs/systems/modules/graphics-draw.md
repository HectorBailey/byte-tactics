# Drawing primitives, polygon spans and construction edges

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `graphics/draw`, 37 annotated entries from `0x4be950` through `0x4c1470`. Sources are [draw.cpp](../../../src/graphics/draw.cpp) and the retained [ScanFillPolygon implementation](../../../src/graphics/draw_4c0330.cpp). The module draws indexed pixels, lines, rectangles, circles and flat/shaded model polygons. It also owns global text-style accessors. Texture mapping, glyph rasterization and sprite-run decoding belong to other modules.

## 1. Two destination contracts

Ordinary interface primitives accept a Surface with dimensions, byte pitch, pixel pointer and inclusive clip rectangle. Many accept null to request the currently locked screen. Their LockScreen/UnlockScreen wrapper behavior is specific to each function, not a universal guarantee that a successful return means visible pixels changed.

Model polygon routines instead accept a GAF-style raw frame with unsigned-short width/pitch and height, color plane and optional byte depth plane. These routines do not lock a screen or consult a Surface clip rectangle. Their depth coordinate is the projected height value supplied by [model rendering](graphics-model-render.md), not a floating-point camera-distance buffer.

Color arguments ultimately identify palette indices. Shade/light/blend operations consume [display lookup tables](graphics-palette.md); they do not construct new RGB pixels in the destination. Existing surfaces and palette storage remain caller/display-owned. This module allocates no persistent primitive object or render command queue.

## 2. Clipping and line operations

ClipLine (`0x4bea20`) obtains the surface clip rectangle, records original direction and X/Y deltas, then clips each endpoint against left, top, right and bottom. Intersection arithmetic is integer multiply/divide and mutates the supplied coordinates. Wrong-side direction or a required zero divisor rejects the segment. This is a fixed endpoint-clipping procedure; it does not perform arbitrary subpixel antialiasing.

DrawLine (`0x4be950`) clips then delegates to BlitLine. For a supplied surface it returns 1 even if clipping rejected the entire line. For null it returns the screen-lock result and unlocks only after a successful lock. DrawPixel (`0x4bee60`) creates a zero-length segment and uses the same clipped line primitive. Despite a misleading nearby source comment about a character, its body does not select a font or draw a glyph.

DrawLitLine (`0x4bec70`) uses the display light table and returns zero when that pointer is null. DrawBlendedLine (`0x4bed70`) passes the alpha table to the remapped line blitter without the same local pointer guard. Both use ClipLine first. Their pointer-shaped return is a success/lock value, including the literal value 1 on the supplied-surface path, not a dependable returned destination pointer.

ReadPixel (`0x4befe0`) directly reads pixels[pitch*y+x], with no clip or coordinate validation. A null surface acquires/releases a screen lock, but failed locking leaves its return color uninitialized. Its return must not be used as a lock-status boolean or a validated out-of-bounds sample.

DrawPolyline (`0x4bf060`) draws count-1 adjacent segments without automatically closing the contour. DrawPolylines (`0x4bf260`) consumes consecutive point lists according to the supplied counts array, advancing once past each list's last point. Both lock once for null surface and clip every segment. A supplied surface returns 1 even with no drawable segments. Counts of zero are not a special empty-list representation in the multi-list pointer progression: the pointer still advances one element per row.

DrawPolygon (`0x4c07b0`) joins adjacent Vec3 X/Y coordinates and explicitly joins the last to the first, ignoring Z. It returns 1 regardless of individual DrawLine outcomes. It requires a valid positive count because the closing call indexes count-1 and element zero without a count guard.

## 3. Rectangles and destination remapping

ClipRectangle (`0x4bf620`) rejects wholly disjoint or reversed rectangles and clamps an overlapping rectangle in place to the Surface's inclusive bounds. FillRectangle (`0x4bf6f0`) copies the caller rectangle first, then clips and fills through FillSolidRect. Its result reflects clipping/locking more closely than DrawLine's supplied-surface result.

DrawRectangle (`0x4bf8c0`) draws top, right, bottom and left edges, each clipped separately. It does not fill the center. The supplied-surface branch leaves its return local uninitialized. DrawLitRectangle (`0x4bf7b0`) draws the same four edges through DrawLitLine, but its supplied-surface path returns its initially zero result even after drawing. Neither return should be interpreted as an exact pixel count.

XorRectangle (`0x4bfd60`) copies/clips the rectangle and calls XorRect. The [assembly implementation](graphics-blit.md) reads a legacy descriptor with word-sized width and pixels at offset 4, whereas this wrapper passes an ordinary Surface with pixels at offset 12. Clipping does not resolve that interface discrepancy, so the call must not be described as a verified working Surface XOR. For a compatible descriptor, applying the same XOR twice can restore pixels only when intervening content and affected geometry are unchanged. The supplied-surface branch returns the value argument's stack-slot contents; it is not a conventional success result. GrayRectangle (`0x4bfe10`) requires the display gray-table availability bit, then remaps existing destination pixels through that table. It leaves the supplied-surface return status uninitialized.

FadeRectangle (`0x4bf4d0`) remaps each destination pixel through one selected 256-byte table row. Negative levels clamp at -32 and select shade-table row level+32. Nonnegative levels clamp at 31 and select the light-table row directly. Thus zero selects the first light row, and -32 selects the first shade row. The generated palette tables define the actual brightness of those rows; a name such as fade-in does not establish monotonic opacity.

FadeRectangle copies the supplied Surface descriptor but passes the original rectangle to ClipRectangle, so it can mutate the caller's bounds. A null rectangle creates bounds from display width/height and clips them to the actual surface. Row addressing uses surface.width rather than surface.pitch. Its pixel pointer is plain char and is used directly as the remap index, with no unsigned conversion, so signed-char interpretation can index before the selected row for high palette values. A missing table returns zero from inside the operation before the ordinary null-surface unlock at the end, leaving that lock unbalanced on this path.

DitherRectangle (`0x4bff20`) clears alternating byte positions to zero using row/phase parity. It clips a copied rectangle, processes an unaligned prefix, applies alternating dword masks to the middle and clears a trailing suffix. The right middle boundary is rounded upward to a four-byte boundary; some narrow/unaligned cases can therefore clear pixels beyond the requested right edge. This literal implementation is not a general safe per-pixel clipping promise. It returns 0 on screen-lock failure and otherwise 1, even if clipping produced no operation.

## 4. Circles are connected line segments

DrawCircle (`0x4c0070`) starts at center X plus radius and uses fixed-angle trigonometric helpers at 0x800 angle increments through 0x10000, producing 32 connected segments. It clips each segment through the line helper. It is a polygonal circle outline, not a filled disk or pixel-perfect midpoint-circle rasterizer. With null destination, each segment performs its own lock/unlock path.

DrawDashedCircle (`0x4c01a0`) divides the full angle by the supplied segment count n, samples using that integer step and draws every other segment according to the start parity. Integer division can leave a final remainder rather than exactly sampling a full turn. n=0 divides by zero; n greater than 65536 produces zero step and a nonadvancing loop, while negative counts can also produce invalid progression. Callers must supply the intended positive bounded count. Neither circle routine performs world projection or reads unit range by itself.

## 5. Filling ordinary polygons

FillPolygon (`0x4c0310`) is a void adapter to ScanFillPolygon (`0x4c0330`). The latter acquires a screen when needed, finds the polygon's top/bottom vertices and horizontal extent, rejects an off-clip or zero-height shape, then walks backward and forward from top to bottom to populate two X endpoints per row.

Each rising edge uses a 16.16 X increment, initialized with a 0xffff fractional bias before extracting whole positions. Edges above/below the clip are advanced/truncated. The two walks assume appropriate ordered convex or horizontally monotone geometry; they are not a general multiple-intersection fill for arbitrary concave/self-crossing paths. Reversed winding can exchange expected left/right spans instead of automatically normalizing them.

For rows from minimum Y up to but excluding maximum Y, it clamps X endpoints and fills right-left bytes when positive. Right and bottom endpoints are therefore excluded by these polygon fill loops even though Surface clipping bounds themselves are inclusive. It returns 1 after processing, which does not guarantee a nonempty span was emitted. Its local span table holds 2048 rows, with no dynamic allocation or check that the clipped height fits. Valid geometry/counts and bounded render dimensions are preconditions.

## 6. Raw-frame model polygon rasterization

FillFlatPolygon (`0x4c1000`) and FillShadedPolygon (`0x4c0c70`) construct two edge walks similarly, but accept raw frame coordinates, depth, and, for the shaded form, shade values. They reject shapes wholly right, above or below the frame and clamp Y to zero through height-1. They walk rows below maximum Y, so that clipped final row is also excluded. Every rising edge interpolates X and Z in fixed point; the shaded path additionally interpolates its shade coordinate.

Both use 2048 local Span records. They do not validate vertex indices supplied by a higher model layer, normalize winding, clip arbitrary nonmonotone polygons or allocate more rows. They call their span writer only when x2-x1 is positive. Those caller tests matter because the writer computes slopes by dividing by that width before a later post-clip count check.

FillFlatSpan (`0x4c06e0`) derives depth slope, advances its starting depth when X starts negative, clamps X2 to width-1 and emits x2-x1 pixels. With a depth plane, the interpolated depth is converted to a byte; a pixel draws when stored depth is at most that incoming byte, then updates stored depth. Higher or equal values win, opposite to a conventional nearest-low-Z test. Without a depth plane it fills color unconditionally. This function clips horizontally, but requires an already valid row.

FillShadedSpan (`0x4c0b10`) also interpolates shade. A drawn pixel becomes shadeTable[(shadeWhole<<8)+baseColor], then updates depth where present. Shade rows are not locally clamped or table-availability-checked; [model lighting](graphics-model-render.md) supplies its masked 0..31 values. Direct callers with incompatible shade values can index outside the ordinary 32-row allocation. Equal-depth later polygons overwrite earlier pixels, so draw order matters at coincident surfaces.

## 7. Construction wireframes use scanline endpoints

DrawPolygonEdges (`0x4c0820`) builds two X/depth edge walks for each supplied projected polygon. It does not call DrawLine for every contour edge. Instead, each positive-width scanline is sent to PlotSpanEnds (`0x4c0a90`), which draws only its two X endpoints.

With no depth plane, PlotSpanEnds writes both endpoint colors directly. With a plane, it independently converts each endpoint's interpolated depth to a byte, draws when that value is at least stored depth, and replaces stored depth on success. The center remains unchanged. Zero-width scanlines draw neither endpoint. DrawPolygonEdges itself performs no frame clipping before populating or emitting its 2048-row span array, and PlotSpanEnds performs no endpoint/row clipping. Model-image bounds must contain the contour.

[Construction rendering](graphics-model-render.md) supplies closed face contours and animated colors after applying filled-region recoloring. The resulting wireframe is therefore a depth-tested scanline boundary of visible model faces. It can vanish behind a higher stored surface, overwrite equal-depth earlier work or differ with hull polygon topology. It is not an X-ray scaffold inferred from unit footprint, and it does not reveal hidden COB pieces or bypass water/child-composite passes.

## 8. Point containment is strict and winding-dependent

PointInPolygon (`0x4c1320`) rejects fewer than three points. For each consecutive pair, including last-to-first, it requires the signed edge cross-product inequality to be strictly positive in the implemented orientation. A point on an edge is rejected. All edges must pass, so the routine assumes a consistent convex half-plane interpretation; it is not a ray-crossing test for arbitrary concave polygons. Reversing the vertex order changes acceptance. Integer products can overflow for unbounded coordinates.

## 9. Global text-style accessors

SetTextColors (`0x4c13a0`) changes foreground/background display fields independently, leaving a field unchanged for argument -1. SetTextKeyColor (`0x4c13d0`) writes the transparent text color without that sentinel exception. GetTextKeyColor (`0x4c13f0`), GetTextForeColor (`0x4c1400`) and GetTextBackColor (`0x4c1410`) return those global fields.

SetFont (`0x4c1420`) installs a nonnull pointer and ignores null, without allocating, retaining a reference count or freeing the old font. GetFont (`0x4c1440`) returns that pointer. GetFontHeight (`0x4c1450`) reads its first byte, while FontHeight (`0x4c1470`) reads the first byte of an explicitly supplied font. Neither height accessor guards null. Glyph metrics and drawing are handled by the font module; these functions only select/read shared presentation state.

## 10. Coverage, interactions and failure interpretation

All 37 address-assigned functions were inspected, including the separate scan filler and the two span writers placed out of address order. The coverage includes clipping, ordinary/remapped lines, pixels/readback, polyline lists, rectangle effects, circles, polygon outlines/fills/containment, construction endpoints, raw depth/shade spans and nine text/font accessors.

The module has no unit command, construction-payment, damage, packet or save format. Interface code and model rendering supply coordinates/state; [blitters](../../../src/graphics/blit.cpp) and [surfaces](../../../src/graphics/surface.cpp) implement lower-level memory/screen operations. A boolean-looking result can mean lock success, clip acceptance, unconditional completion or an original uninitialized return, as described per routine. These are static source contracts and original limits, not results of a build or runtime rendering comparison. No source listings were copied into this document.
