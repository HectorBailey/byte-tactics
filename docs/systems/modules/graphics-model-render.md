# Model state, cached rendering, construction wireframes and attachments

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `graphics/model_render`, 35 annotated entries from `0x458160` to before `0x45b250`. The main implementation is [model_render.cpp](../../../src/graphics/model_render.cpp), with three retained rendering files and three functions physically in [unit_motion.cpp](../../../src/units/unit_motion.cpp). Address ownership matters: the shared arena allocator is documented under [unit_motion](units-unit-motion.md), while the model draw/state/shadow functions in that file belong here.

## 1. Geometry, instance state and ownership

Object3do is shared model geometry: named nodes, vertex/face lists, local origin offsets and sibling/child links. A runtime Object State owns a flat array of 0x36-byte piece records and one mutable point array per piece. Each record retains its shared geometry pointer, translation/rotation fields, transformed offset, flags, modified marker and runtime parent/child/sibling links. The state header retains owner, cached color/depth picture, cached shadow, base angles, dirty state and draw count.

CreateObjectState (`0x45a8d0`) counts the geometry tree, allocates 0x22 header bytes plus count*0x36, clears it and recursively builds records. CountObjects (`0x45ae80`) includes the node, its descendants and siblings. AddStateEntries (`0x45aec0`) copies each geometry point list into an owned vertex buffer, marks the piece colored and lit, and marks it visible only when its geometry has at least three vertices. It preserves child/sibling parent relationships and increments the actual populated count. Geometry nodes with fewer vertices can still exist as nonrendered attachment or animation points.

CreatePlayerObjectState (`0x45a950`) performs that initialization, retains its supplied owner pointer, and reorders the flat array to match the script's piece-name list case-insensitively. For script entry i it searches runtime entries from i onward and swaps a match into slot i. An unmatched name does not produce a diagnostic here. LinkStateEntries (`0x45af90`) then reconstructs tree pointers by matching geometry pointers, because swapping records invalidates the earlier positional links. Script piece numbers therefore depend on this name-order pass rather than raw geometry traversal order.

FreeObjectState (`0x45aaa0`) frees every per-piece point array, releases picture/shadow arena handles when the global cache exists, then frees the state. It does not free shared Object3do geometry. Allocation sizes, counts and null validity are trusted; there is no local transactional rollback for a failed point allocation during recursive construction.

## 2. Updating transforms and script-controlled visibility

UpdateObjectState (`0x45ab10`) compares the owner's three base angles against the cached angles using signed-short differences. A change of at least eight on any axis copies all base angles, marks animation dirty and clears the root modified marker. If the root is colored, it also resets the cached draw count. Sub-eight differences alone do not force that update.

When dirty, RestorePieceVertices (`0x45b030`) restores original geometry vertices and zero offsets when the piece's modified marker is **zero** or force is nonzero. Despite the marker's provisional name, the restoration condition is not simply modified!=0. It propagates its restoration result to children, while a sibling receives the original force argument. PoseModel (`0x45b0a0`) processes children before applying the current piece's transform. It maps the piece's short angle fields into XYZ, adds state base angles for the outer/root call, adds local translation fields to the geometry node origin, then invokes TransformPieces.

TransformPieces (`0x45b150`) rotates and translates the mutable points and offset only for pieces whose modified marker is zero. It recursively applies that same transform to children and, when the deep argument requests it, siblings. The postorder pose walk lets child-local transforms occur before ancestor transforms. [COB and script calls](units-unit-script-calls.md) supply the changing piece translations, rotations and flag state; the renderer consumes those values rather than issuing build or weapon script callbacks itself.

DrawUnit (`0x45ac20`) performs equivalent dirty/base-angle preparation before drawing. A nonzero carrier pointer suppresses drawing the unit independently. Instead its parent can draw it through the cargo chain. For visible attached children it prepares their state too, excluding runtime flag 0x20000. Piece visibility bit 1 controls participation in bounds, polygons and construction edges; hidden turrets, platforms or hull pieces do not become visible just because construction is in progress.

## 3. Cache initialization and picture selection

UnitTable::Construct (`0x458160`) clears the inherited cache pointers and scratch pointer. Initialize (`0x458180`) initializes the arena and allocates one 600 by 600 depth frame named CompositeBuffer. Failure of either step returns zero; failure of the second step does not locally undo the initialized arena. Destroy (`0x4581c0`) frees the scratch frame if present and frees the arena buffer. These methods do not select a ship-specific surface size.

