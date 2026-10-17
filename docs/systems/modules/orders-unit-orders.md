# Ground and common unit action state machines

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md) | [Naval case study](../../naval-systems.md)

Module group: `orders/unit_orders`, 50 annotated functions from `0x401c20` through `0x406c70`. Main source: [handlers and registration tables](../../../src/orders/unit_orders.cpp). Separate implementation files are linked beside their behaviors below. The group owns 23 common order registrations and 22 ground registrations, with shared callbacks for several registrations. Aircraft handlers have a separate module even though their table is physically declared in this source file.

## 1. Execution contract

Handlers receive the executing unit, order instance and delivered event bits. The order stores a state byte, linked target, optional position, starting X/Z and three handler-specific integer slots. Count, duration, progress, radius and retries alias those slots. [Order execution](orders-order-list.md) defines event delivery, waits, queues, cancellation and persistence.

For a primary order, return 0 resets state, 1 increments it, 2 retains it, 3 retains and delays, 4 retains with a small randomized delay, 5/8 removes it, 6 rotates it to the tail, 7 clears the queue and 9 retries or yields according to whether another node follows. Secondary execution interprets several terminal values differently. A failure returning 7 can affect more than one action.

Deadlines are simulation ticks, normally 30 per simulated second. A deadline works with flags controlling which events can wake the action. Arrival 0x20, path failure 0x40 and destination replacement 0x80 originate in [destination/path-goal logic](orders-order-targets.md). Delivered events differ from the order's stored wake mask.

Weapon claiming/releasing coordinates action ownership. Assigning a target does not guarantee a shot: dispatch chooses a command family, while weapon simulation applies eligibility, reach, aim, reload and costs. AppendOrder is historically named: [insertion](orders-order-list.md) establishes that it prepends, allowing temporary work to interrupt a parent action.

## 2. Stop, selection and waiting

StopOrder (`0x401c20`) announces status, clears all three weapon targets and, for a canfly definition in runtime mode 2, prepends VTOL_LANDIFCAN at its current position. It completes without directly moving an airborne model to terrain height.

MakeSelectableOrder (`0x401cc0`) clears runtime 0x8000, sets 0x20 and completes. WaitOrder (`0x401ce0`) with a nonzero radius queries visible enemies around the unit; any result ends the wait. Otherwise a nonpositive remaining duration ends it. A positive duration is reduced by a random 150..179 ticks and that interval becomes the next deadline. Coarse sampling can overshoot the remaining duration. With zero radius, state 0 waits the supplied duration, state 1 completes and invalid state returns 7.

WaitForAttackOrder (`0x401fd0`) ends if its linked target disappears. Otherwise it installs mask 0x18, advances, then completes on the next execution. It does not independently scan for a new enemy. QMove/QPatrol share `0x403160`, wait 60 ticks and return 6. They serve as queued/rally metadata rather than executing ground movement themselves.

## 3. Activation, cloak and standing settings

Activate/Deactivate (`0x403010`, `0x403040`) change state selector 1 only with definition flags2 bit 2, onoffable. CloakOn/CloakOff (`0x403070`, `0x4030a0`) change runtime request 0x800 only with flags2 bit 13, derived from positive cloak cost. [Economy processing](game-economy.md) determines actual cloak activation and spending.

StandingMoveOrder (`0x4030d0`) writes runtime bits 18..19. StandingFireOrder (`0x403100`) writes bits 20..21 and, for parameter 0 or 1, clears targets on slots with flag 0x10. These are literal body offsets. Other provisional views label bits 18..19 fire and 20..21 move. This naming mismatch is explicit rather than resolved from English handler names. Both complete immediately; two-bit assignment truncates the supplied value.

## 4. Destruction, paralysis and carriage

SelfDestructOrder (`0x402010`) uses its count's upper nibble as an initialization marker and the definition's three-bit countdown as the initial value. Ordinary steps decrement the low bits, select countdown speech and wait 30 ticks. Zero schedules a random delay below 15, then self-damage 30000 of kind 3. A nonzero done slot bypasses countdown. Event 2 cancels with speech when runtime 0x4000 is absent.

