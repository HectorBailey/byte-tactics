# Terrain placement, occupancy, and spatial queries

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `map/terrain`. The implementation is [terrain.cpp](../../../src/map/terrain.cpp) and its four retained companion files: [map insertion](../../../src/map/terrain_47cc30.cpp), [map removal](../../../src/map/terrain_47d0e0.cpp), [footprint height](../../../src/map/terrain_47d820.cpp), and [candidate footprint probing](../../../src/map/terrain_47e2d0.cpp). This writeup covers the major contracts of all 26 annotated entry points.

## 1. Three representations of a unit's location

Terrain cells, spatial buckets, and attachment links serve different purposes. Terrain cells record which unit occupies a particular footprint cell and whether building geometry blocks it. Spatial buckets accelerate searches over larger regions. Attached children belong to a parent relationship and need not appear as independently linked bucket roots.

The terrain cell is a packed 13-byte record containing two unit IDs, sample and bound heights, metal, a feature identifier, feature-origin offsets, and flags. A unit holds a world position, footprint-cell position, footprint size, spatial bucket, parent attachment and linked-list fields. The world X/Z coordinates use fixed point; footprint coordinates are integers.

A single field named owner is therefore ambiguous. `SetOwner` in this module changes the spatial bucket; it does not transfer the unit to another player. Player ownership transfer belongs to unit commands.

## 2. Spatial bucket maintenance

`SpatialBucket::PrependUnit` inserts a node at a bucket head. `UnlinkUnit` walks pointer links to the requested node, reconnects its predecessor to the next node, and clears the removed link. Its shown search assumes the node belongs to the list; it has no explicit unsuccessful-search return.

`SetOwner` moves a root between bucket lists when its bucket changes. For an attached unit it changes the bucket pointer without performing the same independent-root unlink/relink sequence. `ClearFootprintAndUnlink` first removes cell occupancy, then unlinks an unattached root from its bucket and clears the bucket pointer.

These operations require valid membership relationships. Reusing a unit link simultaneously as an unrelated list node would break those assumptions. The code's parent test explains why attachment and spatial-root membership cannot be reconstructed independently.

Evidence: `0x47cb00`, `0x47cb40`, `0x47cb60`, `0x47cbd0` in the main file.

## 3. Adding occupancy

Map insertion updates the mover's occupancy timestamp when a motion object exists. It checks footprint bounds; an out-of-bounds unit is assigned to an overflow bucket rather than proceeding with ordinary cell writes. An in-bounds unit is assigned to a spatial bucket using its world X/Z position divided into larger regions.

The cell-writing branch distinguishes the building-associated runtime flag from two mobile movement modes. Buildings consult each yard-mask byte. Depending on yard-open state, the selected mask bit determines which cells claim the primary unit slot; another mask bit sets the blocking flag. The building branch also updates height bounds in a rectangle padded by one cell and refreshes movement pass maps.

Mobile mode 1 writes the primary unit slot. Mode 2 writes the secondary slot. The two slots are not an unrestricted list of all overlapping models, nor do their names alone define surface and underwater layers.

An existing occupant does not always get overwritten. The branch considers whether that occupant has an active type-3 owner, sets conflict/reclaim flags on the relevant units, and chooses whether to replace the slot or leave it for later resolution. Those flags are part of the reconciliation mechanism for competing occupancy information.

Evidence: `AddUnitToMap`, `0x47cc30`, in the insertion companion.

## 4. Reclaiming and removing occupancy

`ClaimFootprintCells` returns immediately unless the reclaim/dirty flag is set. It clears that request flag and revisits the footprint using building yard masks or the current mobile slot. It can mark conflicting objects and retain a pending request rather than pretending every conflict resolved synchronously.

`ClaimFootprintsInArea` applies the footprint-claim visitor to overlapping objects. The visitor's virtual callback is a thin call to `ClaimFootprintCells`. `ForceNeighborFootprintReclaim` marks the requesting object, visits its area to reconcile footprints, and refreshes pass maps. The adjacent `0x47caf0` routine is empty.

Removal does not indiscriminately clear every occupied cell. It clears a unit slot only where that slot still contains the removed unit's own ID. The building path also removes its blocking bits and recomputes padded height bounds. The overflow-bucket case skips ordinary terrain-cell removal.

