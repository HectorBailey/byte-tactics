# Mission conditions, elimination rules and replicated movement goals

Status: finished. The `game/victory` group owns 99 annotated entries in `0x48dfb0..0x490ac0`, including generated deleting destructors. Implementation is [victory.cpp](../../../src/game/victory.cpp), with [CheckVictory](../../../src/game/victory_490230.cpp) separate. Ownership follows the address inventory, so the movement-goal classes and memory-status probe at the end of this interval are included even though their responsibility differs from mission victory.

## Condition storage and evaluation lifecycle

[MissionConditions](../../../src/game/mission_conditions.h) contains sixteen victory pointers, a victory count, sixteen defeat pointers, a defeat count and active. Its constructor is owned by [selection](ingame-selection.md), immediately before this module's address boundary; it initializes counts and active, not every pointer slot. Each condition holds satisfied and celebrated, initially zero, and virtual satisfaction, death/capture/create and save/load operations. The base IsSatisfied (`0x48ea00`) returns the stored flag; all three base event handlers (`0x48ea10..0x48ea30`) do nothing. No registered condition overrides creation notification, so BuildUnitType is found by polling rather than a creation-event override.

RegisterConditions (`0x48e010`) reads the current mission TDF record and appends condition instances. It does not clear existing counts/instances first and does not enforce either sixteen-element bound. Ordinary single registration can add eleven victory and seven defeat conditions; repeated registration can exceed capacity. FreeConditions (`0x48dfb0`) directly deallocates each recorded pointer, without resetting counts, nulling pointers or calling a derived virtual destructor. These condition types contain inline fields, but repeated FreeConditions or later use still encounters released pointers. Deactivate (`0x4904b0`) sets active zero, affecting the top-level CheckVictory/CheckDefeat gates rather than disabling every method.

AllVictoryConditionsMet (`0x48fed0`) lazily appends DestroyAllUnits if the list is empty, then requires every IsSatisfied result to be nonzero, stopping at the first false. AnyDefeatConditionMet (`0x48ff40`) similarly supplies AllUnitsKilled and returns on the first true. Polling order can delay later condition side effects until earlier conditions permit evaluation. Notifications (`0x4904c0`, `0x490520`, `0x490580`) call every victory entry then every defeat entry without testing active. These calls do not themselves set the global game-result flags.

The player updater in [players_464290.cpp](../../../src/game/players_464290.cpp) periodically calls campaign victory first and defeat only when victory is false, then manages the endgame countdown/result bits. This gives victory precedence at that consumer when both tests could be true. [Players](game-players.md) documents the countdown/observer/network behavior; [endgame](frontend-endgame.md) captures the resulting winner bit and presents it.

## Registration fields and parsing constraints

| TDF field | Registration condition | Constructor input |
| --- | --- | --- |
| KillEnemyCommander | Integer nonzero | No parameters. |
| DestroyAllUnits | Integer nonzero | No parameters. |
| KillAllMobileUnits | Integer nonzero | No parameters. |
| BuildUnitType | String field found | Unit type name. |
| CaptureUnitType | String field found | Unit type name. |
| KillAllOfType | String field found | Unit type name. |
| KillUnitType | String field found | Name, count. |
| MoveUnitToRadius | String field found | Name, x, projected map y/z coordinate, radius. |
| UnitTypePassesX / UnitTypePassesZ | String field found | Name, coordinate. |
| VictoryTimerRunsOut | Integer greater than zero | Time multiplied by thirty. |
| CommanderKilled | Integer nonzero | No parameters. |
| AllUnitsKilled | Integer nonzero | No parameters. |
| AllUnitsKilledOfType | String field found | Unit type name. |
| UnitTypeKilled | String field found | Name, count. |
| DeathTimerRunsOut | Integer greater than zero | Time multiplied by thirty. |
| AnyUnitPassesX / AnyUnitPassesZ | Integer at least zero, default minus one | Coordinate. |

The first eleven are victory conditions; the remainder are defeat conditions. Empty categories receive the defaults described above. String presence, not nonempty contents, drives string-field registration under the [TDF contract](util-tdf.md). Names with parameters are parsed using an alphabetic-only scanset before a comma and integer conversions afterward. Return counts are ignored, integer locals need not have been initialized, and scansets have no explicit field width. Names containing digits/underscore or malformed punctuation can therefore fail parsing rather than behave like a general unit-name grammar. Condition names are copied into 32-byte fields using unbounded strcpy.