The speech array has six entries indexed by countdown, without a local guard for values 6/7 representable by the definition. Both SelfDestruct and SelfDestructFG share the callback with different defaults. Asset/caller constraints matter.

ParalyzeOrder (`0x402d10`) caps duration at 1800, claims weapons, clears targets, removes the movement destination, waits and sets state selector 16. It zeros its duration so a later execution clears that selector and finishes. BeCarriedOrder (`0x402fc0`) ends when carrier becomes null; otherwise it claims weapons once and waits repeatedly for ten ticks. Attachment coordinates and packets belong to [positioning](units-unit-position.md) and COB callbacks.

## 5. Stationary combat and unit-type targeting

AttackNoMove (`0x402160`) ends on missing target or mask 0x10808. After announcement it claims weapons with argument 0, assigns slot 0 and installs 0x11808. Its final state releases with argument 3 and returns 9. It never attaches a path goal to close range.

GuardNoMove (`0x4021f0`) releases weapons and waits 30 ticks. It adopts slot 0's target if runtime 0x10000000 is present, remembers position, claims ownership and chooses a persistence limit of 3..5 passes. Event 0x4000 resets the counter; otherwise it increments. Within that limit and reach it waits with 0x7008. Later an 80 percent random branch restarts the target cycle. Interrupt 0x10008 directs recovery to a random visible enemy within 640 world units of the remembered position. This differs from a moving escort.

AttackUType (`0x401e00`) requires canattack, waits 1..90 ticks and scans slots for the requested definition and nonallied owner. It scores squared horizontal distance minus a random amount below half that distance, preferring the last equal score. A winner passes through attack dispatch and is prepended; the search resets. No winner ends it. This body adds no independent sight, completed-build or comprehensive live-slot check.

AttackSpecial (`0x403190`) resolves attack mode 3, merges the resulting defaults, selects weapon index 2 and retains state. It converts command type rather than implementing a new projectile.

## 6. Movement, patrol and kamikaze

MoveGround (`0x4031d0`) rejects a carried mover, attaches an approach circle with supplied radius plus four and watches 0xe0. Arrival announces completion and removes it; other wakes return 9. Actual acceleration, obstacles and water travel belong to [steering](units-unit-scripts.md) and [movement classes](units-movement-class.md).

AttackKamikaze (`0x403260`) refreshes cached position while its linked target exists, rejects a carried mover and approaches with radius at least 16, otherwise the definition's kamikaze radius. It waits 60 ticks and watches 0xe0. Arrival prepends SelfDestruct with done set, selecting immediate destruction. Path failure returns 8; other wakes reset approach. Missing target alone retains its last position; 0x10008 terminates.

Patrol (`0x4033a0`) requires motion and ensures a return patrol node. It releases weapons, attaches a zero-radius destination and samples for attacks. Event 0xe0 returns to movement state and rotates the node. Otherwise fire-at-will target lookup and IssueAttackOrder can suspend patrol behind a new attack. No attack sets a random 30..59 deadline and returns 4. The loop depends on queue membership and the return-position helper.

## 7. Chasing and suppression

AttackChase (`0x4034a0`) ends on event 0x800, mask 0x10008, missing target or exceeding its optional starting-position leash. Initialization requires motion, a nonflying definition and runtime 0x80000000. It remembers position, resets its approach step and chooses a weapon when selector is zero.

It clears movement before testing reach. Without event 0x3000, a reachable target is assigned after weapon claims 0/2 and waits with 0x13808. Otherwise position search uses weapon range: full-range approach; randomized positions at range with quarter-range acceptance; half-range approach; exact center; ring radii range/half-range; ring twice-range/range; repeat. Vertical difference above eight world units skips side approaches toward the half-range stage. The randomized-side branch retains its step in that body; another transition must advance it.

