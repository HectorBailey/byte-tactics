# Order destinations, target geometry and path goals

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `orders/order_targets`. All 111 annotated entry points in the address interval `0x44ce40` to `0x44f6e0` are accounted for here. Primary implementation: [target objects and path goals](../../../src/orders/order_targets.cpp), [rectangle perimeter generation](../../../src/orders/order_targets_44da00.cpp), and [point-vector insertion](../../../src/orders/order_targets_44ec30.cpp). The group includes target geometry, destination serialization, local/remote waypoint state and pathfinder lifetime. Actual search expansion belongs to [pathfinding](../../../src/ai/pathfind.cpp); action scheduling belongs to [order instances](orders-order-list.md).

## 1. Three different objects participate in movement

An action order owns the intent and receives status flags. An OrderFx-derived destination describes acceptable cells or a changing world position. A PathGoal-derived object belongs to the moving unit and exposes waypoints to steering. These objects have different lifetimes and completion tests. The target object's type number does not identify an attack/repair/build command.

| Destination type | Type value | Contract |
| --- | ---: | --- |
| OrderFx base | 1 | Default virtual implementation and source-order flag forwarding |
| PathOrder | 2 | Fixed position, linked unit/piece or fixed position associated with a unit |
| AirManeuverOrder | 3 | Advancing point and horizontal step vector, optionally turning the step |
| ApproachRadius | 4 | Circular cell acceptance and approximate distance outside its radius |
| RingApproach | 5 | Two-radius acceptance with distinct containment and distance conventions |
| PointMarker | 6 | Rectangle border cells and distance to that border |

The base constructor stores its source order. AddFlags (`0x44ced0`) ORs bits into that order's status at +0x4e when a source exists. Replacing/completing a destination therefore informs the action without directly destroying the action or popping its queue. Base FillGoalCells clears the supplied list; default distance is zero, ContainsCell is false, desired-heading lookup fails, KeepAfterComplete is false and serialization-to-bits does nothing. ContainsUnit delegates to the virtual cell test using the unit's cell coordinates. Base IsFxStyle returns true; PathOrder and AirManeuverOrder return false. The base save method returns zero.

Deleting wrappers restore base-class dispatch and free memory only when deletion flag bit 0 is set. PathOrder additionally unlinks its embedded unit reference. These wrappers are included in the module's entry-point coverage, rather than treated as separate gameplay systems.

## 2. Coordinate spaces and circular approach

World positions use 16.16 fixed-point X/Y/Z. Cell geometry uses a pair of shorts. ApproachRadius construction (`0x44cf60`) subtracts the source unit's +0x7e/+0x80 pair shifted by 19, adds 0x80000 for rounding, then shifts right by 20. Thus a cell spans 16 world units and the source-unit offset participates in the conversion. This pair is the unit footprint in the unit-order view. Local provisional target declarations call it an origin; the subtraction converts a centered world position to a footprint-relative cell.

The inverse world-position method (`0x44d2c0`) writes X/Z as `(cell coordinate * 2 + source offset) * 2^19`. It leaves Y untouched. Callers cannot infer a terrain height from this successful return.

ApproachRadius stores the world-unit radius and `(radius / 16)^2`, using integer division. ContainsCell and ContainsUnit compare squared cell distance to that stored square inclusively. A small positive radius below 16 consequently accepts the center cell only. Goal-cell generation clears the vector and inserts the center alone; it does not enumerate a circle perimeter.

The distance estimator (`0x44d350`) uses `18 * major axis delta + 7 * minor axis delta`, subtracts the stored world-unit radius and clamps to zero. This deliberately differs from the exact squared-cell containment test. Search heuristic distance and geometric acceptance can disagree near a boundary.

## 3. Two-radius approach and rectangle borders

RingApproach construction (`0x44d3b0`) uses the same center conversion, stores both radii and stores each integer-divided-by-16 square. The literal containment rule is **distance squared <= first radius squared and >= second radius squared**, inclusive. The distance estimator (`0x44d840`) instead calls the first raw radius inner and the second outer: values above the second return excess, values below the first return deficit, otherwise zero. These conventions are not interchangeable. The source's names do not justify silently reversing one test to describe an ideal annulus. If the first radius is smaller than the second, the containment interval can be empty even though the heuristic still returns a distance. This is a directly observed inconsistency; particular action behavior depends on the arguments supplied by its caller.