ANYTYPE, ignoring case, becomes an empty wildcard only in the radius and victory X/Z constructors. It is not a universal wildcard for build/capture/kill conditions. X/Z thresholds are converted to cell coordinates by shifting right four. Radius shifts left sixteen; its center initially stores the raw configured x and second coordinate with y set to sentinel `0x12345678`. Timer thresholds are absolute game ticks derived from time times thirty, not a duration added to the registration tick. Arithmetic and negative input are not generally validated beyond the registration comparisons.

## Player ranges and unit-event timing

Many campaign conditions deliberately use physical player slots zero and one. The first unit range at game offset `0x1bca` is player zero's inclusive Begin/End pair; the range at `0x1d15` is player one's. The shared local iteration scans every storage record with nonzero unitDefIndex and stops when its visitor returns zero. It does not automatically require completed construction, live flags, ownership stability or no transport. Each visitor adds only its own predicates.

ApplyUnitDeath in [units_485010.cpp](../../../src/units/units_485010.cpp) calls NotifyUnitDied before clearing orders, unlinking footprints or removing the dying unit from normal storage. Death-triggered recount conditions use at most one as the remaining-count test, consistent with the dying record still being visible. This timing is part of their contract; interpreting the threshold as allowing an arbitrary surviving enemy would omit the callback order.

GiveUnitToPlayer in [unit_commands.cpp](../../../src/units/unit_commands.cpp) calls NotifyUnitCaptured before changing owner and before several destination-validity checks or new-unit allocation. Capture conditions consequently see the old playerIndex, and notification can occur even when a later handoff step returns without completing transfer. The predicate is about that event path, not independently confirmed successful acquisition by player zero.

## Victory predicates

| Condition and principal addresses | Actual success rule |
| --- | --- |
| KillEnemyCommander, death `0x48ea40` | Dying unit belongs to slot one and its definition name equals its owning side's commander name, ignoring case. Latches satisfied and announces once. |
| DestroyAllUnits, poll `0x48eb40` | Player one's unitCount is zero. Announces once but returns the live count test without setting satisfied. New units can make a later poll false. |
| KillAllMobileUnits, visitor/death `0x48ec00/0x48ec20` | On death of slot-one unit with nonnull motion, count nonzero-motion records in slot one's unit range, stopping after more than one; latch when count is at most one. No initial empty-army poll. |
| BuildUnitType, visitor/poll `0x48ed50/0x48edb0` | Lazily resolve type ID and scan slot zero for matching unitDefIndex with buildLeft exactly zero. Latches; does not additionally test transport, holdoff or the general eligibility flag. |
| CaptureUnitType, capture `0x48eeb0` | Event unit's old playerIndex is one and name matches. Latches regardless of the eventual destination owner. |
| KillAllOfType, visitor/death `0x48ef80/0x48efb0` | Before satisfaction, a slot-one matching-name death resolves ID and recounts that type in slot one; at most one latches. Count/ID are refreshed on the triggering death rather than initialized from a complete startup census. |
| KillUnitType, death `0x48f0f0` | Matching-name slot-one death decrements a strictly positive remaining count; reaching zero latches. A configured zero/negative count does not immediately succeed and does not decrement. |
| MoveUnitToRadius, poll/visitor `0x48f200/0x48f250` | Resolve center once through the terrain projection helper, enumerate horizontal range, and accept a slot-zero unit matching name/wildcard with flag `0x20`, buildLeft zero, transfer holdoff zero, and no carrier or carrier flag `0x40000000`. Latches. |
| UnitTypePassesX, visitor/poll `0x48f370/0x48f3e0` | Slot-zero matching-name/wildcard record within two cell units of threshold X. Latches and stops enumeration. |
| UnitTypePassesZ, visitor/poll `0x48f4c0/0x48f530` | Same for Z. |
| VictoryTimerRunsOut, poll `0x48f610` | Unsigned gameTick at least the constructed endTick. Does not itself set satisfied or play the condition sound. |

Every latching victory above except the timer plays Victory Condition once by testing celebrated and setting it. DestroyAllUnits also announces on its first true poll. BuildUnitType retries resolution whenever cached ID remains zero; enumeration filters unused unit IDs, so an unresolved zero does not match those skipped storage records.

The radius center helper is [ClampWorldPosToTerrain in line_of_sight.cpp](../../../src/map/line_of_sight.cpp), not a simple read of height at a fixed X/Z. It accepts whole-map x and projected vertical coordinate, clamps them, shifts to 16.16 and searches/interpolates terrain using the half-height projection. It overwrites the center vector, removing the sentinel. [VisitObjectsInRange](map-terrain.md) then uses horizontal X/Z squared distance, not a three-dimensional sphere. Victory X/Z checks likewise test proximity to a band, not directional crossing, prior position or which side of the line was crossed. A record already in the band can satisfy immediately.