After removal, a conflict flag can cause neighboring objects to reclaim their footprints. Pass maps are refreshed through the unit-aware update path. Removing a visible unit therefore has consequences beyond its sprite: cell ownership, blocking heights and cached navigation values must agree.

Evidence: `0x47c790`, `0x47cac0`, `0x47caf0`, `0x47db20`, `0x47ed30`; removal `0x47d0e0` in its companion.

## 5. Building-placement validation

`CanBuildAt` initializes placement-height and metal-sum output globals, rejects footprints that violate its map-border rules, and optionally applies observer sight/exploration checks. A null observer argument takes a different route from a player-visible placement query; callers must choose the intended contract.

It then scans footprint cells and uses the yard mask to choose checks:

| Mask bit | Role in this scan |
| --- | --- |
| `0x01` | Reject an existing blocking cell when the visibility-conditioned test applies |
| `0x02` or `0x04` | Reject a foreign primary-slot occupant under that condition |
| `0x08` | Accumulate support-height minimum and maximum |
| `0x10` | Accumulate a separate maximum height constraint |
| `0x20` | Reject a blocking feature |
| `0x40` | Reject the referenced feature property at its high flag byte |
| `0x80` | Require a qualifying referenced feature property somewhere among these cells |

The high feature-property tests are described by their actual location and role here; assigning a stock gameplay label requires the feature loader and content evidence. Every visited cell also contributes its metal byte to the placement accumulator.

If support samples exist, their maximum-minus-minimum must fit the definition slope limit and the proposed height is their minimum. If the sampled interval is empty, proposed height is sea level minus draft, narrowed through the shown byte result. The separate maximum constraint cannot exceed that height. Seabed bounds must also fit the unit's maximum and minimum water depths.

The routine returns permission and publishes the chosen height on success. Its outputs are not a resource debit, construction order, or created unit.

Evidence: `CanBuildAt`, `0x47d2e0`.

## 6. Yard transitions and placement modes

`IsFootprintClear` checks the cells selected by the proposed open/closed yard-mask bit. Another primary-slot occupant blocks the change, while the unit's own ID is allowed. Bounds are checked before scanning.

`SetYardOpen` changes the runtime state only if that clearance test succeeds. It then marks/reclaims the footprint and refreshes all movement pass maps over the footprint. A COB door animation and a successful navigational yard transition are separate outcomes.

`CanPlaceUnitFootprint` uses its mode argument to select a different contract. At the outer bounds, mode 2 can accept the out-of-map case that other modes reject. An immobile definition delegates to building validation. For mobile definitions, modes other than 1 return success after their preliminary checks; mode 1 scans feature blocking, primary occupancy, depth and slope.

Its second parameter is declared as a pointer in this source view but is only compared through its low 16 bits to a cell owner ID. The body does not dereference it. Callers must therefore be traced as an ID-like contract rather than assuming it reads another definition's footprint. The source records uncertainty about the original type/intent; this writeup preserves the actual operation.

When a mobile cell's slope exceeds the ordinary limit, underwater cells receive the additional water-slope allowance, while above-water cells reject. Definition slope limits, movement-class path costs and construction support-height rules are related but distinct checks.

Evidence: `0x47d970`, `0x47dac0`, `0x47db70`.

## 7. Height selection and footprint alignment

`GetFootprintHeight` scans mask-selected cells and returns the minimum support height, or sea level minus draft when no valid support interval exists. It also computes another height maximum that the function does not consume. This function selects a height; it does not apply all of `CanBuildAt`'s validity conditions. A returned height alone is not proof that construction is legal.

`SnapWorldPosToFootprint` skips definitions with the mobile code set. Otherwise it converts the requested fixed-point position into footprint-relative grid coordinates, reconstructs the aligned world X/Z, and takes Y from the footprint-height helper. A footprint's parity/size affects its center alignment in eight-unit increments relative to the 16-unit terrain grid.

Evidence: height `0x47d820` in its companion; snapping `0x47ddc0`.

## 8. Navigation cell values

The pass-map cell query returns 0 for unusable terrain, 1 for usable terrain outside the preferred slope threshold, and 3 for fully preferred terrain. It resolves feature blocking, checks the primary occupant and associated motion/timestamp state, checks depth, and chooses land or underwater slope limits from the cell's lower terrain bound.

Depth is expressed as terrain limits: minimum terrain height is sea level minus maximum water depth; maximum terrain height is sea level minus minimum water depth. A larger minimum water depth excludes shallow water. Draft affects vertical placement elsewhere and must not be substituted for this navigation limit.

