# Steering, motion state, script geometry and command cursors

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `units/unit_scripts`, `0x43cd20` through the boundary before `0x43f0e0`. Its implementation is split between the motion portion of [order_list.cpp](../../../src/orders/order_list.cpp) and [unit_scripts.cpp](../../../src/units/unit_scripts.cpp). It owns steering, motion component lifetime/update/save, occupancy-script state, model-piece queries and cursor/action hints. The COB virtual machine and script-call interface are separate modules.

## 1. Motion component and behavior choice

`UnitMotion::UnitMotion`, `0x43dc00`, clears velocity, inertial bank/pitch vector, speed, turn and path-lock timestamp, starts mode 1 with obstruction flag clear, and retains the definition's movement-class pointer/value.

It allocates one of four behavior objects according to owner role and canfly. Active remote type-3 ownership chooses PackedPosGoal for aircraft and PatrolGoal for nonair units. Other ownership chooses LiteGoal for aircraft and AiSearchGoal for nonair units. These provisional class names denote different motion interfaces; they do not mean all PatrolGoal users are executing an explicit user patrol command.

`DestroyObject`, `0x43dd10`, invokes the behavior's deleting virtual operation when present. Unit lifecycle later releases the containing motion allocation. `UpdateMotion`, `0x43dd20`, updates behavior first, selects aircraft or ground steering, updates position, updates movement-rate callbacks, then recomputes SFX occupancy. This ordering makes script-facing movement state a consequence of the latest physical step.

The [unit tick](units-unit-position.md) runs local authoritative motion after orders; replicated unit-state processing invokes motion update for qualifying remote movers through its own path. These services are connected rather than two independent simulations of the same local command.

## 2. Ground steering and braking

`SteerGroundUnit`, `0x43cd20`, asks whether the behavior currently has an active path/goal. Without one it clears turn and reduces speed by definition brakerate through the shared velocity helper.

With a goal it requests three path points. When the second point is more than 80 world units away, it can pull that steering point backward along the first-to-second segment, limiting look-ahead adjustment by segment length. It computes horizontal distances, desired heading and signed angular difference, then limits actual turn to definition turnrate and marks movement when heading changes.

Acceleration versus braking uses two estimates: distance needed for the existing heading difference at current speed, and squared stopping distance from speed/brakerate. It accelerates only when both path-distance comparisons leave sufficient room; otherwise it brakes. This is not instant orientation followed by constant-speed translation. Large turning error or an approaching endpoint can slow a ship before reaching the visible marker.

The [adjacent velocity helper](orders-order-list.md) applies pitch-based maximum speed and the underwater cap before converting heading into X/Z velocity. Steering assumes usable turn/brake parameters for its divisions; zero or malformed definition values are not independently repaired here. Route generation and obstacle-search costs belong to pathfinding.

## 3. Aircraft steering and flight mode

`SetFlightMode`, `0x43d210`, changes the motion mode only when different. Entering mode 1 clears speed/velocity, updates bank/pitch with a zero input and clears activation bit 0. Entering other modes sets that bit. This can invoke Activate/Deactivate through the common state transition interface.

`SteerAircraft`, `0x43d290`, moves actively only in mode 2; other modes clear velocity/speed/turn. In mode 2 the behavior supplies target position, target velocity and desired heading. The routine damps velocity using acceleration relative to maxvelocity, then applies a horizontal magnitude limit from the field loaded as brakerate. Excess motion contributes a heading-directed correction. Field names do not imply that every aircraft formula uses the same ground braking interpretation.

Outside the overflow spatial bucket, vertical velocity approaches target altitude with a limit of one world unit or one quarter of current speed, whichever the branch selects. It clamps turn to definition turnrate. Horizontal steering compares target displacement and target-relative velocity, with acceleration constrained to the definition's acceleration magnitude and a distance floor of eight world units in its coefficient calculation.

It adds the resulting X/Z acceleration, stores full velocity length as speed, and passes the velocity change into bank/pitch. The behavior's chosen position/velocity determine attack-run, landing or travel objectives; this function supplies the physical response rather than independently choosing an enemy or repair pad.

## 4. Bank and pitch inertia

`ApplyBankAndPitch`, `0x43d0d0`, retains approximately 0.95 of its previous inertial vector, adds the new motion delta, rotates horizontal components by heading and scales them by definition bankscale/pitchscale. An angle helper compares those values against a gravity-derived denominator.

