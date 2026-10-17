# Unit storage, creation, death and terrain helpers

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `units/units`, `0x485010` through the boundary before `0x486fd0`. Implementation: [units_485010.cpp](../../../src/units/units_485010.cpp) and the retained [ground-height companion](../../../src/units/units_485070.cpp).

This module allocates live-unit slots, initializes their runtime components, registers normal or replicated creation, produces death events, removes units and creates corpses. It also owns several map-coordinate/height helpers. Definitions, orders, damage calculation, rendering, movement, script execution and save serialization have their own modules, connected through the lifecycle described here.

## 1. Runtime identity and allocation

A unit record occupies 0x118 bytes and contains component pointers, three weapon slots, rotation/position, footprint coordinates and size, spatial/attachment links, definition/player pointers, script/model state, definition index, slot ID, squad index, resource state, attacker metadata, construction fraction, health and runtime flags.

`AllocateUnitMemory`, `0x4854a0`, copies maxUnits into units-per-player and allocates maxUnits times ten plus one slots. The stored pool count is an unsigned short. It zeros the allocation, initializes each slot's ID to its array index and sets its definition pointer to the sentinel definition at index zero. Separate HOT UNITS and HOT RADAR UNITS buffers reserve maxUnits times ten times two and times ten bytes respectively.

Slot zero is reserved: its player index becomes 0xff and its player pointer is null. The ten player records each receive a disjoint inclusive range of maxUnits slots starting after zero. Each range records first/last pointers and IDs, and every contained unit slot receives its persistent player pointer, player index and squad index minus one.

Players are sorted before ranges are assigned. `ComparePlayers`, `0x485940`, compares player network IDs for multiplayer type 3, otherwise player addresses. Unit-ID ranges therefore follow this ordering rather than an unconditional slot-number-times-capacity formula. Network identity and player-array position must not be assumed interchangeable.

The array storage survives individual deaths. Definition index zero marks an unused slot, while runtime flag 0x10000000 guards live lifecycle operations. The ID is a reusable slot index, not a monotonically increasing creation serial. Target/reference cleanup must prevent an old pointer or ID from silently becoming a reference to a later occupant.

## 2. Definition-based initialization

`InitUnitFromType`, `0x485a40`, selects the definition from the unit's definition index and sets live/runtime flags, footprint dimensions, draw-related properties and movement/fire defaults. Its completed argument determines initial construction state:

| Creation state | buildLeft | Health |
| --- | ---: | ---: |
| Completed | 0 | Definition health |
| Under construction | 1 | 0 |

buildLeft is the fraction remaining, so completed fraction is one minus buildLeft. Rendering and economic progress should follow that convention rather than reading a larger value as more completed.

The initializer clears COB low state bits, activation state, work time, damage timers, transfer holdoff, sight caches, kills and attacker metadata. It copies position, derives footprint-origin cell coordinates from world X/Z and half-footprint offsets, initializes rotation, resets all three weapon targets and releases their weapon state, and clears network dirty flags.

It copies standing move/fire settings and the initial-cloak request from the definition, initializes build-menu page state, resets the unit's resource slot with its player index, marks transport piece 0xff, randomizes hover phase and assigns squad 0. The private definition member called canAttack on this path is actually flags1 bit 4, loaded from init_cloaked; the separate command capability comes from flags2. Several other provisional runtime bits are copied without assigning them invented gameplay meanings here.

The helper initializes fields on a reusable slot rather than allocating a new unit record. Proper death cleanup and caller-selected ownership range are consequently prerequisites, not optional follow-up repairs.

## 3. Script, model, weapons and motion components

`InitUnitScript`, `0x485d40`, chooses the unit model by definition index. With COB data, it allocates a UnitScript, binds that data, creates a player/unit-associated object-state tree, links the script to that state, and starts Create. Without COB data it leaves the script pointer null, creates ordinary object state and assigns its owner pointer manually. Both branches clear the state's draw-frame field.

The annotated deleting destructor `0x485e30` supplies UnitScript's polymorphic destruction through the COB base. Its virtual callbacks for model pieces, effects, unit values and attachment are implemented by the separate script-interface module.

`CreateUnitMotion`, `0x485e50`, allocates UnitMotion and initializes heading from the definition's build-angle value. `InitUnit`, `0x485e90`, initializes aim-callback vtables for the three weapon slots, stores the type index, performs definition/script initialization, initializes weapon slots and metal extraction, and creates motion only when the definition's mobile code equals 1. Having a visible model or an attack flag alone does not allocate movement state.

The null guard around the aim-vtable loop is not a valid-null-input contract: subsequent operations dereference the unit unconditionally. The initialization path expects a real slot and successful component allocations. Definition initialization can choose a randomized heading, then motion creation replaces heading for mobile-code-1 units.

## 4. Ordinary creation and its rejection paths

`CreateUnit`, `0x485f50`, accepts player, definition ID, world position, completed flag, runtime mode and optional explicit slot ID. It rejects definition zero and definitions lacking availability bit 0x800000. If a per-type limit differs from minus one, it counts that type in the player's inclusive range and refuses at the limit. Unfinished units count because the scan tests definition index, not construction completion.