The footprint query scans a whole rectangle, propagating any 0 immediately and downgrading the result to 1 if any cell has merely acceptable slope. The edge-strip query first checks the occupied rectangle. If that is fully preferred, it checks four touching strips, including corners; a nonpreferred strip downgrades the final result to 1 rather than necessarily making the footprint unusable.

This clearance grading explains why a path can be possible but penalized near obstacles or rough terrain. The movement-class cache and pathfinder consume these values; the terrain routine itself does not choose a complete route.

Evidence: `0x47de60`, `0x47dfc0`, `0x47e1f0`; [movement class implementation](../../../src/units/movement_class.cpp).

## 9. Candidate-position probe and pad slots

`CanPlaceFootprintAt` converts an existing unit and candidate world position into a footprint rectangle and rejects out-of-bounds cases. It checks a visibility-mask sample and returns success before detailed terrain checks when that sample lacks the unit owner's visibility bit. That early success prevents interpreting this planning probe as omniscient collision knowledge.

The visibility index uses footprint X in both center-offset terms, including the row term. For a nonsquare footprint this differs from using the footprint's Y extent to locate its center. The source identifies this as a suspected original bug. The actual sample, rather than an idealized geometric center, controls the early-success branch.

When the sample permits detailed checks, the routine rejects blocking features, blocking cell flags, foreign occupants in either slot, disallowed depth and excessive local slope. Aircraft without amphibious permission can have their lower permitted terrain bound raised to sea level.

`IsPadSlotFree` is a different test: it rejects a pad that is itself attached and scans the pad's child list for the requested piece index. It does not validate the aircraft footprint, repair permissions or approach path. Free attachment capacity and legal landing terrain must be established separately.

Evidence: candidate probe `0x47e2d0` in its companion; pad slot `0x47e570`.

## 10. Area, rectangle and range visitors

The footprint-area visitor enumerates nearby spatial buckets and tests rectangle overlap against root units and their attached children. It is suitable for footprint reconciliation because children can intersect the requested cells without independent root membership.

The world-rectangle visitor clamps bucket coordinates, enumerates roots and accepts positions within inclusive X/Z limits. Its test is on root origin, not complete model or footprint overlap. The range visitor enumerates buckets around a point, then checks squared horizontal distance against the squared range. It likewise walks roots in the shown body and leaves additional eligibility filtering to the visitor.

These APIs are not interchangeable. A large model whose origin is outside a query can fail an origin-based query while overlapping a footprint-area query. A range visit is a spatial shortlist, not proof that a candidate is allied, damaged, visible or a valid weapon target.

Evidence: `0x47e5c0`, `0x47e750`, `0x47e890`.

## 11. Sampling reclaimable resource sites

`PickRandomReclaimableResourcesInRadius` samples a square around the supplied center in 48-world-unit increments, using half the supplied fixed-point extent on each side. Despite the function's provisional name, the shown sample loop does not apply a circular distance filter to that square.

It resolves features and requires two feature flag bits before considering their two resource/value fields. It collects nonzero values into separate candidate arrays. For each nonempty array it draws three random candidate indices and returns the highest value among those samples. It is not an exhaustive maximum or nearest-resource search.

Missing candidate classes clear their output pointer/value. The overall result reports whether either class was found, and the temporary arrays are released. Callers must supply writable result storage for nonempty outputs. The algorithm's exact stock interpretation of the two resource classes follows their feature fields; the sampling and selection behavior is established here independently.

Evidence: `0x47ea40`.

## 12. Lifecycle, failure paths and completeness

The module does not own disk persistence or network packet formats. It updates runtime cell/bucket state and navigation caches; unit creation, movement, attachment, death and restore code invoke it. Save restoration must rebuild consistent objects before relying on these lists, and movement must keep cell and world coordinates in agreement.

Frequent diagnostic mistakes include treating a geometry-derived height as placement permission, treating a free pad piece as a complete landing check, treating every spatial query as a footprint-overlap test, and assuming every mode performs the same collision scan. The explicit branches above resolve those distinctions.

All annotated entry points are covered by the sections above, including the empty support routine and visitor callback. Original intent behind the pointer-shaped owner-ID argument and exact asset labels for unnamed feature bits remain explicitly unresolved; neither is needed to describe the confirmed runtime operations.
