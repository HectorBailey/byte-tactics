# Terrain features, wrecks, reclaiming and fire

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `map/features`, from `0x421e60` through `0x4256a0`. The main implementation is [features.cpp](../../../src/map/features.cpp), with retained companions for [pool initialization](../../../src/map/features_421f20.cpp), [definition loading](../../../src/map/features_4224b0.cpp), [replacement links](../../../src/map/features_422ea0.cpp) and [save restoration](../../../src/map/features_424c00.cpp).

Features are map objects such as trees, rocks, geothermal sites and unit wrecks. The module provides definitions, footprint records, optional instance state, replacement transitions, damage, reclaim payouts, fire, reproduction and persistence. A wreck can retain a moving 3D position while its blocking footprint remains a terrain-cell object. It is not a live unit with orders, weapons and an ordinary unit ID.

## 1. Definitions, cell origins and stateful spots

The system separates three layers:

| Layer | Contents | Lifetime |
| --- | --- | --- |
| Feature definition | Name, footprint, model or GAF sequences, resource values, damage threshold, flags, replacement IDs | Shared by all instances of that type |
| Terrain footprint | Origin feature ID plus offset markers in other occupied cells | Exists while the feature occupies the map |
| Feature spot | Model state, position/velocity or transition animation, damage, rotation, cell position and transient flags | Allocated only when per-instance state is needed |

A terrain origin contains a normal feature index below 0xfffb. Nonorigin footprint cells carry 0xfffe and byte offsets back to the origin. Empty is 0xffff. Some other high sentinel values are reserved rather than normal definition indices; placement handles 0xfffc specially by writing that marker directly.

`GetCellFeature`, `0x421e60`, resolves a 0xfffe continuation cell to its origin ID and returns empty for other high sentinels. The private lookup used by reclaiming additionally rejects missing cells and some invalid indices. Not every entry point performs the same normalization or bounds checks, so callers must supply the cell form required by their operation.

Cell flag bit 0 identifies associated spot state. The adjacent two-byte cell field is interpreted either as a spot index, static damage accumulation, or footprint-origin offsets according to the cell's role. Reading it without the feature/flag context produces misleading results.

The feature definition is 0x100 bytes. Provisional views disagree on resource-field names: the loader and reclaim body establish energy at +0xec and metal at +0xf0. The shared header uses different placeholder labels. This document follows the actual field readers/writers rather than assuming all declarations have settled names.

## 2. Discovering and loading definitions

`LoadFeatureFileList`, `0x4222e0`, recursively discovers `features\*.tdf`, creates parsers and retains successfully loaded files. Failed loads discard their parser. `FindFeatureFile`, `0x422460`, resets each parser and returns the first one with a record matching the requested feature. Duplicate names therefore depend on file enumeration order. `FreeFeatureFileList`, `0x4223e0`, deletes the retained parsers and their vector.

`FindFeatureType`, `0x422dd0`, searches already loaded definitions case-insensitively and returns minus one when absent. `FindOrLoadFeatureType`, `0x422e40`, uses that lookup and otherwise appends a definition. `LoadFeatureType`, `0x4224b0`, treats a missing TDF record as fatal. It reallocates the shared definition array and returns the previous count as the new index. A pointer into the old array must not be retained across a load that grows it.

Definition fields include footprint X/Z, height, description, damage threshold, energy and metal, spread/reproduction parameters and burn timing. Resource integers are converted through unsigned 16-bit values before storage as floats. `sparktime` is converted from seconds to 30 Hz ticks and stored in a short field.

The flags are loaded separately: animating, animation/shadow transparency, flammability, geothermal, blocking, reclaimable, autoreclaimable, indestructible, no-display-info and no-draw-under-gray. Autoreclaimable defaults to 1; the other listed flags default to zero. Four explicitly named fortification/dragon's-teeth definitions force no-draw-under-gray. These flags describe different consumers' permissions, rather than one universal feature-active state.

## 3. Model and GAF presentation resources

An `object` field selects a 3D model and clears the no-object flag. Without it, the definition selects a GAF file under anims, normal and shadow sequences, and optional burn, death and reclaim sequences with shadows. An empty optional sequence name returns null through `FindOptionalGafEntry`, `0x4222b0`.

Burn, death and reclaim sequences have their looping kind cleared so they can finish. Normal animated definitions initialize shared normal/shadow sequence references. These shared references advance once per feature type, giving ordinary instances of that type shared animation progress. A burning or dying instance instead uses a spot with its own transition reference.