After movement, 0x40e0 returns to reach testing with delay. Reachable targets are reassigned with deadline 30 and 0x148e8; otherwise weapons release and 0x100e8 waits 30. The ring acceptance/heuristic disagreement in [target geometry](orders-order-targets.md) applies to these actual radius pairs.

Suppress (`0x4038a0`) attacks a fixed position and rejects flying definitions. It stores selected weapon range. Selector 2 claims all and assigns slot 2; other selectors claim 0/1 and assign slots 0/1. It waits with 0x1c00, then releases. Event 0x400 restarts/rotates firing. A mobile unit with positive remaining radius approaches, decreases that radius by a random amount below one third of weapon range and resumes firing. Nonmobile/exhausted cases return 9. There is no local positive-range guard around that random helper.

## 8. Factory and shipyard construction

BuildingBuild (`0x402640`) is factory production despite a provisional comment calling it mobile construction. It waits for runtime 0x20000000, checks queued count, activates selector 1 and waits for build stance. QueryBuildInfo selects the construction piece, initially -1. The product footprint is checked at that piece's world position using the factory runtime mode.

Blocked placement retries after 15 ticks; allocation failure after 300. A new unfinished target attaches to the factory piece with construction flag 1, receives both standing-setting fields and GetBuilt. Factory selector 8 becomes active and its build menu refreshes.

The productive state calls AddBuildProgress with integer workerTime / 30 converted to float. A successful work request emits six nano particles from the builder's nano piece into target bounds; failed work emits none here. While buildLeft is nonzero it waits one tick with 0x0a. Completion announces, clears selector 8, calls FinishConstruction, drops the target, decrements count, refreshes the menu and resets.

Cancellation event 2 credits the integer part of `(1 - buildLeft) * target metal`, scaled to 0.5/0.7 for active AI type 2 on difficulty 0/1, otherwise full. It calls FinishConstruction, damages the unfinished target by 30000 kind 9, clears selector 9 and refreshes the menu. This is not an energy refund. Event 8 instead announces stopped construction, decrements count and resets.

Construction visuals consume the target's buildLeft and QueryBuildInfo attachment independently of the source factory's build state. Shipyard wireframes and partial models are explained in the [naval reference](../../naval-systems.md). This handler neither draws them nor selects their palette. Different product scripts/models place pieces differently with the same production handler.

## 9. Mobile construction and assistance

[MobileBuild (`0x403a20`)](../../../src/orders/unit_orders_403a20.cpp) snaps site X/Z to the product footprint grid and attaches a border destination. A path failure is tolerated if distance minus both footprint half-diagonals is within build range; otherwise the site is unreachable.

Blocked placement announces once, retries every 30 ticks and fails after more than ten prior retries. Clear placement claims weapons, snaps position and creates an unfinished target. Allocation failure waits 300. Success assigns GetBuilt, starts relative-heading build animation and waits for stance. Work uses integer workerTime / 30, successful six-particle nano requests, deadline tick plus 300 and one-tick waits with 0x0a. Completion removes the action. Event 8 fails and event 2 completes, both refreshing selection; neither uses the factory refund path.

HelpBuild (`0x403f70`) requires motion and builder flag 0x40. Its target-footprint radius is `integer(sqrt(X*X + Y + Y) * 16) / 2`, not a two-squared-axis formula. It attaches ring radii build-range-plus-radius and radius. Path failure reports unreachable; an already completed target ends assistance. It claims weapons, starts animation, marks selection dirty and waits for stance.

Assistance uses integer workerTime / 30, successful six-particle requests, work deadline tick plus 300 and one-tick waits. It ends when buildLeft reaches zero, announcing completion and setting bit 2. Missing target fails; cancellation bit 2 refreshes selection and completes. [Progress helpers](../../../src/ingame/control_panel.cpp) and [economy](../03-economy.md) supply funding/shortage behavior.

## 10. GetBuilt and inherited commands

GetBuilt (`0x402da0`) runs on the product, linking back to its builder. While unfinished it claims weapons, waits 300 ticks, then 30, then repeatedly waits. In the last state event 0x8000 schedules 30 ticks; event 1 schedules 11 and applies unfinished-build decay for 11. It sets waiting after each stage. Work notifications affect decay rather than granting every unfinished unit permanent protection.

