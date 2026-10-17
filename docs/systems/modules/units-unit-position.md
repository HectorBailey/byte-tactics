# Unit positioning, attachment, tick execution and state replication

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `units/unit_position`, from `0x48a7f0` through the boundary before `0x48bae0`. Sources are [unit_position.cpp](../../../src/units/unit_position.cpp), containing twelve annotated functions, and SetStateBits in [unit.cpp](../../../src/units/unit.cpp), `0x48b090`. This module connects world coordinates, map occupancy, height/alignment, attachment, unit tick execution and periodic network snapshots.

## 1. Position, footprint origin and movement mode

A unit keeps fixed-point world X/Y/Z separately from its footprint-origin cell coordinates. The conversion subtracts half the footprint in world units, adds half a cell for alignment and divides by the 16-world-unit cell scale. The same conversion must be used when comparing old and new occupancy.

`SetUnitPosition`, `0x48a9f0`, accepts a position and low-two-bit mode. If the derived origin and mode match current state, it only copies world position. Otherwise it removes old occupancy, copies position/origin, updates mode, reinserts occupancy and refreshes sight. Both paths set runtime bit 0x10000 to request height-related work and return the resulting flags.

This distinction avoids rebuilding a footprint for every subcell move. It does not mean a subcell move leaves all presentation state unchanged, because world position still moves and the height-dirty flag is set. The function does not validate a complete route or construction placement before changing position.

`GetHeadingBetween`, `0x48a980`, derives heading from horizontal coordinate differences through the angle helper. `GetHeadingBetweenOrDefault`, `0x48a9b0`, returns the caller's supplied heading when both horizontal deltas are zero. Y does not affect either heading result.

## 2. Height selection and alignment

`SnapUnitToGround`, `0x48a7f0`, sets Y to bilinear ground height, or at least sea level minus draft for definitions with canhover, bit 12. Its max-based hover expression can call the ground sampler twice. It is an immediate height assignment rather than a general movement update. The private field called floats means canhover here, as established by the FBI loader.

`UpdateUnitHeight`, `0x48a870`, enters when height-dirty bit 0x10000 is set or the definition has canhover. It clears that bit first. Further assignment requires motion and mode 1. Within that branch, upright (definition bit 20) selects direct ground sampling, with hover definitions clamped to sea level minus draft. Otherwise floater (bit 19) selects a fixed water-relative Y. Its integer expression uses draft times 0xffff plus sea level before shifting to fixed point, whose low-word result corresponds to sea level minus draft.

Without those direct branches it calls [four-point ground/water alignment](units-unit-targets.md). That can set height, pitch and bank from model support points. No motion or a different mode can therefore consume the dirty flag without performing these same Y assignments. Aircraft flight and attached-unit positioning require their own motion/script logic.

Draft, permitted water depth, terrain slope and floating presentation are separate inputs. Setting a model's Y above the seabed does not establish a legal path or a free footprint.

## 3. Attachment request and packet

`AttachUnitToPiece`, `0x48aac0`, requires a live child without the building/immobile-associated runtime bit 0x20000000 and without cargo of its own. A nonnull target must be live, differ from the child and have no carrier. A null target requests detachment. This excludes the shown forms of nested transport while allowing a root target to hold a child chain.

It broadcasts and locally applies a seven-byte type-10 record: child ID, optional target ID, piece byte and low-mode byte. The child itself must be valid before the initial predicate, despite the later optional-ID formatting branch. This primitive does not check transport capacity, alliance, pickup range or a free pad piece; callers establish those policies.

`ApplyAttachUnit`, `0x48ab70`, resolves IDs and repeats the important child/target guards. It removes the child from its existing carrier's linked child list or from its root spatial bucket. It then stores the requested transport piece and either prepends it to the new carrier's child chain or restores it as an independent spatial root.