Without an explicit ID it selects the first free slot in the player's range. With an ID it requires that slot to belong to the player's range and be unused. No capacity or an occupied explicit slot returns null. The body assumes valid player/type inputs before these checks; it does not validate all indices against independent array bounds.

After initialization, creation assigns the supplied mode, updates height and inserts map occupancy, then sends the new-unit event. For completed immobile units it broadcasts a self builder-link. Definition-dependent activation and one provisional special-state flag are applied for completed creation. The unit is revealed, current-unit and total-created counters increment, and victory conditions receive NotifyUnitCreated.

This routine does not call the complete build-placement validator before creation. Site legality belongs to callers and [terrain placement](map-terrain.md). Successful allocation therefore must not be confused with evidence that a factory or user command was permitted to place that type at the requested location.

## 5. Replicated creation is a different contract

`CreateUnitFromPacket`, `0x4861d0`, uses a 0x11-byte spawn record containing player byte, type ID, slot ID and three fixed-point position integers. Its separate player argument chooses the player record; the body does not use the record's player byte as a replacement for that argument.

It requires the selected player to have a unit range. It derives the global slot from the supplied ID, kills an existing occupant with kill type 0, initializes the requested type as unfinished, updates height, inserts occupancy, reveals it, increments counters and notifies victory conditions. It does not retransmit the ordinary new-unit event here.

This path does not repeat ordinary availability, per-type limit, free-slot search or player-range membership checks. It relies on validated replicated inputs and ownership established elsewhere. An ID of zero produces a null pointer which subsequent live-slot access does not safely accept. These are internal trust requirements of the shown body, not permission to treat arbitrary packets as valid creation requests.

## 6. Producing a death event

`IsUnitCommander`, `0x486460`, compares the unit definition name case-insensitively with the commander name associated with the owner's side. Commander status here comes from that name relationship rather than a generic largest-unit or builder test.

`KillUnit`, `0x4864b0`, ignores records without the live flag. For a commander it clears an owner flag. It then computes script severity and wreckage mode according to the numeric kill type and health:

- Type 7 uses severity zero and initial wreckage mode 1.
- Types 4, 5 and 9, or a unit with positive health, skip the normal lethal-severity query and begin with zero severity/wreckage.
- Other lethal paths compute a percentage-based amount from negative health, maximum health and previous health percentage, clamp it to 1..100, then query Killed through the unit script. The query can provide the wreckage result.

An unfinished unit forces wreckage mode zero. The event stores type 12, unit ID, last attacker network ID, optional attacker unit ID, signed severity byte and four-bit wreckage/kill-type fields, totaling 0xb bytes. Active type-1/type-2 owners broadcast it, then the local path calls ApplyUnitDeath with its local flag set.

If this was a commander and the configured commander-death rule is active, the owner remains active and locally controlled through those roles, the routine also invokes player-wide destruction after local death handling. These conditions are distinct from a mission's general victory-condition evaluation.

## 7. Applying death and detaching dependencies

`ApplyUnitDeath`, `0x4866d0`, locates the slot and ignores a nonlive record. It may create temporary sight at the death position for the viewing player, restores attacker metadata from the event, and notifies victory conditions before destroying the runtime components.

Cleanup then deletes orders, removes queued speech, leaves the squad, removes owned projectile associations, detaches from a carrier, and processes carried children. Cargo receives damage amount 30000 with type 3 when the parent's kill type is 3, otherwise type 6, and is detached. The loop consumes the changing cargo head rather than assuming the original chain survives its callbacks.

It clears map footprint/spatial membership and conditionally removes line of sight when map flag 2 is enabled. For a received event with positive script severity, it starts the Killed script using that severity. The local generating path already performed its query and does not repeat this received-event branch.

This order matters: order callbacks, listeners, attachments, sensor state and spatial references must be resolved while enough unit state still exists. Simply zeroing the slot or freeing its model would leave externally owned links and map occupancy behind.

## 8. Losses, kills and ranking

Death accounting depends on kill type rather than applying identical statistics to every removal. Types 1 and 6 enter ordinary loss accounting. Type 5 enters that accounting only with a distinct known attacker slot. It increments owner losses, credits a known enemy attacker player's kills only for a completed victim, and updates commander losses/kills when the victim's name matches the side commander.

An attacker unit can receive a kill-count increment for a completed enemy victim. Local attacker activity can also inform music selection. Type 3 has a separate loss path gated by the local player's share-vision relationship with the owner. Other numeric removal modes do not automatically execute these loss branches.

Eligible credited attackers in skirmish/multiplayer can move upward in the kill ranking. The comparison uses commander kills when commander-death mode equals 2, otherwise total kills, and skips hidden/inactive-style entries according to the shown fields. Moving to rank zero emits the lead announcement. A UI flag controls kill/loss panel flashing.

These rules explain why removing a unit for cleanup, transfer-like operations, reclaiming or combat need not produce the same scoreboard result. Numeric cases are preserved here where a universal original enum name has not been established.