The GAF loader searches earlier feature entries for a matching stored filename. On reuse, the new definition points its sequence lookups into the earlier GAF but keeps its owning GAF pointer null and marks its filename as REUSE. Cleanup frees the owning allocation once. The exact 16-byte filename comparison/storage is an implementation constraint, not a general asset-identity registry.

`ResolveFeatureLinks`, `0x422ea0`, resolves `featuredead`, `featureburnt` and `featurereclamate` to indices, using 0xffff for absent links. Resolving a link can load another definition. The loop rereads the growing count and table, so newly appended definitions also receive link resolution. This establishes replacement chains without recursively traversing them during every runtime transition.

## 4. Spot pool and teardown

`InitFeatureAnimPool`, `0x421f20`, allocates 0x800 spots of 0x30 bytes each, links them into a free list, and sets the active/rest heads to minus one. It also allocates a 0x118-byte feature-unit helper and initializes its flags/definition reference. The map's initial feature-name list is loaded into definitions afterward.

`AllocFeatureSpot`, `0x4232a0`, removes the free head, moves it to the active list and clears its used flag. Exhaustion returns 0x800, which is outside the valid slot range. Callers check this sentinel. `MoveFeatureSpot`, `0x4232f0`, reconnects the old doubly linked membership and inserts at the requested list head. Traversals follow the stored previous link from each head; update code saves its next traversal index before operations that move or release the current slot.

`FreeFeaturePool`, `0x422170`, walks active/rest slots and frees model object states for 3D features, frees the helper and spot storage, releases owning GAF/model allocations, then clears the definition array and count. Parser discovery has its own cleanup operation. Neither definition cleanup nor pool teardown is interchangeable with removing a single footprint from the map.

## 5. Placement and map interaction

`GetFootprintCentre`, `0x421eb0`, converts a footprint origin and dimensions to a fixed-point world center and samples ground height for Y. Each terrain cell spans 16 world units; center placement uses half-footprint offsets.

`PlaceMissionFeatures`, `0x423160`, skips empty mission feature names, finds or loads each type, and locates a cell. Sprite features use the mission coordinates directly; model features subtract half their footprint to obtain the origin. It places with owner value 10. This field is stored on the origin; it does not give the feature live player-controlled unit behavior.

`PlaceFeature`, `0x423c50`, ignores 0xffff, handles the special 0xfffc marker, then checks that the normal footprint fits the map's right and bottom limits. Its ordinary path expects a valid supplied cell. It walks all cells in the new footprint and tries to remove existing features. If an old feature refuses removal, placement returns null. Earlier removals are not rolled back. A subsequent spot-allocation failure can likewise occur after old features were removed.

A model requires a spot. It records type, zero damage, origin, optional supplied world position/rotation or generated defaults, and a newly created model state. The origin gets its type, spot index and state flag. A normal sprite instead stores its type, clears the state flag and starts its static damage field at zero. Other footprint cells receive continuation markers and offsets. Geothermal placement starts timed subparticles of index 4 at the supplied or computed position. Placement refreshes affected movement pass maps.

The return value is the model spot, or null for ordinary sprite placement as well as failures. Null is consequently not a universal placement-failed result. Callers must understand the selected representation.

`StampFeatureMetal`, `0x422040`, scans direct feature origins. An indestructible definition with a nonzero +0xf0 value writes that value, narrowed to a cell byte, over its footprint's metal field. This establishes a terrain-metal contribution from selected map features. It is separate from a later reclaim payout.

## 6. Removal and replacement

`RemoveFeature`, `0x4246b0`, normalizes continuation cells to their origin and rejects reserved/empty IDs. Unless forced, it refuses indestructible definitions. For stateful instances it releases model state where applicable and moves the spot to the free list. It clears the origin and continuation cells, then refreshes pass maps. `RemoveAllFeatures`, `0x424840`, scans the map using forced removal, including origins and continuation markers.

`KillFeature`, `0x423550`, normalizes an offset cell, then chooses a death or reclaim transition sequence for sprites according to its flag argument. If the selected sequence exists, it needs an unused origin and a free spot. It initializes transition/shadow state and remembers the reclaim distinction. An already stateful sprite or exhausted pool returns without beginning that transition. Without a sequence it directly calls the replacement operation.

`ReplaceFeatureWithDead`, `0x423710`, selects the dead or reclaimed replacement ID. A spot's reclaimed bit can override the supplied mode. For stateful instances it preserves position/rotation through removal and replacement; otherwise it uses ordinary placement defaults. `ReplaceFeatureWithBurnt`, `0x423bf0`, removes the origin and, if present, places the burnt-linked definition. The update loop expands the same burnt-replacement logic inline.