At exactly zero buildLeft it marks selection dirty. A motion-enabled product walks the builder's primary list and dispatches QMove/QPatrol positions into its own concrete movement/patrol actions. If both units are Ready it copies standing fields; active local player type 1 also gets the builder's group value directly, without squad-vector maintenance. If no movement copied, it assigns Park, then completes. A ship, aircraft and vehicle can therefore inherit different dispatched movement types from one factory rally marker.

## 11. Stockpiled weapons

BuildWeapon (`0x402b70`) uses selected slot, count and progress ticks. Nonpositive count ends it; stockpile at least 200 waits 300 ticks. A round starts at zero and advances by five, capped to weapon build time. Each charge is the difference between truncated cumulative energy/metal costs at new and previous progress.

RequestEnergyAndMetal must accept both before progress changes. Failure retries after ten ticks; incomplete success waits five. Completion increments stockpile, decrements queued count, refreshes menu and resets. No nano effect is requested. Build time is a divisor without a local zero guard; normal commands provide a valid weapon index.

## 12. Three repair roles

| Handler | Work relationship | Requirements |
| --- | --- | --- |
| SelfRepair, 0x402430 | Linked builder repairs executing unit | Completed linked builder and active executing unit; no approach |
| RepairUnit, 0x405300 | Executing builder repairs target | Motion, builder flag, completed mode-1 target and optional starting-position leash |
| RepairUnitNoMove, 0x405740 | Executing builder repairs target | Builder flag, completed target and active executing unit; no approach |

All use integer workerTime / 30 from the actual builder. Successful AddRepairProgress requests six nano particles into target bounds. Failed work still retries without that effect. Work deadline is tick plus 150 on the executing/repaired unit for SelfRepair and on the builder otherwise. The loop waits one tick watching event 8; full health advances to completion speech.

Mobile repair subtracts both footprint half-diagonals from distance. Outside build range it attaches a footprint destination, watches 0xe8 and waits 30..59 ticks. Path failure ends it. In range it claims weapons, starts relative-heading animation and waits for stance. Target runtime 0x0c stops animation, waits 15 and restarts approach. No-move repair instead advances toward completion on that moving mask. SelfRepair has no equivalent target-motion exit. Missing references end/fail with repair speech.

## 13. Capture

Capture (`0x404270`) requires motion and cancapture, flags2 0x1000. It rejects a target with that same bit, reporting it cannot be captured. This is the literal test, not a separate capturable property. Unfinished targets, missing targets and event 0x10008 fail.

Base duration is integer target energy times 30 / 2000 plus target metal times 30 / 140 plus 150, capped to 1800. It then scales by `(health + maxHealth) / (2 * maxHealth)` with the body's integer arithmetic order, and integer `(kills / 5 + 10) * duration * 10 / 100`. Lower health speeds capture; experience extends it. The initial cap precedes later scaling.

It claims weapons, approaches the target footprint and tests distance minus both half-diagonals against build range. After animation/stance readiness it emits six reverse nano particles from target bounds every two ticks, increases elapsed by two and sets work deadline tick plus 900. Moving target motion plus runtime 0x0c stops animation, waits 30 and resets. At duration it calls GiveUnitToPlayer and announces completion without checking a return status. Its progress loop requests no energy/metal payment.

## 14. Live-unit reclaim

ReclaimUnit (`0x404730`) requires motion, canreclamate (flags2 bit 10) and CanReclaim. Missing target or 0x10008 ends it. It approaches the footprint, computes a damage pulse for 15 ticks through the [order helper](orders-order-queue.md), starts animation and waits for stance. Later path failure returns 9.

Work requires squared horizontal world distance within `(build range + target radius)^2` and renewed CanReclaim eligibility. At accumulated duration at least 15 it applies the pulse as kind-5 damage and resets duration. Two-tick work intervals can cross the threshold at 16. Each valid interval requests six reverse nano particles and sets work deadline tick plus 900, without a local resource request. Leaving range/eligibility stops animation, waits 15 and resets. Damage helpers supply actual recovery/destruction; particles do not imply a damage pulse every two ticks.