Piece value minus one sets runtime bit 17 on attachment; detachment clears that bit and sibling/carrier pointers. The mode byte is copied into the child's motion object's two-bit state. An active local type-1/type-2 child attached to a target lacking definition bit 9 gets a BECARRIED order. Selection eligibility is refreshed afterward.

The body assumes consistent old list membership, a valid spatial bucket where used and the appropriate motion object. It does not safely repair an arbitrary corrupt attachment graph. Root spatial membership and parent linkage must agree before and after the operation, which is why the [terrain visitor](map-terrain.md) treats attached children separately from bucket roots.

## 4. Activation/state changes and their side effects

`Unit::SetStateBits`, `0x48b090`, sets or clears the requested bits in the activation byte, then compares old/new values. No change means no transition callbacks or packet from this method.

| Changed bit | Side effects |
| --- | --- |
| Bit 0 gained/lost | Starts Activate/Deactivate and queues the corresponding speech |
| Bit 3 gained/lost | Starts StartBuilding/StopBuilding |
| Bit 2 gained | Queues speech and marks linked order/listener objects with status flag 0x10000 |
| Bit 2 lost | Queues the opposite speech event |

Bit 2 also participates in foreign-unit visibility suppression in the [player visibility helper](game-players.md). Provisional comments calling every bit a generic working state should not replace that concrete use. The link notification in this body happens on gaining bit 2, not losing it.

Any changed activation byte marks selection orders dirty. Active type-1/type-2 owners broadcast a four-byte type-17 event containing unit ID and the new byte. Received snapshot restoration applies a set mask and complementary clear mask through this same interface, so the transition machinery, not just a raw byte copy, reconstructs the state.

## 5. Per-unit execution order

`UpdateAllUnits`, `0x48ad30`, clears the active-unit tick count and visits qualifying active controller roles 1/2/3 with a valid slot index. For every nonzero-definition slot it increments that count and executes this sequence:

1. Update wind-generator callbacks.
2. Update weapons for active local type-1/type-2 ownership.
3. Run existing COB scripts with one tick of execution input.
4. Decrement recent-damage and ownership-transfer timers when nonzero.
5. Clear a selection/action flag when construction, transfer holdoff, readiness or carrier permission makes it ineligible.
6. Every 30 ticks, retain previous health percentage and calculate the current clamped percentage.
7. For active local ownership, process water damage and self-repair, primary/secondary orders, then motion and height.
8. Apply pending death when runtime bit 14 is set, using the stored damage kind.

Remote units still participate in shared presentation/script/timer work, but local authoritative weapon/order/motion branches are guarded by owner role. The separate replicated-motion reader updates remote movement state as described below.

## 6. Hazards, self-repair and camera follow

Water damage requires enabled mission water damage and a nonzero amount, the 30-tick boundary, unit whole Y at or below sea level and absence of canhover. Floater alone is not the exemption tested here. It calls DamageUnit with kind 11. That kind follows the damage helper's special reaction/broadcast exclusions; touching water is not simulated by a generic particle contact.

Self-repair requires a nonzero definition rate, health below maximum and the tick's low three bits zero. It passes integer `(rate times 8) divided by 30` as float repair progress. Integer truncation happens before that conversion; very small rates need not produce a positive increment in this branch. The repair-progress helper determines health/account effects.

After a player's range, network flag 1 enables SendUnitStates for active local controller roles. After all players, automatic camera-follow mode decrements its timer unless key 0xf9 is down. Expiry resets to 90, cycles selection and cycles camera follow. This is distinct from unit locomotion and does not itself issue a move order.

## 7. Full unit snapshot format

`WriteUnitState`, `0x48b200`, writes a type index using the configured unit-definition bit width. Definition zero ends that record, expressing an empty slot. Nonempty records then write:

| Field | Bits |
| --- | ---: |
| Health | 16 |
| Quantized remaining construction fraction | 8 |
| Activation byte | 8 |
| Low movement mode | 2 |
| Attached flag | 1 |