Replacement is implemented as removal followed by placement, with the same removal/placement restrictions described above. It is not a transactional swap that guarantees a new instance regardless of allocation or indestructibility. Definition links determine whether a destroyed wreck becomes a smaller wreck, disappears, or changes to another feature.

## 7. Reclaim payout and order interaction

`ReclaimFeature`, `0x4237d0`, resolves the feature at a fixed-point X/Z position. Missing features fail. A stateful sprite also fails, preventing payout during an active sprite transition. Otherwise it adds the definition's energy and metal to the reclaiming unit's resource accumulators, invokes the reclaim transition, and returns 1.

For an active type-2 player, difficulty 0 scales each payout to 0.5 and difficulty 1 to 0.7; other cases receive the full amount. These are unit accumulators feeding the economy, not an immediate direct update of a displayed global resource counter.

The primitive itself does not test the reclaimable flag, range, build stance or time spent reclaiming. The [ground reclaim order](../../../src/orders/unit_orders_404ad0.cpp) checks reclaimable, approaches the footprint when required, runs the building script, waits for build stance and consumes a resource-dependent work timer. While enough time remains it emits reverse nano particles from the feature box toward the unit's nano piece, twice per work callback. The primitive supplies the final payout/transition.

Payout precedes the transition, and the primitive does not verify that removal or animation allocation succeeded. Its returned 1 means it passed its own guards and issued the transition, not that the original footprint has definitely disappeared. Caller eligibility and pool state therefore matter to the full contract.

In multiplayer the primitive broadcasts a six-byte feature packet using the reclaiming unit's player identifier, subtype 0xff, and cell X/Z. Resource addition is performed locally by this primitive; the feature packet tells peers to apply the corresponding visual/world transition.

## 8. Weapon damage and authority

`DamageFeature`, `0x4244b0`, first requires option bit 8, a direct normal feature ID, and a destructible definition. It does not normalize a continuation cell itself. In multiplayer, a nonhost local participant sends a six-byte feature-damage request to the host using the weapon index/kind as subtype and returns without local authoritative accumulation.

The host, or single-player execution, handles the effect. A flammable, unoccupied sprite-state origin hit by a fire-starting weapon attempts ignition. Otherwise a model spot accumulates damage, provided its stored origin matches the supplied X/Z. Static sprites accumulate damage in the origin cell's two-byte field. Reaching the definition threshold invokes the death transition.

Stateful sprites do not take the static/model accumulation branches. Ordinary subthreshold host damage remains host state rather than producing a broadcast on every hit. A lethal result broadcasts subtype 0xfd. Ignition has its own broadcast path.

The [packet dispatcher](../../../src/network/net_game_453d40.cpp) decodes type 15: 0xfd kills, 0xfe starts burning with resending suppressed, and 0xff reclaims. Other subtype values resolve a weapon definition and call damage processing. These semantics describe the shown sender/receiver pair; connection validation and packet transport are separate network concerns.

## 9. Ignition, smoke and spreading

`StartFeatureBurning`, `0x4233a0`, requires a valid direct origin, a burn sequence, no existing state flag and an available spot. This primitive does not itself require the definition's flammable flag; damage and spread callers perform that policy check. It initializes the burn/shadow sequences, stores origin and type, and chooses a timer from half the configured spark time plus a random amount below that half. The timer is stored in a byte, so long configured values are narrowed.

It plays the treeburn sound and, unless its suppression argument is set, broadcasts subtype 0xfe. A received ignition uses suppression. The suppression bit also prevents that received instance's update from initiating its own spread countdown, avoiding duplicate authoritative spread from each peer.

`UpdateFeatures`, `0x424050`, emits white-smoke index 5 on ticks divisible by three for active burning sprites. Smoke placement uses the current burn frame's dimensions/origin and randomized offsets around the footprint center. It advances burn and optional shadow sequences. When the burn sequence ends, burnt replacement happens. While the sequence still exists, a positive unsuppressed timer is decremented; reaching zero invokes `SpreadFire` once. If animation ends before the timer expires, that spread call is skipped.

`SpreadFire`, `0x4239c0`, tests the seven-by-seven cell square centered on the burning origin, excluding the origin itself. It considers direct, nonstateful, flammable features and compares a random 0..99 draw against the candidate definition's spread chance. It then takes five wind-directed steps in cell coordinates, with each step adding twice the stored wind X/Z components, and attempts ignition at changed sampled coordinates under the same rules. This is a square neighborhood plus discrete downwind samples, not an expanding circular flood.

