# Movement classes, passability maps and occupancy refresh

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `units/movement_class`, twenty annotated entry points from `0x440230` through `0x440cd0` in [movement_class.cpp](../../../src/units/movement_class.cpp). The shared [MovementClass declaration](../../../src/units/movement_class.h) defines its 32-byte records. This module owns the 32-class table, compact passability maps, full/partial rebuilding and timestamp-based occupancy refresh. It also contains logo selection, a four-field copy helper and map compatibility gating.

## 1. Class state and parameters

Each class stores a name, footprint X/Z, minimum/maximum water depths, normal/preferred land and underwater slope limits, map dimensions, packed cells and a timestamp. Units can share the same class/map even though their individual steering or weapon state differs.

Static initialization/destruction at `0x440230`/`0x440290` constructs and releases the 32-entry table. The constructor, `0x4402e0`, starts with null name/map, zero footprint/dimensions/timestamp, maximum depth 10000, minimum depth minus 10000 and all slope thresholds 255. The destructor, `0x440320`, releases name and cell storage.

`ReadMoveInfo`, `0x440340`, reads FootPrintX/Z with zero defaults and depth/slope fields with their current values as defaults. badslope defaults to half maxslope; badwaterslope to half maxwaterslope. It limits maxslope to maxwaterslope, then badslope to the resulting maxslope and badwaterslope to maxwaterslope. Values are narrowed into short/byte fields as part of assignment, not comprehensively range-validated before narrowing.

`FindMovementClass`, `0x440420`, linearly searches the 32 nonnull names case-insensitively. A miss returns null. The [unit loader](units-unit-types.md) reads MOVEINFO classes and falls back to FBI-local movement parameters when a named class is unavailable. A default zero-footprint record is not by itself a valid ready-to-use navigation shape.

## 2. Two-bit storage layout

Each pass-map location is a two-bit value. Sixteen vertically stacked cells share one 32-bit word; words run across X within a sixteen-row band. The word index is width times (Y divided by 16), plus X, and bit shift is twice (Y modulo 16).

`ResizePassMap`, `0x440470`, replaces storage with width times ceiling(height/16) words. It records dimensions, releases old storage and allocates the new buffer when nonzero. It does not initialize every cell to a final passability value; building fills it afterward. The legacy release/allocation forms are visible in the body and do not supply an independent failure recovery path.

`SetPassMapCell`, `0x4404c0`, clears the chosen two bits and ORs in the supplied shifted value. It assumes valid coordinates and a value within 0..3. It does not independently mask an arbitrary oversized value down to two bits before insertion. The inline Get reads the same layout and masks to 3; InBounds is available separately rather than automatically called by every accessor.

## 3. Raw terrain grades

The [terrain module](map-terrain.md), especially `0x47de60`, supplies the raw classification:

| Grade | Meaning |
| ---: | --- |
| 0 | Blocked by feature/occupancy, depth or excessive slope |
| 1 | Passable but outside the preferred slope threshold |
| 3 | Preferred/passable terrain |

The raw test resolves feature continuation cells, applies feature blocking and primary occupant/timestamp policy, checks terrain bounds against sea level minus water-depth limits, then chooses land/underwater slope thresholds according to the cell's lower bound. A map is class-dependent, not one universal traversability bitmap for all ships and vehicles.

Occupancy also depends on time: a primary occupant without motion, or with motion timestamp older than the class timestamp, blocks. More recent moving occupancy can be treated differently so transient movers do not become identical to permanent structures in cached route data. Immediate collision/placement still has separate live-object tests.

## 4. Full footprint construction and edge grading

`BuildPassMap`, `0x440500`, allocates one scratch row sized for the larger map-plus-footprint axis, with padding. Its first pass obtains raw grades across each terrain row, computes the minimum over the horizontal footprint extent and writes the packed map. A fully preferred interval is downgraded to 1 when its neighboring samples are below 3.

Its second pass reads each packed column into scratch storage and repeats minimum/edge grading across the vertical footprint extent. Zeroed scratch padding makes boundaries behave as blocked neighbors rather than allowing a footprint to continue outside the map. The scratch buffer is released afterward.

This two-pass construction expands point terrain into footprint clearance and marks less-preferred space near nonpreferred edges. A one-cell-wide gap can be blocked for a large ship even when every gap cell is individually passable. A grade-1 fringe can influence path preference without making the position completely impossible. The route search's cost policy is a separate consumer.

