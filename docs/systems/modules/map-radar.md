# Minimap imagery, mapped terrain and contact display

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md) | [Sight and exploration](map-line-of-sight.md)

Module group: `map/radar`, eight functions in [radar.cpp](../../../src/map/radar.cpp), `0x4665d0` through `0x466dc0`. This module constructs the minimap picture, applies information shading, draws contacts/range overlays and maintains picking records. It consumes precomputed unit visibility/contact flags; it does not itself build radar/sonar/jammer detection coverage.

## 1. Picture size and terrain source

BuildRadarPicture (`0x466780`) fits playable map dimensions into a 126-pixel maximum dimension, preserving aspect through integer arithmetic. It records centered gadget origin and resulting width/height, then allocates the base picture surface.

If map loading supplied radarFrame, it downsamples that image into the surface. The [GAF helper](../../../src/graphics/gaf.cpp) defines source/destination order; a reversed local parameter label in a declaration does not reverse this transfer. Without an embedded image, it allocates a temporary surface twice the final dimensions, samples terrain artwork at scaled world positions, resolves each 16-bit tile index and chooses that tile's local pixel. Out-of-range tile indices fall back to tile zero. It downsamples the temporary picture and frees it.

The base image represents terrain artwork, not live terrain occupancy or sensor range. Map dimensions and target dimensions must be nonzero for the divisions. Degenerate aspect ratios can truncate a short dimension to zero without a local corrective clamp.

ResizeRadarPicture (`0x4665d0`) separately rescales an existing frame. Null returns. It copies original pixels to TEMP RADAR PIC, changes the destination header width/height, clears it and draws the selected source quad into an aspect-fitted centered destination quad. Fitting uses supplied dimensions minus 32 horizontally and 128 vertically, matching playable border exclusion. It frees the temporary afterward. It does not allocate a replacement destination backing buffer merely because header dimensions change, so sufficient caller-provided storage is a precondition.

## 2. Initialization, rectangles and cleanup

InitRadar (`0x4669b0`) builds the picture, allocates mapped and final surfaces, and records inclusive display bounds as origin through origin-plus-size-minus-one. It marks terrain mapping dirty, starts blink timer at seven and clears blink phase. It does not reserve any radar emitter unit or activate every sensor on the map.

FreeRadar (`0x466aa0`) frees picture, mapped and final surfaces and nulls those three pointers. Other dimensions/picking-list state are not independently reset in that body.

CalcRadarViewportRect (`0x466b70`) scales actual camera scroll into minimap coordinates, adds gadget origin, and derives inclusive right/bottom from viewport tile dimensions times 16. The rectangle is a camera footprint, not a sight-range circle. Camera clamp/update calls maintain it when the view moves.

## 3. Mapping and current-sight shading

UpdateRadarMapped (`0x466c20`) rebuilds only when mapChanged is set, clearing that bit before walking pixels. Each minimap pixel maps to one half-resolution information cell. If the displayed player's permanent mapping bit is absent it writes fogColor. If mapping is present and that player's current-sight byte is nonzero it copies the base terrain pixel. Otherwise it copies through the display colorMap to produce the mapped-but-unobserved shade.

The private byte-grid name explored is misleading here: its nonzero value is live sight coverage, while the unsigned-short mapping buffer preserves exploration history. The update marks final redraw pending. It does not increment sight or reveal unexplored cells merely by drawing them.

DrawRadar (`0x466b00`) draws only when pending is set, clearing pending, copying final surface at gadget origin and drawing the camera rectangle in HUD palette color 14. The pending flag and mapChanged flag represent different stages: one controls rebuilding shaded terrain, the other requests screen presentation after composition.

## 4. Unit contact eligibility

DrawRadarUnits (`0x466dc0`) clears the hot-radar count and starts by copying mapped terrain into final surface. It scans the inclusive unit array and requires a nonzero definition index. A unit draws when information restrictions are disabled, runtime contact mask 0x300 is nonzero, or its owner is the displayed player.