DrawObjectState (`0x458810`, physically in [unit_motion.cpp](../../../src/units/unit_motion.cpp)) uses camera scroll X/Z in 16.16. For runtime-0x20000000 owners, its color-group selector is the inverse of activation bit 1; otherwise it tests the motion object's +0x20 field for zero. A zero draw count triggers rebuilding. Missing bitmap clears the shadow handle; static/runtime-0x20000000 models and zBufferFlag-bit-1 models require a bitmap when absent. An unfinished static model with runtime flag 0x2000 also rebuilds if the existing picture lacks a secondary plane. It calls BuildObjectPicture when required, draws, then increments the count. Arena eviction can clear a retained owner handle, making later rebuilding necessary.

DrawObjectPieces (`0x458430`) uses DrawObjectPicture when a cached bitmap exists. Without one, it visits visible pieces and calls direct DrawPiece. Thus the fallback can draw actual polygons rather than assuming every cache allocation succeeds. It is a simpler path with different composition capabilities from a depth picture.

BuildObjectPicture (`0x4586a0`) measures the transformed visible model and allocates a bitmap. One plane is allowed only when its force-depth argument is zero, owner zBufferFlag bit 1 is clear and buildLeft is exactly zero. Otherwise it requests a two-plane bitmap. It records image origins and chooses lit rasterization only when runtime bit 0x20000000 and the shading option are both set; other cases use ordinary piece drawing. The bitmap pointer determines success, with zero returned when allocation yields no picture.

## 4. Projection, bounds and polygon material selection

MeasureModel (`0x4581e0`) projects every visible transformed vertex using screen X=X and screen Y=-Z-Y/2, taking signed whole coordinates from fixed values. Its optional translation applies before projection. Bounds start at zero, so the model origin remains inside them, and receive two-pixel margins. AddModelBounds (`0x458310`) performs the related expansion into existing bounds. Geometry is measured after piece posing, not from the FBI footprint or a fixed ship rectangle.

DrawPieces (`0x459830`) processes pieces in reverse array order. A visible piece participates when useColor is -1, its colored flag matches useColor, or the owner is unfinished. The construction exception relaxes the grouping filter but preserves visibility. Each vertex receives the same projection, image-origin offset and a secondary height/depth value of model Y plus 50, or plus 125 for definition flags1 bit 30 (digger). These values are not normalized to the full height of each model.

Faces use their indexed vertices. When firstFace is anything other than -1, processing begins at face 1; it does not begin at the numeric value stored in firstFace. In the current local flag view, the branch named textured draws a flat polygon with the face color field, while its opposite draws a texture only for four-vertex faces. This behavioral description avoids taking that provisional bit-field name literally. Texture selection uses a direct picture, a player-color frame, frame zero, or an advancing sequence according to the face flags and useColor. Nonquad faces in the texture branch are skipped rather than triangulated here.

DrawPiece (`0x4584d0`) is the direct surface equivalent. It adds owner world X/Z relative to camera, uses owner world Y, and projects with viewport offsets X+128 and Y+32. It resolves the same material alternatives but does not create a separate height plane, apply the construction recolor pass or merge cargo as a combined depth image by itself.

The rasterizers trust geometry sizes. Local vertex arrays hold 2000 points and face arrays hold 25. There are no per-face bounds/index checks in these draw loops. Construction edges need one additional closing point, so their 25-entry temporary supports fewer than 25 face vertices safely. Model validity is an asset precondition.

## 5. Lighting and optional supersampling

SetLightVector (`0x4597f0`) converts its three integer arguments to floats by multiplying by 0.01; it does not normalize the vector. DrawLitPieces (`0x459c70` in [its retained file](../../../src/graphics/model_render_4581e0.cpp)) uses the same visibility/group and projection rules, plus normals and palette shading.

For each face it constructs and normalizes a cross product from its first three indexed vertices. Repeated indices select an upward normal. The code assumes those first three entries exist and does not separately validate a minimum face count. It accumulates face normals at each referenced vertex, averages by incidence count and uses the dot product with the global light vector. Lit pieces convert five times that value to an integer and mask with 31, as established by the 5.0 constant in [global data](../../../data/globals.csv). This masks/wraps instead of clamping a negative or oversized shade. Unlit pieces use row 15. Shaded polygon/texture consumers then use the palette lookup tables.

Both DrawPieces and DrawLitPieces optionally render at twice width and height when anti-aliasing is enabled, runtime bit 0x20000000 is set and useColor is nonzero. They reuse the CompositeBuffer, double projected X/Y and origins, clear its color/depth planes and draw. Color is downsampled through [GAF alpha-table blending](../../../src/graphics/gaf.cpp); the depth plane takes every second sample rather than averaging four depths. HalveFrame (`0x4587b0`) exposes that nearest-sample secondary-plane operation separately. The extra plane remains height/depth data, distinct from the lighting row interpolated within a lit face.