## 15. Terrain-feature reclaim

[Reclaim (`0x404ad0`)](../../../src/orders/unit_orders_404ad0.cpp) resolves the feature on every call. Missing feature or absent flag 0x80 fails. Motion and canreclamate are required. It approaches the border and fails on path failure, aiming build animation inside feature height above ground.

Duration is integer `15 + 0.5 * (metal + energy)`. After stance/speech it decrements by two every two ticks. While remaining time exceeds 15 it requests reverse particles twice, six each, from ground-based feature bounds. Work deadline becomes tick plus 300. Expiry calls ReclaimFeature once. WorkerTime does not scale this duration. [Feature handling](map-features.md) supplies removal and resource authority.

## 16. Resurrection

[Resurrect (`0x404db0`)](../../../src/orders/unit_orders_404db0.cpp) resolves a reclaimable feature through states 0..5, requires motion and canresurrect (flags2 bit 11), approaches and waits for animation stance. It copies a 64-byte feature name, truncates at the first underscore and resolves that prefix as a unit definition. Missing definition fails.

Preparation time is integer `target buildTime * 0.3 / (workerTime / 30)`. The inner division is integer and locally unguarded against zero. Preparation requests six forward nano particles each tick and work deadline tick plus 300, without an energy/metal request.

It creates a unit at the order position for the resurrector's owner. Allocation failure waits 300 and retries. After creation it validates the origin feature cell, copies feature rotation, removes the feature and, for game type 3, broadcasts a six-byte removal packet with type 0x0f, subcode 0xff and cell X/Z. The new unit becomes completed with health 1. A later state announces and dispatches mode 8 against it, prepending repair/service when available.

Creation precedes sentinel-cell validation; that failure path does not delete the already created unit. Name-prefix mapping, feature state and nonzero work rate constrain resurrection. It is not arbitrary revival of every wreck.

## 17. Repair patrol and scavenging

[RepairPatrol (`0x405980`)](../../../src/orders/unit_orders_405980.cpp) remembers linked target position and ensures its return node. It approaches with radius 16, waits 60 and watches 0xe0. At least 20 percent energy storage permits a scan within definition range for damaged/unfinished allies, choosing randomly. Normal mode-8 dispatch and IssueRepairOrder still gate inserted work.

DamagedAllyCollector (`0x405d90`) excludes self, nonallies, modes other than 1, completed full-health units and units last damaged by this owner with kind-5 reclaim damage. This prevents automatic repair immediately undoing the owner's reclamation. Collection alone does not guarantee service eligibility.

Below 20 percent energy or metal, patrol looks for reclaimable feature resources. Preference is metal when metal-low, energy when energy-low, then metal whose full yield fits, then energy whose yield fits. It clears movement/mask and prepends Reclaim, yielding patrol. It does not unconditionally reclaim every nearby wreck.

## 18. Standby, mines and parking

Standby (`0x405fe0`) requires motion, releases weapons and waits one tick with 0x10000. Later fire-at-will lookup can insert attack and end standby; otherwise it polls every 30..59 ticks. StandbyMine (`0x406090`) instead requires runtime 0x20000000. With a mode-1 candidate and nonzero 0x300000 standing mask it prepends immediate SelfDestruct. Blast collision is downstream.

Park (`0x4061a0`) requires motion. Flying definitions convert to current-position VTOL_MOVE. Others attach a rectangle around their position, with size from footprint X plus three when the definition's signed height field is nonnegative. Extent is size times eight by size times six cells, starting size times four/three behind the current cell. Arrival or another queued node ends it; otherwise it waits 30 and resets. The marker accepts a border, so this is not merely a center-position zero-velocity command.

## 19. Escort, retaliation and assistance