`BuildAllPassMaps`, `0x440940`, counts named classes, resizes their maps to the current terrain size and builds each. It updates a loading-progress byte and finally assigns 100. Its accumulator begins at 100 before adding another 100 per class, so transient progress values do not implement a perfectly bounded zero-to-100 fraction at every intermediate store. With no named classes the division branch is skipped and final progress still becomes 100.

`NopAfterFeatureEnum`, `0x440930`, is empty and introduces no additional passability transition.

## 5. Local refresh includes footprint influence

`MovementClass::RefreshPassMap`, `0x440830`, accepts a changed origin and size. It expands left/top by the class footprint and extends right/bottom one cell beyond the changed rectangle, clips to map dimensions, and recomputes each affected origin using terrain's whole-footprint-plus-edge-strips query.

The changed physical cells and the affected candidate origins are not the same rectangle. A building/tree added at one cell can invalidate a larger set of origins for a wide footprint, and its edge preference can change nearby grades. Refreshing only the visibly occupied cell would leave stale route data.

`RefreshAllPassMaps`, `0x440a40`, applies that operation to every named class. [Feature placement/removal](map-features.md) and [unit occupancy/yard changes](map-terrain.md) call these services so route caches track world changes.

## 6. Unit timestamps and delayed occupancy updates

`RefreshPassMapsForUnit`, `0x440a70`, uses a unit's motion timestamp when motion exists. It saves the old timestamp, stamps the current tick and refreshes classes whose timestamp is newer than that saved value. Without motion it refreshes all named classes over the object's origin/footprint.

`MovementClass::RefreshMovedUnits`, `0x440af0`, advances a class's reference time to current tick minus 30, clamped so early ticks do not underflow. It temporarily stamps the requesting mover current, refreshes its footprint when its previous stamp predates the old class time, and, when the window advanced, scans live moving units whose timestamps crossed the old-to-new interval. Their footprint regions are refreshed. It then restores the requesting mover's prior timestamp.

`RefreshUnitIfStale`, `0x440be0`, provides a narrower operation: refresh the requesting footprint only when its motion timestamp is older than the class timestamp.

These paths maintain the distinction between recently moving units, older/static obstructions and class caches at different refresh ages. They do not imply that every route is rebuilt every tick, that all moving units are collision-free, or that a timestamp alone authorizes movement through an occupied footprint.

## 7. Clearing class resources

`FreeMovementClasses`, `0x440a00`, releases each class name and pass-map allocation, clears dimensions and pointers, and leaves the static table storage itself available until its normal destructor. It does not free live unit records or the global terrain cells. Existing definition/path pointers into the table need surrounding lifecycle discipline before class state is reused.

## 8. Adjacent helpers and map checksum policy

`FindUnusedLogo`, `0x440c10`, marks logos used by active controller roles 1/2/3 with valid slot indices. Color indices at or above 9 occupy mark 9. It returns the lowest free index, or zero when none is free. Zero is consequently both a valid free result and the all-used fallback, not a unique failure sentinel.

`CopyPathLockFields`, `0x440ca0`, copies four integers from source offset +4 to destination +0x99. Their rectangle-like shape is not enough evidence to assign independent geometric boundary semantics here; the helper simply preserves those fields.

`CheckMapCrc`, `0x440cd0`, rejects zero terrain-file length. With a host-info record advertising version at least 2, or major 1/minor at least 2, it compares the mission's computed map checksum with the advertised CRC. Older versions or no host skip the comparison and return success after the length gate.

The [mission checksum](map-map-load.md) combines selected OTA/TNT content. This compatibility check does not hash the generated pass-map bytes and does not prove every external model/texture asset is identical. Its location near movement classes should not be read as a pathfinder validation test.

## 9. Completeness and investigation points

All twenty annotated entry points are covered. The module owns runtime navigation caches, with no independent save-bank or packet payload. Definition/TDF data supplies class parameters; terrain supplies raw grades; unit/feature mutation supplies invalidation; pathfinding consumes the resulting values.

For a naval route that cannot use a channel, compare footprint width, depth and slope thresholds before inspecting the ship model. For a route that stays blocked after wreck removal, trace origin-expanded refresh calls and class/mover timestamps. For a passable but avoided shoreline, inspect grade-1 edge/slope preference and the pathfinder's costs. These are distinct causes even when the rendered water looks continuous.