No local growth of the 600 by 600 scratch allocation occurs when its header is resized. Oversized model/composite or doubled dimensions therefore depend on the surrounding size assumptions, not a bounds-safe dynamically enlarged supersampling target.

## 6. Construction recoloring: thresholds, pulse and band

ShadeByIntensity (`0x458dd0`) returns zero without drawing construction edges if the image has no secondary plane or the owner has exactly zero buildLeft. Otherwise it computes A by truncating buildLeft*255. Two triangular-wave palette indices range from 0xA0 to 0xAF. Their phases derive from unit ID XOR 5/9, plus gameTick*33/30 and gameTick*57/30 respectively. Rendering can therefore pulse while resource-paid build progress is unchanged.

RecolorByShade (`0x458d30`) skips pixels already equal to the transparent color key. It divides remaining pixels into lower, middle and upper regions using byte threshold L: lower means depth below max(0,L-4), upper means depth at least L, and middle is the intervening band. For each region the action can retain the original pixel (-1), make it transparent (-2), or replace it with a palette index. The secondary plane is not rewritten by this recolor loop.

| A range, while buildLeft is nonzero | Byte threshold L | Lower region | Middle band | Upper region |
| --- | --- | --- | --- | --- |
| Above 235 | (A-235)*255/20 | Transparent | Pulse 1 | Transparent |
| 201..235 | (A-200)*255/35 | Transparent | Pulse 1 | Transparent |
| 116..200 | ((115-A)*255/85)-1, converted to byte | Pulse 1 | Pulse 2 | Transparent |
| 31..115 | ((30-A)*255/85)-1, converted to byte | Original | Pulse 2 | Pulse 1 |
| At most 30 | A*255/30 | Original | Pulse 1 | Original |

Arithmetic is integer arithmetic. The middle two thresholds intentionally include negative signed intermediates followed by byte conversion, which wraps rather than clamps. A can be zero after truncation while buildLeft is still positive, so the routine still runs in that case. Exactly zero remaining construction skips the whole operation. There is no generic full-model-height normalization and no new scaffold mesh.

After recoloring, DrawPieceEdges (`0x458fa0` in [its retained file](../../../src/graphics/model_render_458fa0.cpp)) projects visible piece faces, honors the same face-zero convention, repeats the first face point to close each contour and invokes DrawPolygonEdges with pulse 2. The [draw implementation](../../../src/graphics/draw.cpp) generates scanline endpoints and tests height against the secondary plane when present. These are occluded polygon contours, not an unrestricted X-ray line grid. Hull topology, mast height, model origin and script visibility alter the apparent wireframe at equal build progress.

## 7. Cached pictures, attached products and shipyard composition

DrawObjectPicture (`0x459200` in [its retained file](../../../src/graphics/model_render_4589c0.cpp)) returns if no picture exists. It computes owner screen position, retains the camera for later direct pieces, and obtains ground height for shadow projection. Its ordinary picture position is X+128, Z-Y/2+32; shadow drawing adds another five X pixels and uses ground height instead of owner altitude.

With a one-plane picture, it draws applicable shadows, rebuilds if cache operations invalidated the bitmap, then draws the picture normally or blended. Visible pieces lacking the colored bit are drawn directly afterwards. Eligible cargo children are also drawn piece by piece in this branch. A one-plane cache does not provide the same interleaved child-depth composition as the two-plane branch.

With a secondary plane, MergeIntoComposite (`0x4589c0`) expands bounds to include the parent and every cargo child without runtime flag 0x20000, offset by child-minus-parent position. It updates the scratch header, copies both planes directly when dimensions match, or clears and reblits each plane separately with temporary pointer/origin swaps. It then applies the parent's construction recoloring. The bounds/offset path contains fixed/short conversions and shift arithmetic; it is not a general perspective transform or a float bounding-box library.

The caller then redraws useColor=0 pieces when the parent lacks runtime 0x20000000 or has zero buildLeft. The common DrawPieces construction grouping exception still applies here. This later draw is part of the literal ordering: the final picture cannot be inferred solely from the earlier parent's recolor call for every runtime flag combination.

For each eligible cargo child, the caller builds a forced two-plane picture with useColor=-1, applies that child's own ShadeByIntensity and merges it into the parent composite using child-parent X/Z offset, projected Y offset and whole Y depth adjustment. [DrawFrameDepth](../../../src/graphics/gaf.cpp) copies a nontransparent source pixel only when destination depth is at most source depth plus the supplied vertical adjustment, and stores the adjusted depth as a byte. It rejects negative starting offsets but performs no right/bottom clipping in that loop. Parent bounds and geometry assumptions must make the destination large enough.