[FollowGround (`0x406300`)](../../../src/orders/unit_orders_406300.cpp) ends on missing target, rejects carried followers and flying targets. It releases weapons and chooses a random offset with spacing `(follower footprint X + target footprint X + 2) * 16`. Each refresh approaches current target position plus that offset with half-spacing radius, deadline 30 and mask 0x18.

It reacts to the guarded unit's attacker on event 0x10 when the attacker's alliance table does not list the follower owner and the follower's exclusion bitset lacks that definition. IssueAttackOrder override 1 is tried first. Failing that, nonzero runtime 0x300000 permits individual retargeting of slots with flags 2 and 0x10 and without weapon-definition bit 26. A slot changes if empty, unreachable or its old target is in the per-weapon exclusion bitset. Weapon simulation still gates actual shots.

A builder follower can interrupt to repair a damaged guarded unit through mode 8. With both units builders and guarded action flag 0x100000, it can copy actionable target/position work. MobileBuild/BuildingBuild instead generates HelpBuild against the current product. It avoids assisting an action aimed back at itself. Temporary work prepends while guard remains resumable, explaining assistance at shipyards.

## 20. Ground transport and teleport

GroundPickup (`0x406780`) requires motion, canload and target footprint X no larger than transport capacity. This is not an area comparison. It waits for COB, starts TransportPickup with target ID, announces and waits 15. After another busy wait any nonnull target carrier counts as success, not necessarily this transport. Otherwise at most three attempts approach the target exactly between retries. Missing target/event 8 fails. Attachment is script-driven.

GroundUnload (`0x406900`) selects the cargo head, calls TransportDrop with its ID and packed requested X/Z, waits 15 and waits for COB. Success means that cargo carrier differs from this unit. After three attempts it returns 9; otherwise it approaches/restarts. A canhover definition uses integer radius `height180 * 1.5`, others zero. It completes one cargo-head action rather than detaching the entire chain. Event 8 fails; invalidation before its final carrier comparison is not guarded there.

[Teleport (`0x406aa0`)](../../../src/orders/unit_orders_406aa0.cpp) scans the inclusive unit array for every other unit position within the teleporter's world bounds, including Y. It preserves relative offset at destination, requests 30 teleport particles with parameter 5 and calls SetUnitPosition flag 1. There are no local alliance, completion, transportability or destination-passability tests. The teleporter itself stays. Position helpers determine terrain/collision consequences.

## 21. Registration, overlays and persistence

RegisterUnitOrders (`0x403180`) installs 23 common entries; RegisterGroundOrders (`0x406bf0`) installs 22 ground entries. Defaults supply status, callback, draw callback, target/draw mask, flags, internal marker and name. SelfRepair and Standby_Mine have internal marker 1. Shared callbacks explain registration versus function counts.

Movement/patrol/follow/service actions use path overlays; MobileBuild uses a footprint overlay; attack, pickup/unload, teleport, capture, HelpBuild and no-move repair use their registered coverage callbacks. These are command UI, separate from nano particles and construction models. [Overlay dispatch](orders-order-list.md) explains masks and the queued-position cursor; rendering modules implement the callbacks.

Four unit-pointer support functions cover trivial vector destruction, [copy](../../../src/orders/unit_orders_406c10.cpp), [fill](../../../src/orders/unit_orders_406c40.cpp) and [conditional pointer construction](../../../src/orders/unit_orders_406c70.cpp). They do not delete the pointed-to units. The last copies one pointer-sized value only when destination is nonnull.

Handlers have no independent serializer. [Order persistence](orders-order-list.md) records type, state, reused fields, target and attached destination. COB waits and attachment additionally depend on component restoration. Resurrection directly creates a feature-removal packet; creation, transfer, attachment and positioning helpers handle their own replication.

This source-traced reference accounts for every handler, both owned registration families and adjacent support entries. Literal quirks include standing-field naming disagreement, capture rejecting another capturer, assistance's footprint formula, integer work rates, limited countdown speech storage and resurrection's post-creation validation. Asset-specific animations, construction wireframes, particle rendering and trajectories remain in their respective layers. No builds or game runs were used.