For an attached unit, the remainder is carrier ID in 15 bits and piece in 8 bits. It omits independent world position/rotation/speed in that branch. For an unattached unit, it writes X/Y/Z as three 32-bit values, three 16-bit angles and, when motion exists, 32-bit speed.

Construction zero is encoded as zero. Other values use 1 minus the truncated product of buildLeft and minus 254, narrowed to eight bits. The reader multiplies the byte by the stored build-percent scale constant, approximately 1/255 as recorded in [global data](../../../data/globals.csv). This is a quantized representation, not bit-exact persistence of the original float. [Save records](units-unit-save.md) retain construction bits through a different format.

## 8. Reading full snapshots and updating occupancy

`ReadUnitState`, `0x48b3f0`, reads the definition first. Zero marks an occupied local slot pending death and returns. A differing nonzero type reconstructs the slot through replicated creation using the current position and ID. It then invalidates the cached object picture/frame field, restores health and quantized construction, and marks construction-display dirty bit 13 when the fraction changes.

It restores activation through SetStateBits and reads mode plus attachment state. Attached records invoke ApplyAttachUnit and return. An unattached record first detaches a currently carried unit, then reads position/angles. When origin cell or mode differs, it removes old occupancy, assigns the new values, reinserts and refreshes sight; otherwise it only copies position. It sets height-dirty bit 16, copies angles and restores speed when motion exists.

The distinction between completed construction and zero type is important: zero type means absent unit, whereas zero construction byte on a nonzero type means completed construction. The motion-presence-dependent speed field also requires sender and receiver to agree on the definition's runtime components.

## 9. Player packet with motion updates and a rotating full slot

`SendUnitStates`, `0x48b710`, builds a type-44 bitstream containing packet length and current tick, storing that tick as the player's sync timestamp. It visits live units with motion whose motion serialization interface currently qualifies them. For each, it writes the 16-bit slot index relative to the player's first ID, definition index and interface-defined motion state.

It stops that loop after the current write brings encoded length to at least 0x200 bytes, so this is a soft threshold rather than an absolute packet-size ceiling. It writes the 0xffff end marker, a set presence bit and one full-unit snapshot selected by gameTick modulo maxUnits. That selection rotates across capacity slots, including unused slots, not just currently live units. Empty snapshots eventually communicate removal as well as live-state refresh.

It patches the little-endian length into bytes 1/2, broadcasts using the player identity and releases the temporary stream buffer. The full slot and terminator can increase final size beyond the motion-loop threshold.

`ReceiveUnitStates`, `0x48b920`, consumes header/length/tick, updates syncTick and returns early when the player has no unit range. Otherwise it reads relative indices until 0xffff. Type mismatch triggers replicated creation, then the unit's motion interface reads its payload. It next updates motion/height for every live moving unit in the player's range, and, when the final presence bit is set, reads the rotating full slot using the packet tick modulo maxUnits.

The receiver consumes the advertised length but does not use it here for full bounds validation, nor independently validate every relative index, type or nonzero-capacity assumption. Packet validation and bitstream storage limits belong to the surrounding network pipeline. The motion list is interface-defined state, while the rotating full slot carries the separate health/construction/activation/attachment correction.

## 10. Completeness and diagnostic boundaries

All 13 annotated entry points in the module range are covered, including SetStateBits retained in the shared unit file. The module defines live state events and snapshots, not HapiBank unit save records. Navigation/pathfinding chooses positions, attachment/order handlers choose actions and COB defines animation responses; this module keeps those results coherent with occupancy, execution authority and replicated state.

For a ship that appears at the wrong vertical level, inspect mode, motion, height-dirty state, floating/on-water flags, draft and model alignment. For carried units that disappear from area queries, inspect root versus child membership. For construction visuals that lag remote state, inspect quantization, rotating-slot selection, construction dirty flags and picture invalidation. For a unit that has zero health but still exists, inspect pending-death processing after the tick's earlier work rather than assuming damage immediately freed the slot.