If the source definition has a burn weapon, spreading also invokes its area-damage effect at the feature center. Burn damage therefore comes from the configured weapon and this event; smoke emission by itself is not damage.

## 10. Normal animation, reproduction and moving wrecks

Each feature update advances the shared normal/shadow references of animated types. Separately it decrements a scan cursor, wrapping below zero to the last terrain index. The wrap branch does not process that cell on the same call. Other calls inspect one cell for reproduction, rather than scanning every tree each tick.

A direct nonstateful feature can reproduce when its seed-chance test succeeds. The target is randomly offset within the seed-spread interval. It requires an existing target cell with no feature, but the unit-occupancy check reads the source cell's primary unit slot. The row computation divides the scan index by map height instead of map width. The source identifies this as a suspected original bug affecting nonsquare maps. These details differ from a general target-clear placement policy.

For active model spots with nonzero velocity, the update adds velocity to position. At or below sampled mean ground height it snaps to ground and zeros velocity. Below sea level but above ground it clears horizontal velocity and applies fixed downward velocity -0x2ccc, approximately -0.175 world units per tick. Above water it subtracts the game's gravity from vertical velocity. A spot already at rest is moved to the resting list.

The [unit corpse creator](../../../src/units/units_485010.cpp), `0x486360`, selects a corpse definition/replacement depth and passes unit position and rotation into feature placement. This supplies the moving wreck's instance state. The feature update changes its world presentation position; the origin footprint remains represented by its stored map cells. Sinking a wreck model does not itself allocate a new live underwater unit.

## 11. Save representation

`SaveFeatures`, `0x424890`, opens the Features account and writes a Feature Type Names box of 128-byte names. It scans direct origins and appends records to three boxes:

| Box | Bytes per record | Stored state |
| --- | ---: | --- |
| Normal Features | 8 | Cell X/Z, type index, static damage/spot field |
| Animating Features | 10 | Cell X/Z, type index, damage, frame byte, transition selector and timer high nibble |
| 3D Features | 0x1a | Cell X/Z, type index, damage, world position and rotation |

It stores each category's count as a named integer. Stateful sprite records are saved only when their active sequence corresponds to the definition's burn, death or reclaim sequence, encoded as 0, 1 or 2. An unrecognized active sequence is skipped. Continuation cells are reconstructed by placement rather than saved as independent features.

The format does not preserve model velocity, complete animation-reference internals, every timer bit or original feature owner. Restored model position can match the saved position without resuming the identical falling/sinking trajectory.

## 12. Restoration and its limits

`LoadFeatures`, `0x424c00`, reloads feature TDFs and builds a saved-index-to-current-index vector from Feature Type Names. It first accepts a same-index name match, otherwise searches existing definitions or loads the missing type. Without the name box it builds an identity mapping. It resolves replacement links and frees parser discovery afterward.

For each record category it reads counts, seeks by fixed record size and proceeds only on a complete record read. Normal restoration places the feature then restores the cell damage field. Animated restoration places it, invokes burn/death/reclaim according to selector, and restores damage, frame and only the high nibble of the packed timer/selector byte. The ignition call uses the nonsuppressed argument, so the body does not independently suppress the normal ignition side effects during restoration.

Model restoration places with saved position/rotation then restores damage. All these calls use owner value 10. The loader does not locally validate every saved coordinate or remap index, or check that placement/spot allocation succeeded before later state writes. The complete-read guards protect against short records, but do not constitute full validation of arbitrary save content.

## 13. Container support and investigation guide

The remaining annotated helpers at 0x4251e0 through 0x4256a0 implement destruction/insertion for parser-pointer vectors, size/insertion/erase/destruction for unsigned-short remap vectors, and sequential unsigned-short copying. They support definition discovery and saved-index remapping rather than adding another feature simulation state. Deleting parser objects is explicit in the file-list cleanup, separate from destroying pointer-vector storage.

For a missing wreck, trace corpse selection, placement bounds, replacement/removal guards and spot exhaustion. For a feature that pays resources but persists, inspect the order's eligibility checks and the primitive's payout-before-transition sequence. For unexpectedly blocked routes, examine the footprint and pass-map refresh rather than only the model's visible position. For different fire results across peers, separate host damage accumulation, ignition suppression, spread timing and transition packets. For restore differences, compare the saved fields above with transient state that the format omits.

The module's major functional areas and annotated support routines are covered here. Exact named feature visuals, resource quantities, replacement chains and burn damage depend on loaded TDF/GAF/model/weapon assets. Their values are not inferred from a feature's informal name or from stock-game expectations.