Ring goal-cell generation inserts one point offset from the center's second coordinate by `(first radius + second radius) / 32`. FillWorldPos (`0x44d720`) converts the center, obtains a heading from the source unit to that point, then subtracts the helper's direction vector at the mean radius. The helper's X/Z components are negative trigonometric products. Y is not established by this operation. This is an approach sample, not a list of all acceptable cells.

PointMarker construction (`0x44d8a0`) makes its first X/Y endpoints relative to the source's +0x7e pair, but computes the other endpoints as input position plus size without the same subtraction. The document preserves that asymmetry. ContainsCell (`0x44dcb0`) accepts the inclusive rectangle **border**, not its filled interior. [AppendGoalCell](../../../src/orders/order_targets_44da00.cpp) clears the vector, emits top/bottom points for every inclusive X, then left/right points for interior Y. Degenerate rectangles can produce duplicate points; there is no deduplication pass.

PointMarker FillWorldPos (`0x44dc60`) uses the midpoint of its X endpoints and its second Y endpoint, then applies the source-unit offset conversion. This is the midpoint of one edge, not the rectangle center. Y in the output 3D vector remains untouched.

Its distance function (`0x44dd00`) returns 16 times the distance to the nearest border when inside. Outside an edge it returns 16 times the perpendicular distance. Outside a corner it uses `16 * major + 6 * minor`. Interior points therefore generally have positive distance. A comment claiming zero throughout the interior would not describe the implementation.

## 4. PathOrder flags and constructors

PathOrder stores its moving unit separately from an embedded reference to a destination unit, a piece index, cached world position, optional radius/altitude/heading and a following offset. The [unit reference module](units-unit-targets.md) supplies invalidation/unlink behavior. A target unit disappearing need not leave a valid position source.

| Flag | Effect in this module |
| --- | --- |
| 0x01 | Destination has a unit reference; required for live reference position resolution and persistent following |
| 0x02 | Add a heading-relative following offset; desired heading can face the referenced unit |
| 0x04 | Allow referenced-unit heading lookup and require exact target-heading equality in the ordinary completion branch |
| 0x08 | Explicit altitude enabled; ordinary completion checks vertical error |
| 0x10 | Use strict horizontal approach-radius completion instead of the other completion conditions |
| 0x20 | Position is included in bitstream representation; SetAltitude can derive its Y from ground/water |
| 0x40 | Explicit heading enabled, also used as relative offset heading during following |
| 0x80 | Bypass updating cached position from the linked unit while retaining reference-related heading behavior |

The order-plus-target constructor (`0x44e190`) starts at the moving unit's current position, uses piece -1 and zero radius/altitude. A target whose definition has canfly bit 11 selects flags 7 and a following offset of the mover's primary weapon range, or 100 world units if that range is zero. A nonflying target selects flag 1. This selection examines the target's definition, not the mover's flight capability.

The unit/piece constructor (`0x44e250`) uses flags 5 and the specified piece. A fixed-position constructor (`0x44e2d0`) uses flag 0x20 with no unit reference. The unit-plus-fixed-position constructor (`0x44e330`) uses flags 0xa3: it keeps the reference and supplied position while suppressing live position updates. KeepAfterComplete (`0x44e3a0`) returns true only when flag 1 is set and the reference remains nonnull. It is a persistence policy after arrival, not itself a proximity test.

## 5. Position, altitude, heading and arrival

FillWorldPos (`0x44e3c0`) has two branches. With flag 1 and without 0x80, it refuses a null reference or a target in the overflow spatial bucket. It queries the target piece position, optionally offsets horizontally relative to the target heading plus explicit heading, then adds altitude in 16.16 units. The piece query's exact model behavior is covered in [script geometry](units-unit-scripts.md).

Otherwise, if explicit-altitude flag 8 is absent, it rebuilds cached Y from the moving unit definition's cruise-altitude field plus either sea level or the current spatial bucket's height byte. Selection uses definition flag bit 22; this method alone does not establish a public FBI name for that bit. It is unsafe to infer the reference unit's ground height here. All successful branches cap Y above at 511 world units, copy the cached position and return success. There is no corresponding lower clamp in this function.

SetAltitude (`0x44e6c0`) sets flag 8 and stores a short. For a position-bearing target with flag 0x20, it immediately sets cached Y to the greater of ground height and sea level plus altitude, capped at 511. For a live reference, the later position query adds altitude to the queried piece. SetHeading sets flag 0x40; SetApproachRadius sets flag 0x10. Both store short values without range normalization.

