# Terrain loading, sight coverage, exploration and fog rendering

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md) | [Sight ray tables](map-los-tables.md)

Module group: `map/line_of_sight`, 29 annotated functions from `0x4814c0` through `0x484b50`. Sources: [map/sight lifecycle and rendering](../../../src/map/line_of_sight.cpp), [coverage removal](../../../src/map/line_of_sight_481d50.cpp), [coverage addition](../../../src/map/line_of_sight_482270.cpp), [fog tile building](../../../src/map/line_of_sight_4843c0.cpp) and [SightQuery contract](../../../src/map/sight_query.h). The module includes TNT terrain loading and tile display as well as sight.

## 1. Information layers and coordinates

The map has 16-world-unit terrain cells with 13-byte records, 32-unit information-grid cells, 32-by-32 pixel terrain artwork tiles, a coarser spatial bucket grid, and viewport-local fog-edge cells. Those grids share dimensions through conversions but are not interchangeable arrays.

Two information layers are particularly important. The global unsigned-short visibilityMask holds one persistent reveal/mapping bit per player at each half-resolution cell. RevealAroundUnit only adds the requesting bit. A per-player byte grid at +0x7c instead counts **current sight contributions**, incremented/decremented as eyes move or expire. Provisional names such as explored on that byte grid do not make it permanent exploration. Fog combines both layers: unseen history becomes black, while mapped but currently unobserved terrain is gray/dithered.

Map flags gate those operations: mask 1 enables the permanent reveal path, mask 2 enables dynamic sight coverage, mask 4 selects height-aware ray processing and mask 8 caches viewport fog tiles. These are observed implementation roles; a displayed game option can set several bits through its own configuration logic.

SightQuery references the owning player, cached short X/Z, short sight distance, byte eye height, cached frame/height byte and world 16.16 position. Screen vertical projection is Z minus half Y. Its cache coordinates therefore refer to the projected information plane, not directly to unit occupancy cells.

## 2. Cell lookup and height ranges

GetMapCell (`0x481550`) bounds-checks signed terrain indices. GetMapCellAtPosition (`0x4815a0`) shifts fixed-point X/Z right by 20 and checks the resulting terrain cell. GetOriginCellAtPosition (`0x4815f0`) also resolves feature continuation 0xfffe by subtracting stored offsets, rechecking the origin bounds. Outside returns null. This lookup does not test reclaimability or allocate an object.

UpdateCellHeightRange (`0x483210`) clamps the requested rectangle, then sets each processed cell's low/high to min/max of its height sample and available right/below/diagonal samples. Upper limits clamp to width-minus-one/height-minus-one and loops are exclusive, so final row/column are not rebuilt by the ordinary full rectangle. UpdateAllCellHeightRanges (`0x483370`) calls that path with full dimensions, retaining that edge behavior.

Low/high are geometric bounds used by placement, collision and derived grids. They are not water depth values stored independently for each ship class. Sea level plus movement-class limits derive those later tests.

## 3. TNT versions and environmental parameters

LoadTntMap (`0x483610`) loads the mission's name-slot-1 file through progress-aware file loading. It accepts TNT versions 0x1020 and 0x2000. Other versions report Unknown TNT version through FatalError. It does not independently validate every embedded offset/count against file length before dereferencing them.

Both versions supply dimensions, sea flag, tile/tileset pointers and counts. The older version uses eight-byte attributes with byte feature IDs and threshold 0xfc; the newer uses four-byte attributes with short feature IDs and threshold 0xfffb. Optional generated minimap fields and offsets also differ between versions. The loader converts relative offsets into pointers into the loaded file, copies required buffers, then frees that file.

For version 0x2000 or later, nonnegative mission minimum/maximum wind override defaults; older format uses its header values. Gravity comes from nonnegative mission value for the newer format, otherwise nonzero old header gravity, otherwise fixed 0x1fdb. Configured gravity converts to 16.16 per-tick-squared scale by multiplying 65536/900. Tidal strength uses nonnegative mission float or default 0.5. Sea level narrows the TNT flag to a byte. World dimensions are terrain dimensions times 16.