Both final angle calculations use the rotated X component. The source identifies the pitch calculation's reuse of X, rather than the rotated Z, as a suspected original bug. This observed relationship should not be replaced with a conventional aircraft-physics explanation that the body does not implement.

## 5. Position updates and collision boundaries

`UpdatePosition`, `0x43d6d0`, handles carried units first. It queries the carrier's model-piece world position, clamps floater height to the sea-minus-draft floor, sets position/mode, obtains piece angles and inherits carrier motion velocity/speed when available. A stationary carrier clears inherited motion. The child does not independently follow its own ground path through this branch.

For an uncarried unit it adds velocity to position and returns if position and mode are unchanged. Otherwise it stamps lastMoveTick and derives the new footprint origin. A same-origin/same-mode move updates world position directly and marks moved. Crossing a cell or changing mode performs additional placement handling.

For active local controller roles 1/2 it refreshes the motion obstruction flag from CanPlaceUnitFootprint. If blocked, it clamps X/Z within roughly half a terrain cell of the current footprint center and caps speed to half definition maximum, rebuilding horizontal velocity when needed. It leaves the old footprint occupancy rather than forcing insertion into the rejected new cells.

If not blocked, it removes old occupancy, writes position/origin/mode, reinserts occupancy and refreshes sight. Other owner roles do not repeat the same local placement test in this branch; the stored obstruction state and replicated behavior matter. This is not a complete model-triangle collision test, and a pathfinder's preferred corridor does not remove the live placement check.

## 6. Movement script callbacks

`UpdateMoveRate`, `0x43da70`, selects movement rate zero when blocked, carried or without speed/turn. Otherwise it chooses rate 1 up to moverate1, rate 2 above that through moverate2, and rate 3 above both. It compares this with the unit's stored two-bit movement-rate state.

On a change to zero it starts StopMoving. Changing from zero to a nonzero rate starts StartMoving. Rates 1/2/3 invoke MoveRate1/2/3 respectively, then the two runtime bits update. A unit turning in place can meet the speed-or-turn condition, so a moving animation is not proof of nonzero translation.

COB assets decide whether these events spin tracks, propellers, legs or rotors and how quickly the animation responds. The callback names and thresholds are confirmed here; no universal particle emission is implicit in StartMoving itself.

## 7. SFX occupancy and water states

`UpdateSfxOccupy`, `0x43db50`, derives the integer passed to setSFXoccupy from runtime mode, whole Y, sea level, draft and model top:

| Condition | Assigned state |
| --- | ---: |
| Mode other than 1 or 2 | 0 |
| Y above sea level | 4 |
| Y at/below sea, less than five units below it | 1 |
| Y plus draft equals sea level | 2 |
| Y plus model top below sea level | 3 |

The last three are sequential tests, so a later matching state overrides an earlier one. When below water and none matches, the previous state remains. The helper starts setSFXoccupy only when the result changes, then stores it.

This state informs script-selected effects such as water/ground behavior, rather than directly declaring every unit with state 2 a ship or emitting a wake here. Model top and origin differ, and exact equality with draft differs from the broad submerged test.

## 8. Motion persistence

`SaveMotion`, `0x43dd70`, writes 0x23 bytes under `u%04xmob`, keyed by unit ID. It stores velocity, inertial vector, speed, turn, path-lock timestamp, mode and obstruction flag. `LoadMotion`, `0x43de30`, reads and assigns the same fields.

The load body does not check a complete-read result before copying the local record into the motion object. It assumes a valid named box and compatible unit definition/component. Behavior objects and entire route-search internals are not simply raw pointers copied from that record; their lifetime comes from construction and associated goal/order reconstruction.

## 9. Piece offsets and world positions

`GetPieceOffset`, `0x43def0`, returns zero for missing object/state or out-of-range index. Otherwise it seeds the result from model-piece base offset plus animated translation, then walks its parent chain. Each parent rotates the accumulated offset by its piece angles, incorporates the unit's base angles at the root and adds its translation. The final Z is negated to match the engine's coordinate convention.

`GetPiecePosition`, `0x43e060`, adds the unit world position to that offset. A zero offset from an invalid index consequently becomes the unit origin through this helper, rather than a separately reported failure coordinate.