GetDesiredHeading (`0x44e530`) first considers a live reference when either 0x04 or 0x80 is present. Flag 2 selects heading from mover to target; otherwise explicit heading wins, otherwise the target's own heading is returned. Outside that branch an explicit heading still succeeds; with neither source the method fails. The ordering means explicit heading can be a relative offset for position calculation while facing the target still wins for heading output.

Arrival (`0x44e5b0`) queries a position and computes horizontal Euclidean distance in world units. With flag 0x10 it returns **radius > distance**, strictly, and bypasses null-reference, heading and vertical checks. Otherwise distance must be at most 0.5 world units, flag 1 requires a live reference, flag 4 requires exact heading equality with the target and flag 8 allows at most one world unit of vertical error. There is no angle tolerance here. The position-query return value is not checked before using its output, and the flag-4 test assumes a usable target. Valid construction/caller state is therefore part of this interface's contract.

## 6. Advancing aircraft maneuver targets

AirManeuverOrder (`0x44e740`) stores an owner unit, initial target point and step vector, with heading mode initially disabled. Its position query (`0x44ea60`) returns the current target, then advances internal X/Z by the step. It does not add the step's Y. Calling the query twice advances the destination twice, so it is not a pure getter.

When heading mode is enabled, it computes the step vector's heading and rotates its horizontal components toward the stored heading by at most one eighth of the unit definition's turn rate per query. The angle difference is a signed short, giving the engine's wrapped angular difference. A rotation sets step Y to zero. SetHeading enables this mode; SetAltitude is deliberately an empty operation.

GetDesiredHeading points from the unit toward the current internal target. Completion (`0x44eb60`) succeeds when horizontal distance is strictly below 48 world units, or, with heading mode, when the step vector's heading exactly equals the stored heading. KeepAfterComplete always returns false. Thus this target can finish by completing its turn even when it has not reached the advancing point. Its geometry does not itself select an attack victim or fire a weapon.

## 7. PathGoal interface and source-order notification

The base PathGoal owns a unit pointer and a destination pointer. SetPathOrder (`0x44ef90`) tells the previous destination's source order flag 0x80 before replacing it. This notification is independent of freeing the destination. Base readiness means a nonnull destination; its tick, position fill, drawing, pose export and network serializers are empty, while repath claim and network-change tests return no work. Derived packed-motion goals live in another module and must not be inferred from these stubs.

AiSearchGoal adds a 20-point short-coordinate array, count, four packed flags and a repath timestamp. Construction starts with no active path, no requested repath, and the changed flag set. SetWaypoints (`0x44f080`) copies at most 20 points and marks a nonempty result active. For zero points it tests whether the owner already satisfies the destination; failure reports source-order flag 0x40, then clears active. Both cases clear the repath-request flag and set changed. A zero result does not explicitly reset the old count, so active state determines whether stale points are usable/transmitted.

TruncateWaypointsFrom (`0x44f100`) discards the specified prefix by overlapping forward copy, decrements count, deactivates below two points and marks changed. It assumes a valid nonnegative prefix within the count. Position fill converts world-unit shorts to `(X, 0, Z)` fixed-point positions and repeats the last point when the caller requests more positions than remain. It assumes at least one stored point; zero-count calls would index before the array. Steering must honor readiness.

## 8. Arrival, path reuse and repath cadence

AiSearchGoal TickTowardGoal (`0x44f1a0`) first asks the destination whether the unit has arrived. Success sends flag 0x20 to the source order and clears the destination if KeepAfterComplete is false. Clearing also sends replacement flag 0x80 through SetPathOrder. Persistent unit following can therefore report arrival without detaching its destination.

With two or more points, reaching within squared horizontal world-unit distance 25 of point 1 shifts the array from point 1 forward, removing its old point 0. Below two points the path becomes inactive. A remaining destination requests repath when the owner's occupancy bit 2 is set or fewer than two points remain.

SetPathOrder (`0x44f2a0`) cancels any currently scheduled search for this goal, reports 0x80 to the old source, deactivates and installs the new destination. A path with at least three points can be reused immediately if its final point, divided by 16, passes the new destination's cell test. Otherwise a successful world-position query permits reuse when the final point is less than half the mover's distance from the new destination. If neither condition holds, a usable owner motion object without flag 0x800000 permits a two-point direct seed from owner to destination. Installing a destination initially requests search even if that direct seed becomes active. Null installation clears repath request.

TryClaimRepath (`0x44f260`) returns this goal only when the request flag is set and the current game tick is at least the last claim plus 60, then records the current tick. This is a claim throttle, not a promise that a search finishes every 60 ticks. SetPathOrder can reset an older timestamp to zero using its ten-tick comparison. The comparisons use unsigned arithmetic and do not add a separate wraparound repair. Destruction aborts a matching active search before base cleanup.