## Defeat predicates

| Condition and principal addresses | Actual success rule, meaning defeat |
| --- | --- |
| CommanderKilled, death `0x48f6b0` | Slot-zero commander by owning-side name match. Latches. |
| AllUnitsKilled, visitor/poll `0x48f790/0x48f7e0` | Start satisfied at one on every poll and scan slot zero; any record with flag `0x20`, completed build, zero transfer holdoff and acceptable carrier clears it and stops scanning. Thus unfinished-only or normally transported-only units may not prevent defeat. |
| UnitTypeKilled, death `0x48f8c0` | Any matching-name death, with no ownership restriction, decrements remaining count and latches at zero or below. Unlike the victory count condition, it does not guard remaining count greater than zero. |
| AllUnitsKilledOfType, visitor/death `0x48f9a0/0x48f9d0` | Any matching-name death resolves ID and recounts matching records across both slot-zero and slot-one ranges; at most one latches. Each range iterator can stop once total count exceeds one. |
| AnyUnitPassesX, visitor/poll `0x48fb30/0x48fb60` | Slot-one record within two cell units of X threshold, with no name/completion/transport filter. Latches. |
| AnyUnitPassesZ, visitor/poll `0x48fc40/0x48fc70` | Same for Z. |
| DeathTimerRunsOut, poll `0x48fd50` | Unsigned gameTick at least the threshold converted to unsigned. |

These defeat conditions do not play Victory Condition. Event-only conditions do not notice that a matching type was absent from the initial mission unless a relevant event later occurs. Latched and continuously recalculated predicates coexist: AllUnitsKilled recomputes each poll, while most event/position conditions retain satisfied forever once true.

## Persistence accounts and reconstruction

Every registered condition has paired SaveState/LoadState entries in this interval. Each opens its named HapiBank account and writes/reads Satisfied and Celebrated, with zero defaults on load. Account names are VictoryCondition_ plus the victory field name, or DefeatCondition_ plus the defeat field name; the timer names are VictoryCondition_VictoryTimerRunsOut and DefeatCondition_DeathTimerRunsOut. The principal pairs occupy `0x48eac0..0x48fdb0` alongside the predicate bodies.

KillAllMobileUnits additionally persists NumUnits (`0x48ecb0/0x48ed00`). Both count-down type conditions persist NumLeftToKill (`0x48f160/0x48f1b0` and `0x48f900/0x48f950`). Other transient cached IDs/recount values, configured names, coordinates, radii and timer thresholds are not saved by these bodies. They depend on reconstructing the same condition configuration before loading flags. In particular a saved timer does not store an independent remaining-time value here.

SaveConditions (`0x48fdf0`) and LoadConditions (`0x48fe60`) iterate registered victory then defeat lists only for mission type one. They do not persist the arrays' counts/types or the manager active flag. Accounts are identified by condition class name rather than instance index, so repeated conditions of the same class share/overwrite that account instead of gaining distinct storage. [HapiBank](util-hapibank.md) supplies account/field semantics and defaults; a missing account's creation behavior is not evidence of a valid saved condition record.

## Mode-specific victory, defeat and mismatch timer

CheckVictory (`0x490230`) returns zero when inactive. Campaign type one requires all victory conditions. Skirmish type two checks each other physical slot and succeeds when every nonallied slot's unitCount is zero, without active/type/observer/unitsCreated filtering. Multiplayer type three calls CheckAlliedVictory. Other types return zero. AreEnemiesEliminated (`0x48ffd0`) independently exposes the same skirmish enemy-count test. IsLocalPlayerEliminated (`0x490050`) and the duplicate FUN_00490200 (`0x490200`) simply test local unitCount zero.

CheckAlliedVictory (`0x490080`) first returns false when battleCommanderDeath equals two. Otherwise it examines every other active type-one/two/three, non-sentinel, nonobserver slot. A qualifying slot with unitsCreated zero blocks victory even when unitCount is zero. A slot that has created units and has zero remaining is skipped. Every remaining live participant must have flags_9d bit two set along with the local player, and both local allied and alliedBy bytes for that participant must be set. For each such participant, every active valid slot still holding units or with no prior unitsCreated must also appear in that participant's allied array. The inner scan does not repeat the outer observer exclusion. This requires the checked directed relationships across surviving/not-yet-started participants, rather than just a local ally toggle. There is no additional local-unit-survival check in this helper.