When generated minimap flag is present, it constructs a frame from embedded image width/height/pixels and copies into TED GENERATED PIC. Otherwise radarFrame is null. This is asset loading, not live radar contact discovery.

## 4. Terrain, artwork and initial features

The loader copies the 16-bit artwork tile map at world-width/32 times world-height/32 entries. It allocates width*height terrain records, clears both occupant IDs, initializes feature to 0xffff, clears lower flags and supplies mission surfaceMetal for supported newer content, otherwise zero.

Old attributes copy sample height and embedded metal, update cell flags and, for a new game, place feature IDs below the old threshold with neutral owner 10. New attributes copy height/flags and immediately preserve special ID 0xfffc handling. For a new game they place eligible short feature IDs and mission-defined features. A pending saved-game store suppresses normal initial-feature placement so restoration can reconstruct state instead.

Tileset allocation holds count plus a pointer into copied 1024-byte tile images. The loader then loads shared LOS tables, records viewport tile/block counts, allocates a padded viewport fog grid, updates cell height ranges, builds derived spatial/height layers and clears excluded border features.

It allocates the mapping buffer as terrain-cell-count times two divided by four bytes, zeroes it, allocates screen-sort lists/indices/line-count arrays with extra viewport margins, stamps feature metal, allocates temporary eyes, resets feature reproduction cursor and sets loading progress 100. These allocations support later sorting/rendering, not separate unit creation. Failed allocations are not systematically rolled back by this body.

## 5. Derived spatial buckets and height bands

BuildDerivedLayers (`0x482c20`) creates an overflow spatial record with flags 0x1f. Ordinary buckets cover 128 world units (eight terrain cells), with dimensions rounded upward. Each ten-byte bucket starts with sea-level height, accumulates terrain high values, gains edge flags for top/bottom/left/right and builds a neighborhood-expanded maximum-height byte through horizontal/vertical passes. Unit positioning and spatial queries consume this grid separately from visual fog.

The sight-height grid has terrain width/height divided by two, two bytes per entry and padded allocation count. It begins with extrema zero/255. The routine projects each terrain column using screen-height displacement, distributes sampled/interpolated heights into neighboring half-resolution columns and collects maxima/minima. It finishes by replacing extrema with weighted thirds `(2*max + min)/3` and `(max + 2*min)/3`, each floored at sea level.

These two height-band bytes are a compact occlusion approximation, not a full 3D triangle mesh. Later horizon comparisons use them and ray-step ordinal. Odd/small malformed map sizes, arithmetic denominators and allocation success are not comprehensively repaired locally.

## 6. Border exclusion

ClearBorderFeatures (`0x4833b0`) sets playable/rendered pixel extents to world width minus 32 and height minus 128. It walks the last two terrain columns, projected rows above the visible top and rows beyond the projected bottom. Its ClearFeature helper changes only IDs 0xffff or 0xfffe to exclusion marker 0xfffd. It does not universally remove every ordinary feature definition encountered in those loops.

For lavaWorld it applies that same marker-changing helper to cells at/below sea low height. The routine name therefore overstates a blanket feature deletion. Supported terrain dimensions/heights must keep its border walks within valid cells; it is not an arbitrary-dimension defensive parser.

## 7. Current sight: add and remove

AddLineOfSight (`0x482270`) and RemoveLineOfSight (`0x481d50`) are paired contribution operations. For the displayed player they invalidate fog cache and mark the view changed. Height-aware mode first checks the cached origin, selects a ray table from clamped sightDistance/32 and adjusts the origin byte count by plus/minus one.

For each ray, best horizon starts with index zero and height difference -1. Each step adds its stored offsets, bounds-checks the resulting short coordinates and reads two height-band values relative to cached eye height. A cell qualifies when first difference times best index is greater than best difference times current step ordinal. Qualifying cells increment/decrement the current-coverage byte. A stronger second-band slope updates the remembered horizon. This uses cross-products to compare slopes without division.