A factory product is a live attached unit, so its own hierarchy, owner state, buildLeft, colors and visibility remain relevant inside the producer's image. Yard foreground structures can obscure the product while other sections remain visible. Two products can differ at identical progress because of their local height distribution, attachment transform, draft/sea-level handling and polygon layout. The model module has no switch on a named destroyer/submarine/shipyard to choose a unique wireframe algorithm.

Carrier suppression in DrawUnit and parent-child traversal prevent the normal attached unit from being independently drawn as a second world object. Hidden-attachment bit 0x20000 removes it from preparation, bounds and the corresponding child draw loops. The factory/transport orders and [unit positioning](units-unit-position.md) establish those links and transforms; the renderer does not decide completion or detach a finished ship.

## 8. Water, cloak and final composition

After parent/child depth composition, DrawObjectPicture compares sea level to the parent's whole Y. A positive difference is increased by the normal 50 depth bias, or 125 for digger definitions. For a nonlocal owner without runtime 0x200, CutFrameBelow makes pixels at or below that byte threshold transparent. For the local owner or a unit with that contact bit, TintFrameBelow maps nontransparent pixels at or below the threshold through the [blue palette table](graphics-palette.md). It is an ownership/contact-sensitive indexed remap, not simulated optical refraction.

Digger definitions also receive a final cutoff at 125. The final image uses normal drawing unless activation cloak bit 4 is set or debugMode is nonzero, in which case it uses blended drawing. Sensor visibility decisions elsewhere still control whether the unit reached this draw path; this blend is not a replacement for cloak detection logic.

An attached child's height adjustment and parent's common water threshold affect where its keel/deck pixels are tinted or removed. The detailed [naval construction case study](../../naval-systems.md) connects these draw operations to factory attachment, progress payments and launch orders. Particle requests remain in their own order/effect modules: construction pulsing alone does not show that work or nanolathe emission occurred this tick.

## 9. Shadows and image-copy support

Shadow drawing requires the global shadows option and excludes noshadow definitions (flags1 bit 25). For one-plane pictures, static/runtime-0x20000000 nondigger units use a cached geometric shadow; other eligible units use a silhouette when vehicle-shadow bit 3 is enabled and definition mask 0x81000 is clear. The mask excludes the canhover/floater definition bits in this path.

The two-plane branch has a digger silhouette/cutoff path, a static geometric-shadow path, and the same vehicle silhouette option for remaining units. Underwater silhouette depth can be cut before drawing. The geometric-shadow gate tests unitDefIndex!=0 **or** owner Y at least sea level. With a normal nonzero definition index, that condition permits a shadow regardless of underwater origin; it is not an AND water exclusion. Some shadow/cache operations can evict the model picture, so the caller checks/rebuilds it again afterwards.

MeasureShadow (`0x45a510`) projects visible vertices with X+Y/4 and -Z-Y/4 and margins. DrawShadowShape (`0x45a610`) draws only pieces both visible and colored, projects the same shape and sets height to Y+25. BuildShadow (`0x45a790`, physically in [unit_motion.cpp](../../../src/units/unit_motion.cpp)) clears the shared scratch planes, draws the geometric shape, calls CutOutFrame using the source picture at offset (5,0), compresses the result, allocates an arena picture and copies the compressed bytes. It then restores the measured dimensions/origins and marks the picture compressed. There is no local null check between shadow allocation and copying the result.

MakeSilhouette (`0x45a470`) copies a source frame into scratch and replaces every non-key color with zero. CopyPicture (`0x459170`, also in unit_motion.cpp) copies dimensions, origins, key, color pixels and the optional secondary plane without recoloring. Both rely on existing scratch capacity and do not allocate a separate long-lived image. DecrementCountArgument (`0x458d20`) only executes an empty count-down loop; it does not mutate the model or release handles.

## 10. Coverage and system boundaries

All 35 entries were checked: cache lifecycle, bounds and direct/cached piece drawing, ordinary/lit rasterization, construction recoloring/edges, picture copies/merging, shadow generation, state allocation/reordering/linking/freeing and transform updates. The ownership audit includes DrawObjectState, CopyPicture and BuildShadow in unit_motion.cpp rather than incorrectly assigning them to that module by filename.

Rendering reads simulation state and creates cached pictures. It does not spend resources, advance construction, choose weapon targets, damage units, transmit packets or save unit records. Those systems can dirty geometry/flags or invalidate handles consumed here. Exact named-unit appearance requires its loaded geometry, textures, palette and COB-controlled piece state; the implemented transformations, draw ordering and failure limits above are source-established. No source listings, build or runtime execution was used for this writeup.