Information restrictions are disabled in this body when neither mapping bit 0 nor dynamic-sight bit 1 is set, or UI option bit 9 overrides them. Otherwise it consumes the already prepared 0x300 state. It does not independently test alliance, cloak, radar distance, sonar depth or jammer intersections in this loop. Those upstream rules determine the mask; this renderer is not the complete sensor algorithm.

Unit X scales whole world X. Y scales whole Z minus half whole height, so aircraft icons can shift vertically relative to their horizontal occupancy. The base logo uses owner color. A nonzero recentlyDamagedTimer suppresses its ordinary logo on one blink phase. The hovered ID receives the highlight logo separately, even during that ordinary-logo suppression.

It appends a ten-byte picking record for every eligible unit: short ID and two integer screen coordinates including gadget origin. Blinking does not remove the picking record. The list has no capacity check in this function; [unit allocation](units-units.md) supplies its storage. This record is an interface hit list, not another target/sensor cache.

## 5. Selected-unit range overlays

Only selected units draw their range circles. Radar, sonar, radar-jammer and sonar-jammer values are shown when the unit is active, or when its definition is not onoffable. Detection circles use one HUD palette entry and jammer circles another. Radius scales by minimap width divided by playable map width for both axes, rather than independently correcting an ellipse.

An antiweapons definition, flags1 bit 29, additionally scans all three slots for interceptor bit 30. Displayed coverage radius is weapon coverage **minus 512 world units**, scaled to minimap width. Nonzero stockpile uses a dashed circle with dash parameter 32 and blink phase; empty stockpile uses an ordinary circle. This visual radius differs from [interception acquisition](weapons-weapons.md), which uses the full coverage square around stored projectile aim. Negative displayed radius is not clamped locally.

Drawing a sonar circle does not prove a submerged enemy inside it is currently detected. Activation, upstream coverage, target state and other rules remain separate. Conversely hiding a selected overlay does not delete the unit or its definitions.

## 6. Projectile minimap symbols

The same composition pass walks live projectile records using their current projected position. Ordinary weapons without targetable/interceptor bits draw one pixel unless noradar bit 6 is set, provided current information permits that position or the projectile player equals the displayed player.

Weapons with either targetable bit 29 or interceptor bit 30 instead use nuclogo colored by projectile player. Their eligibility is information visibility or owner-unit player equality. This special branch does not repeat the ordinary noradar check. It assumes a usable owner pointer when testing that ownership fallback, so ownerless specially flagged projectiles are not fully guarded here.

The information helper uses the live per-player byte grid when dynamic sight is enabled; otherwise it uses the displayed player's permanent mapping bit. It bounds-checks the half-resolution projected cell first. This is not a radar-emitter coverage query despite local helper names OnRadar. The composition loop also does not independently filter dead/burst counters here; normal projectile update/compaction and caller order supply the usual input state.

After composition it sets pending for DrawRadar. Projectile icons do not enter the unit picking list, and this pass does not select or fire an interceptor.

## 7. Lifetime, cadence and completeness

The timer initialization and phase consumption are here, while the routine advancing blink time is outside these eight functions. Do not infer a seven-tick repeating blink cadence solely from the initial timer value. Likewise the caller decides when mapping/contact composition runs; DrawRadar's pending gate is not a fixed simulation-frequency guarantee.

There is no dedicated minimap serializer or network packet here. Terrain comes from map loading, current/mapped information from sight and sharing systems, unit flags from upstream visibility processing and projectiles from weapon simulation. On restore the surrounding lifecycle rebuilds surfaces rather than treating them as authoritative game state.

All eight module functions, picture/fog/contact composition, range overlays, picking and cleanup are covered through static source inspection. Remaining full sensor-eligibility and contact-mask production belong to their actual source modules. No builds or game execution were performed.