The ray accessor remains one-based while these callers can pass zero-valued clamp results, as documented in [table indexing](map-los-tables.md). Step getters write two-byte outputs despite wider local views. The implementation's literal narrowing/short-coordinate behavior must be preserved rather than assuming a conventional wide integer ray API.

In mask mode the cached frame index chooses a GAF circle mask. The code clips its rectangle against half-map dimensions and adjusts only cells whose mask byte is not transparency. It increments/decrements byte counters directly without saturating overflow or guarding removal from zero. Correct pairing of sight registration/removal matters, and excessive overlapping eyes can exceed byte representation.

Neither function changes permanent reveal bits. Removing the last current contribution grays already mapped terrain rather than erasing exploration history.

## 8. Permanent reveal and cache refresh

RevealAroundUnit (`0x481930`) walks the same height rays or a GAF mask and sets the owner's bit in visibilityMask only where absent. It uses XOR only after testing absence, so the bit is effectively added rather than toggled off on repeated reveals. Current count arithmetic and permanent mapping are separate even when they traverse the same footprint.

When mapping changes for the displayed player it clears fog-valid bit 8 and marks view dirty. Other-player reveals change their bits without forcing that local draw invalidation in this branch. Sharing those bits or making them usable for targeting is a separate policy.

UpdateLineOfSight (`0x4825b0`) in height-aware mode computes X from whole X/32 and projected Y from whole Z minus half a clamped 0..255 eye elevation, divided by 32. Eye elevation is query eyeHeight plus whole world Y. It updates only when origin cell changes or cached height differs by more than five. A valid old nonzero height removes coverage when dynamic sight is on, then it installs the new cache, checks bounds, adds current sight for mask 2 and reveals history for mask 1.

Outside height-aware mode it selects frame `sightDistance/32 - 5`, clamped to available frame count, derives projected 32-unit origin and subtracts frame offsets. Cell or frame change removes old coverage, stores new cache and adds coverage when mask 2, then reveals when mask 1. Zero table count is not supplied an independently valid frame fallback.

Unchanged cached location/height does not recast rays every call. Terrain/mode changes need appropriate broader invalidation/recalculation, rather than assuming moving-unit update alone rebuilds all information.

## 9. Unit wrappers and full recalculation

UpdateUnitLineOfSight (`0x4827b0`) and RemoveUnitLineOfSight (`0x482090`) populate SightQuery from unit owner, cached cell, definition sight range, frame byte, position and model-top height byte. They floor query position Y to sea level plus one before calling shared logic. A submerged unit's query eye is therefore not simply its raw underwater center height.

InitUnitSightCircleReveal (`0x482830`) and RevealNewUnit (`0x482ac0`) initialize only when dynamic sight mask 2 is enabled. They clear frame byte and either use height-aware update or explicitly initialize a circle mask, add coverage and reveal. The initializer's circle path calls reveal regardless of an extra mask-1 test in that body.

RecalculateLineOfSight (`0x4816a0`) optionally resets permanent bits: zero if mask 1 is enabled, otherwise bytes 0xff. For active player roles 1/2/3 with non-10 index it fills current coverage with zero when dynamic sight is enabled or one otherwise. It scans live unit definition slots, rebuilds their queries and initializes sight under mask 2. It then marks the map/view dirty, clears fog validity and updates mapped radar/contact drawing.

The unit scan here does not independently exclude unfinished units, carrier attachments or every gameplay invisibility state; surrounding lifecycle decides when they are registered/removed. Full recalculation also does not loop through temporary eyes in this body, so its interaction with an existing eye list must be considered by callers.

## 10. Temporary eyes and expiry

InitEyeballs (`0x481500`) resets count and allocates 0x2d0 bytes for twenty 0x24-byte Eye records. FreeEyeballs (`0x481530`) frees the pointer without nulling it here. LoadTntMap performs equivalent eye allocation as part of map creation.