## 9. Reclaim, explosion, corpse and final slot release

For kill type 5 with an attacker, ApplyUnitDeath credits metal equal to completed fraction times definition metal cost into the attacker's resource production accumulator. Active type-2 computer ownership scales this to 0.5 at difficulty 0 or 0.7 at difficulty 1, otherwise full value. This is live-unit reclamation, separate from the [feature reclaim](map-features.md) resource pair.

A positive script severity on a completed unit triggers DetonateUnitWeapon, with a special argument for kill type 3. A positive wreckage level invokes corpse creation, independently of whether the explosion branch ran. Unfinished units had their wreckage level suppressed while producing the event.

`CreateUnitCorpse`, `0x486360`, begins at the definition's corpse feature and follows its dead link for additional depth levels. Reserved IDs stop the traversal. It obtains the unit's footprint-origin cell and passes world position, rotation and owner index to feature placement. Over ground at or below sea level, a successful model spot receives a downward velocity adjustment unless definition bit 0x1000000 (loaded from isfeature) is set. This test is not the separate floater bit. A successful water placement also suppresses the routine's fallback smoke. Otherwise its supplied flag can cause a timed smoke effect.

After event accounting and effects, ApplyUnitDeath clears all unit-reference listeners, deletes script state, releases model state, destroys and deletes motion, clears definition index and live/selection-related bits, and restores the sentinel definition pointer. It decrements the owner's current-unit count. Reaching zero announces player departure in multiplayer or forces destroyed in skirmish. The record allocation and slot ID remain available for reuse.

## 10. Bulk destruction and memory teardown

`KillUnitsOfType`, `0x486e80`, visits the allocated range and invokes kill type 8 on matching nonzero definitions. `KillAllUnits`, `0x486ed0`, uses type 8 for every occupied record. `FreeUnitMemory`, `0x485980`, similarly kills occupied slots before freeing radar/visible buffers and the pool, clearing their owning pointers. It does not independently clear every player's now-stale range pointer in this body; surrounding lifecycle reset must prevent use after teardown.

`KillPlayerUnits`, `0x486f10`, visits the selected player's range when its count and range exist. It selects live units not already marked with bit 0x4000. Active type-1/type-2 owners receive self-directed damage amount 30000 with type 3. Other roles detonate directly, mark the bit and call KillUnit with type 3. This preserves the distinction between locally driven damage processing and direct cleanup of units controlled through other roles.

## 11. Terrain and coordinate helper contracts

The module's small helpers have intentionally different failure results:

| Function | Input/result | Off-map behavior |
| --- | --- | --- |
| GetCellHeight, 0x485010 | Cell coordinates to stored height byte | Returns 0 |
| GetGroundHeight, 0x485070 | World X/Z to bilinear height from four neighboring samples | Returns -1, including missing right/bottom neighbor |
| GetCellMeanHeight, 0x485140 | World X/Z to average of cell low/high bytes | Returns -1 |
| GetCellPosition, 0x485330 | Cell coordinates to world position at stored height | Leaves output unchanged |
| ClampPositionToMap, 0x4853b0 | Clamps world X/Z to whole-unit map limits | Sets outside values to zero or size minus one, leaves Y alone |

Bilinear height uses the whole world-coordinate words, cell size 16 and integer interpolation with low four bits as within-cell offsets. It does not incorporate the full 16-bit fractional coordinate into its interpolation. Mean height is not interchangeable with this surface interpolation or a construction footprint's support height.

`FindHighestPointOnLine`, `0x4851c0`, chooses a number of samples from the larger horizontal delta divided by 16 world units plus one, then samples including both ends. It compares terrain-plus-feature height and the primary occupant's model top/world Y to a running maximum initialized to zero. It is a sampled clearance estimate, not an exact intersection with every model triangle. The shown feature-height lookup indexes the definition table using the raw cell feature field without normalizing offset/empty markers; its caller/data assumptions must be checked before interpreting it as safe for arbitrary cells.

`ShareMapInfo`, `0x485420`, scans the quarter-resolution visibility array and ORs the recipient's bit into cells containing the source bit. It does not clear existing recipient bits or establish a continuing alliance relationship. Future sight updates and other sensor layers are separate operations.

## 12. Completeness and remaining asset-dependent detail

All annotated functions in this module are covered, including lifecycle support and map helpers. It produces spawn/death events and participates in reconstruction, but the save record format belongs to `units/unit_save`; damage calculation and listeners belong to `units/unit_targets`. COB contents determine actual Create/Killed animation behavior, wreckage responses and piece effects, while definitions determine availability, costs, capacity limits, movement code and corpse chains.

For a disappearing unit, trace the death event through references, attachment, occupancy and counters. For a failed construction spawn, distinguish definition availability, per-type limits, slot ownership and caller placement validation. For a wreck that differs from its live model, inspect the selected feature chain and feature simulation. For a reused ID, require the reference lifecycle rather than assuming the ID uniquely identifies one creation forever.