CheckDefeat (`0x490360`) also gates on active. Before the mode switch it reads GetCdPathMismatch. If set and the global mismatch deadline is zero, it assigns floor(rand times 9000 divided by 32768) plus 9000, yielding `9000..17999` for the existing rand range. This is an absolute gameTick threshold, not now plus a delay. Once gameTick reaches it, the deadline resets to zero and defeat returns true. Clearing the mismatch flag does not explicitly clear the saved deadline in this body. [CD checking](game-cd-check.md) documents the source of that unusual flag; this documentation does not infer a broader copy-protection policy beyond the observed calls.

After that check, campaign type one uses any defeat condition, while skirmish and multiplayer both test local unitCount zero. Unknown modes return zero. Active suppresses both normal and mismatch-triggered top-level results, but notifications and direct condition polls remain callable.

## Packed movement-goal family in this address group

The remaining entries extend the [PathGoal/order-target system](orders-order-targets.md). They represent a goal position/velocity/heading associated with an owner unit, rather than mission success conditions. The relevant declarations are [UnitMotion](../../../src/orders/unit_motion.h), [OrderFx](../../../src/orders/order_fx.h), [AirManeuverOrder](../../../src/orders/air_maneuver_order.h), [bit writer](../../../src/network/bit_writer.h) and [bit reader](../../../src/network/bit_reader.h).

PackedGoal constructor (`0x4905e0`) calls PathGoal, copies owner position/heading and zeros velocity. Its generated deleting destructor is `0x490630`. ExportGoalPose (`0x490650`) copies retained position, velocity and heading to caller outputs. TickTowardGoal (`0x490690`) returns without a target; otherwise it asks OrderFx to fill the new goal position and computes velocity as new minus previous position. It measures horizontal owner/goal distance on 16.16 integer components.

Beyond distance `0xa00000`, 160 world units, it replaces goal height with sea level plus definition altitude for seaUnit bit twenty-two, otherwise owner spatialBucket byte one plus altitude, shifted sixteen. This uses the owner's bucket height, not a fresh ground query at the goal. Velocity was computed before that height replacement and is not recomputed. Beyond `0x1400000`, 320 units, it turns heading using GetHeadingBetween. At smaller distances it asks the target for a desired heading and falls back to GetHeadingBetween when refused and farther than `0x100000`, sixteen units. The short-circuit means very distant targets do not receive that desired-heading call. When ContainsUnit(owner) succeeds it adds target flag `0x20`; if KeepAfterComplete is false it clears the path order through SetPathOrder(0).

LiteGoal constructor (`0x4907e0`, generated deleting destructor `0x490840`) adds dirty one and mode zero. SetPathOrder (`0x490860`) delegates to PathGoal then sets dirty. TickTowardGoal (`0x490880`) sets dirty when owner motion's low two mode bits differ from retained mode, then runs PackedGoal movement. HasNetUnitState (`0x4908b0`) returns only dirty bit zero, so movement alone is not a general change detector in this override.

SerializeNetUnitState (`0x4908c0`) emits a two-bit target discriminator: zero for no target, one for OrderFx type two, two for type three. Types two/three serialize their own payload, then two owner-mode bits follow. It copies owner mode into stored state bits one/two while clearing dirty. An unsupported nonnull target type emits no discriminator before the mode bits, so reader alignment would be wrong; the intended contract is that only supported target kinds reach this path. No position/velocity vector is directly serialized by this wrapper.

PackedPosGoal constructor (`0x490940`) inherits retained pose initialization. Its ordinary/generated deleting destructors (`0x4909e0/0x4909a0`) delete the target virtually and null it. DeserializeNetUnitState (`0x490a10`) first deletes any old target, reads two bits and creates the type-two target reader object for one or AirManeuverOrder for two. Zero/three leaves no target. It then reads two mode bits and calls owner UnitMotion.SetFlightMode. This is destructive replacement, without rollback if input or allocation fails. Bitstream bounds validation belongs to the [network bit utilities](network-net-stats.md); this wrapper has no length field or independent validation.

The PackedGoal/LiteGoal generated destructor bodies return through the base destruction/free convention; they add no owned inline resources. PackedPosGoal explicitly owns its received target. Constructors do not initialize every spare byte/flag in the base PackedGoal layout, so avoid assuming that the sender-only dirty/mode byte exists as meaningful state for every derived instance.

## Memory probe and coverage

QueryGlobalMemoryStatus (`0x490aa0`) sets a local MEMORYSTATUS length to 32 and calls Windows GlobalMemoryStatus. It returns no captured values and changes no documented game state in its body. It is not a hidden resource limit or outcome check.

All 98 main-file annotations, including generated lifetime entries, and the separate CheckVictory body have been traced. Mission assets and event timing determine which predicates fire; fixed buffers, array counts and callback ordering are explicit assumptions. No build, game/network execution, dependency installation or source listing was needed for this reference.