AddEyeball (`0x482910`) accepts only when dynamic sight is enabled and fewer than twenty exist. It assigns the displayed player, range, eye height, world position floored above sea and expiry current tick plus supplied lifetime. Its cached cell/frame pointers point into its own record. It initializes height-aware or GAF-circle coverage and mapping, then increments count. Capacity failure is silent.

IsEyeballExpired (`0x482110`) tests expiry **strictly less than** current tick. Equality remains active. ExpireEyeballs (`0x482130`) removes expired coverage, then compacts with remove_if and updates count. Its changed local is not initialized before scanning, so a no-expired case can still read indeterminate state before deciding whether to compact. Assignment repairs the internal cache/frame pointers after records move, rather than copying self-pointers to the old location.

Temporary sight is not a permanent sensor unit and does not create a targetable object. Its lifetime uses simulation time; reference counts should be removed once on expiry.

## 11. Terrain artwork and fog edges

DrawMapTiles (`0x483fa0`) converts scroll to 32-pixel tile coordinates, calculates partial left/right/top/bottom coverage and uses DrawFrameOpaque for those edges. It then draws whole interior tiles with DrawTile using the 16-bit tile-map index into 1024-byte artwork. It does not deform artwork pixels from live terrain heights in this routine.

BuildFogTiles (`0x4843c0`) clears viewport-local two-byte edge masks and selects the information-grid rectangle around scroll with a half-tile overlap. Each absent current-sight count contributes gray-layer corner bits when dynamic sight is enabled. Each missing permanent reveal bit contributes black-layer corner bits. A source sample contributes to four neighboring fog tiles with masks 1/2/4/8, and border passes extend edges at map boundaries.

DrawFogOfWar (`0x4848e0`) rebuilds these masks only when map flag 8 is clear, then caches them. A fully black mask fills a black rectangle. Gray mask 15 draws a full gray/dithered rectangle; partial masks choose one of four phase-varying gray GAF sequences. Black partial edges draw afterward from corresponding sequences. Screen/scroll parity affects dither and sequence phase. Black and gray are thus independently composed layers, not one generic alpha value attached to units.

Current sight, mapping changes and camera scrolling must invalidate that cache through their respective services. Merely writing a grid byte without those flags need not immediately redraw the viewport.

## 12. Picking a world point from projected terrain

ClampWorldPosToTerrain (`0x484b50`) clamps input X/Y to map world bounds, sets fixed X and searches projected Z candidates from the aligned input row plus 128 down in steps of 16. Candidate world Y is max(ground, sea). It finds a projected row at/before the requested screen-like Y, queries the next 16-unit row and interpolates Z using their projected positions, then re-evaluates height.

This is a terrain/sea projected picking operation, not a simple assignment of screen Y to world Z. The interpolation denominator is not independently protected from zero in the selected condition. If no crossing is found, the function returns its last candidate. Callers receive the derived point, not an explicit failure result.

## 13. Teardown, ownership and limits

FreeMapResources (`0x483dd0`) frees eyes, feature pool, generated radar image, sorting arrays, tileset, mapping mask, terrain cells and tile map. It nulls the main map pointers, deletes viewport fog storage/container, clears both derived-grid dimensions/counts/pointers and deletes/nulls overflow bucket. The eye pointer is not separately nulled by that first free. LOS table lifetime is handled through FreeLosTables (`0x4814f0`) and the global construction entry (`0x4814c0`), not an extra destruction in this map-free body.

Mutable map persistence belongs to [plot-map saves](map-plot-map.md); feature instances, units and player grids have separate restore paths. There is no direct packet serializer in this module. Radar/sonar, cloaking, contact classification and automatic weapon acquisition require their own consumers and cannot be inferred solely from sight circles.

All 29 entries, TNT loading, environment conversion, derived grids, both visibility algorithms, temporary eyes, tile/fog rendering and projected picking are covered through static source inspection. Limits such as byte-count wrap, ray indexing, uninitialized eye-change state, zero-size/divisor assumptions and partial-allocation teardown are explicitly retained. No build or game execution was performed.