`GetPieceCenter`, `0x43e0b0`, reads the selected piece's vertex array, builds component minima/maxima and returns their midpoint plus unit world position. Its bounds start at zero, so a piece with every vertex positive on one axis still includes zero in that axis's bounding range. It does not locally validate the piece index or all geometry pointers. `GetPieceAngles`, `0x43e180`, combines unit angles with the selected piece-state fields, using the record layout's corresponding next-record word for bank.

These functions answer different geometric questions: origin/offset, vertex-bound center and angle state are not interchangeable. Weapon muzzle, aim origin, target sweet spot and nano emitter can therefore be distinct positions on the same visible unit.

## 10. Script-selected weapons, aim and nano pieces

`QueryWeaponPiece`, `0x43e1e0`, asks QueryPrimary, QuerySecondary or QueryTertiary with a default result of piece zero. `GetWeaponPiecePosition`, `0x43e240`, uses an explicitly supplied nonnegative piece or obtains that query result, then returns its world position.

`GetAimFromPosition`, `0x43e2e0`, asks AimFromPrimary/Secondary/Tertiary, initially minus one. A retained minus one falls back to the corresponding Query result; otherwise the explicit aim piece is used. Thus missing AimFrom can still yield a usable firing reference through Query without making all muzzle and aiming origins identical.

`GetSweetSpot`, `0x43e3c0`, queries SweetSpot with default zero and passes the piece to GetPieceCenter. `GetNanoPiecePosition`, `0x43e400`, queries QueryNanoPiece and uses its world offset. Nano/reverse-nano particle code receives that emitter position later; this query itself does not create particles.

Weapon indices are expected within 0..2 for the three-name arrays, and these query paths expect the appropriate script interface. COB return values and model-piece order determine named units' exact muzzle/effect placement.

## 11. Cursor classification is a separate service

`OrderModeTakesTarget`, `0x43e470`, returns false for modes 5, 10 and 14, true otherwise. `GetOrderCursor`, `0x43e490`, returns numeric hint codes, with 0x13 as the general unavailable/default result. It shares capabilities and interaction tests with [command resolution](orders-order-dispatch.md) but does not create a MissionType or insert an order.

Attack mode can return code 2 for a dropped primary weapon, code 1 for an attack-capable mover, or code 1/3 from stationary weapon reachability. A point attack also rejects the primary anti-air-only case. Special-attack cursor checks current energy/metal against per-shot costs, unlike merely selecting ATTACKSPECIAL in the dispatcher.

Patrol uses 7, repair 6, follow 5, capture 4, air pickup 8, ground pickup 0xc, unload 0xd, mobile build 0x10 and teleport 9 under their corresponding predicates. Ordinary move uses 0xe. Resurrection and reclaim can use 0xa/0xb. These numbers are returned interface codes; which bitmap/glyph depicts them belongs to cursor resources and drawing.

Move-context cursor priority tests visible resurrectable features before some target interactions, then capture/reclaim/service/landing/pickup/follow. Default mode with interfaceType 1 favors selectable-local code 0xf, then enemy/friendly codes 0x11/0x12 and service-feature hints. The other variant redirects enemy attack/reclaim, considers unfinished repair, local selection, features and movement.

Local selectability requires matching local player index, ready/selectable flag, zero buildLeft, no transfer holdoff and appropriate carrier permission. Some cursor feature macros assume a nonnull position before visibility evaluation. The cursor does not apply the dispatcher's initial nonlive-target guard identically. These differences make the hint related to, but not definitive proof of, the eventual action and its success.

## 12. Completeness and cross-system use

The module's steering, component/update/save, SFX state, geometry queries and cursor helpers are covered. It defines no standalone packet transport; motion behavior interfaces and unit replication consume/provide its state. Script bytecode execution and piece mutation belong to COB and unit_script_calls, while route/goal geometry belongs to order_targets/pathfinding.

For ship motion that stops against a valid-looking route, separate steering speed, live footprint checks and blocked-cell clamping. For a wake or muzzle effect at the wrong point, trace setSFXoccupy, COB query output, piece hierarchy and world conversion before the particle renderer. For a cursor that suggests an action which does not execute, compare the hint's gates with GetOrderType and the chosen order handler.