## 9. Local-to-remote waypoint synchronization

AiSearchGoal HasNetUnitState (`0x44f480`) becomes true when the changed flag is set or cached occupancy bit 2 differs from the owner's current value. Its serializer (`0x44f4a0`) writes one occupancy-mode bit, then a two-bit count. Inactive paths send zero points; active paths send at most the first three of the local 20. Each point contributes two 16-bit coordinates. It caches the sent occupancy bit and clears changed after writing. The payload is therefore 3 bits plus 32 bits per transmitted point, excluding its enclosing unit packet.

PatrolGoal is the receiving waypoint holder here, despite a name that could suggest a player-issued patrol command. Its reader (`0x44f5c0`) restores the occupancy bit and zero to three points from that representation. Readiness requires at least two points. Its position fill performs the same fixed-point conversion and last-point repetition. It inherits base no-op search/tick behavior and does not run the local AiSearchGoal search machinery merely because it has a similarly shaped list.

Changing a long local path need not transmit every remaining point. Remote steering receives a short horizon through later unit synchronization. Packet scheduling and transport are handled by [unit positioning/replication](units-unit-position.md) and networking modules, not by the point serializer.

## 10. Destination saves and network representation

| Object | Save record | Restored payload |
| --- | ---: | --- |
| ApproachRadius | 16 bytes | Packed short center, radius, squared radius |
| RingApproach | 24 bytes | Center, two radii and two squared radii |
| PointMarker | 20 bytes | Four integer rectangle endpoints |
| PathOrder | 54 bytes | Mover/target IDs, five shorts, cached XYZ, following offset and temporary reference-shaped fields |
| AirManeuverOrder | 42 bytes | Owner ID, flags, target and step vectors, heading and additional saved shorts |

Save methods open the named bank box, seek to zero and write the fixed-size record, returning success without testing the resulting write count. The simple area records leave their first four bytes uninitialized. PathOrder and aircraft records leave their first eight bytes unassigned; additional structure bytes are not a portable object format. Loaders only apply payloads after a full-size read. Circle/ring constructors guard their source/file/name pointers, while the rectangle and more complex loaders rely on valid supplied arguments. A short read is not a safe default initialization of every payload field.

PathOrder restores the mover and referenced unit via recursive LoadUnit lookup and reconnects its embedded reference. It does not deserialize saved process pointers into a live reference list. Temporary reference nodes used during record handling are unlinked. Its bitstream representation is different: 8 flags, optional piece and target ID (16 bits each), optional radius/altitude/heading (16 each), and optional XYZ (32 each), in that order. There is no following-offset field in that packet. The network constructor zeros absent radius/altitude/heading but does not generally initialize all absent positional/following fields. Invalid received IDs are not checked against array bounds in this constructor.

AirManeuverOrder's bitstream writes only one flag bit, six 32-bit target/step components and an optional 16-bit heading. Its save-only fields do not travel there. The reader receives the owner from its caller and installs source zero. Neither network target representation recreates a local source action merely by decoding its geometry.

## 11. Lifetime, support routines and boundaries

CreatePathfinder (`0x44f6a0`) allocates the shared game pathfinder. DestroyPathfinder (`0x44f6e0`) deletes it and clears the game pointer. These wrappers do not check for an already existing object, so creation is a lifecycle operation rather than an idempotent ensure call.

The point-vector insertion specialization and `_Destroy`, size, uninitialized copy/fill and copy-range helpers (`0x44ec30` to `0x44eef0`) implement storage for four-byte pairs of shorts. Destruction has no per-element work because points are trivial. Insertion grows/copies storage as needed; it supplies no spatial filtering, collision handling, deduplication or target eligibility. Constructors, scalar deleting destructors and these library specializations account for the remaining annotated entries.

For naval units, this module determines which cells or linked-unit positions count as arrival, when following remains attached and which short waypoint horizon a remote peer receives. Water passability, collision, turn/acceleration execution, repair/build/transport range selection and combat eligibility belong to their calling systems. It contains no construction wireframe renderer, wake generator or weapon selection policy. Those systems consume positions and status outcomes established here.

This reference is based on static source inspection. The ring convention mismatch, asymmetric rectangle endpoints, unchecked position-query result, incomplete short-read defaults and zero-count waypoint preconditions are explicit implementation limits. No gameplay run was used to confirm how particular assets exercise them.
