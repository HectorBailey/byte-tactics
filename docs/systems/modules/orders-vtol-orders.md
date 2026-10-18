# Aircraft actions, landing, combat and airborne service

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md) | [Common and ground actions](orders-unit-orders.md)

Module group: `orders/vtol_orders`, 30 annotated functions from `0x40f200` through `0x4158d0`. The [main aircraft implementation](../../../src/orders/vtol_orders.cpp) contains shared helpers and several handlers; separately retained handlers are linked below. The 22-entry aircraft table is physically in [unit_orders.cpp](../../../src/orders/unit_orders.cpp). Its registration wrapper at `0x415b20` is physically and address-assigned to [network/net_stats](../../../src/network/net_stats.cpp), so it is a cross-module dependency rather than a thirty-first function here.

## 1. Orders choose objectives; motion supplies flight

These handlers attach PathOrder or AirManeuverOrder destinations, claim/release weapons, call COB, schedule waits and insert temporary work. [Aircraft steering](units-unit-scripts.md) interprets the resulting position/velocity/heading, applies acceleration and turn limits and updates the unit. An aircraft action does not directly move a model along a precomputed animation curve.

[Order execution](orders-order-list.md) defines return values and primary/secondary queue behavior. [Target geometry](orders-order-targets.md) defines fixed-point positions, linked units/pieces, altitude, strict approach radii and desired heading. Class_0044e190/e250/e2d0/e330 spellings in provisional sources are constructors/views of that PathOrder family. In particular, an approach radius bypasses the ordinary target-heading/altitude completion conditions. AirManeuverOrder advances its internal destination on position queries, and its SetAltitude does nothing.

Most initialization requires a nonnull motion object and definition canfly bit 11. This is independent of canhover, water flotation, hoverattack and current landed/flying mode. [Definition loading](units-unit-types.md) establishes these flags. Durations below are simulation ticks, ordinarily 30 per simulated second. AppendOrder prepends temporary work; AppendOrderToTail actually appends recovery work.

PrepVtolClimb (`0x40f200`) claims weapons, detaches a carrier with attachment flag 2 and activates state selector 1. For motion mode 1 it switches to mode 2 and attaches a current-position destination with half cruise altitude, watching caller mask plus 0xe0. Other modes do not receive that new climb destination. Prep therefore combines release from a pad/carrier, activation and a conditional ascent stage.

## 2. Boundary recovery

Several handlers detect the overflow spatial bucket, representing a position outside the ordinary map partition. LandIfCan, Follow, SeekAttack, SeekGuard, AirToAir and AirToGroundHover construct a destination 800 world units toward map center, with radius 128 and mask 0xe0, then retain their state. Center Y is not used to define horizontal heading.

AirToGround instead installs a 30-tick deadline and changes to attack state 2. AirStrike has no equivalent overflow-center branch in its body. These differences matter when tracing an aircraft that keeps running past an edge: boundary recovery is not one universal rule in every aircraft action.

## 3. Terrain landing and idle behavior

VtolLandIfCan (`0x40f2a0`) immediately ends if another order follows or path-failure event 0x40 arrives. On initialization it preserves a nonzero supplied origin, otherwise uses current position, chooses a random orbit angle and prepares climb.

Its search first tests the current position with CanPlaceFootprintAt. A valid location starts EndTransport, attaches a current-position destination, clears activation selector 1 and chooses altitude so water does not automatically force the landing plane to sea level. Specifically it computes max(ground, sea), then uses ground-minus-sea at or below sea and zero otherwise through PathOrder SetAltitude.

If the current location fails, it tries twelve random offsets, one at each radius 64 through 240 in increments of 16, snaps each to the aircraft footprint grid and tests placement. A valid candidate gets a destination and the search state repeats after movement. If none works, an 0xe0 event decrements angle by 0x5555 and it circles 160 world units from the remembered origin with radius 64. The final state sets flight mode 1 only on arrival 0x20; other completion wakes fail. It does not reserve terrain during candidate sampling.

VtolStandby (`0x40f7d0`) claims weapons, remembers the source unit's whole-unit X/Z, watches 0x10000 and waits one tick. Fire-at-will target lookup can insert an attack, clear its mask and reset standby for later resumption. Otherwise a mode-2 aircraft with cargo wanders 8..39 world units around the remembered point at cruise altitude, waiting 30..44 ticks. A mode-2 aircraft without cargo prepends LandIfCan and ends standby. Other cases poll every 30..59 ticks. Idle cargo therefore changes the landing policy.

VtolMove (`0x40fa20`) prepares climb, announces, releases weapons, snaps its destination to its footprint grid and attaches a fixed position with mask 0xe0. Its final state completes and gives movement speech only if no later node follows. That state does not independently require the wake to be arrival rather than another 0xe0 event.

## 4. Ground-ally search and attack search

[SeekAttack (`0x4103e0`)](../../../src/orders/vtol_orders_4103e0.cpp) ends on 0x40. At initialization it first tries IssueAttackOrder for an existing target; otherwise it establishes a remembered orbit origin, random angle and climb state. Its recurring state releases weapons and, below `3 * (maxHealth >> 2)`, looks for landing hosts within 3840 world units. A random returned host gets a prepended VTOL_LANDING and the search resets. It then tries fire-at-will acquisition. Failing an attack, 0xe0 decrements angle by a random 0x5555..0x7554, and it orbits at weapon-range-plus-160 with radius 128 and a 30..59 deadline.

[SeekGuard (`0x410850`)](../../../src/orders/vtol_orders_410850.cpp) also ends on path failure and remembers a random orbit around a default/current origin. Its recurring helper prioritizes damaged-aircraft landing as above, then visits ground allies in definition range. [GroundAllyVisitor (`0x410c70`)](../../../src/orders/vtol_orders_410c70.cpp) collects nonself, nonflying units whose own owner's alliance table lists the searching owner. It does not filter full health or require construction work. The first collected unit, rather than a random one, is passed to guard-mode dispatch 7 and a new order is prepended without a local zero-type check. With no candidate it orbits at weapon-range-plus-160, radius 128, deadline 30 and mask 0xf8; event 0xe0 decrements angle by 0x4000 plus random below 0x2000.

Landing search returns candidate hosts through a separate spatial helper. Choosing a host here is not reserving one of its script-reported pieces. Pad availability is checked during the later landing action.

## 5. Escort and cooperative work

[VtolFollow (`0x40fbe0`)](../../../src/orders/vtol_orders_40fbe0.cpp) requires a live target and absence of 0x48. It copies target position, prepares climb, chooses a random orbit angle and releases weapons. A hostile attacker on the guarded unit can trigger override attack on event 0x10, subject to the escort's definition-exclusion set. Failing insertion, nonzero runtime 0x300000 permits slot-level retaliation under the same flags 2/0x10, weapon bit-26 exclusion and old-target reach/category tests used by ground escort.

CanRepair plus mode-8 dispatch can insert direct service against the guarded unit. Assistable builder work requires both definitions' builder bit, a nonzero current action, flag 0x100000, a repair-compatible current target and avoidance of self-target work. MobileBuild, BuildingBuild and VTOL_MobileBuild map to VTOL_HelpBuild. Other actionable work can copy its target/position while translating RepairUnit, Reclaim, ReclaimUnit and HelpBuild into their VTOL variants.

Otherwise 0xe0 shifts orbit angle. Runtime 0x80000000 chooses weapon-range-plus-160 spacing; its absence uses 320. The destination accepts radius 128, waits 30 and watches 0xf8. When target/event requirements fail, an otherwise empty queue gets VTOL_SEEKGUARD appended before follow ends. Guard can thus recover to searching rather than becoming permanently idle after losing its specific target.

## 6. Patrol

[VtolPatrol (`0x410e70`)](../../../src/orders/vtol_orders_410e70.cpp) ensures a return patrol node, announces, prepares climb and releases weapons. The next state clears 0xe0 in the delivered-status storage. In recurring state, 0xe0 rotates the patrol node. Otherwise it flies to a point 320 world units beyond the supplied patrol position along the heading toward that position, with approach radius 336. This overshooting waypoint is distinct from exact ground-waypoint arrival.

Damage below `3 * (maxHealth >> 2)` can prepend a random landing host within 3840. Otherwise fire-at-will acquisition can suspend patrol behind an attack. It waits 30 ticks when neither action interrupts. The large radius, overshoot and queue rotation jointly govern waypoint behavior.

## 7. Landing-pad lookup and host attachment

FindLandingPad (`0x411840`) reuses a requested piece if IsPadSlotFree succeeds. Otherwise it initializes four results to -1, queries QueryLandingPad and returns the first reported free piece, or -1. Piece IDs and layout come from the host's COB/model. The four-result limit is explicit. This method is an availability check, not a reservation.

[VtolLanding (`0x4118e0`)](../../../src/orders/vtol_orders_4118e0.cpp) fails if the host disappears. After climb and a random angle it checks availability, using nested FindLandingPad calls in the initial state. With no pad it circles the host at primary weapon range and adds 0x4000 to its angle. With a pad it first approaches host center within 160, then re-queries and approaches the selected piece within 48. A pad lost during approach resets landing with speech.

After an intermediate state it either advances on arrival or rechecks the pad, attaches a piece-following destination and sets altitude zero for an unladen aircraft, or cargo model-top whole height when carrying a unit. It starts EndTransport, waits 15 ticks and repeats until arrival. Final path failure ends it. Final availability is checked again, so another aircraft can consume a previously free pad.

With cargo, EndTransport runs and cargo attaches to the host piece. Without cargo the aircraft itself attaches. An unladen damaged aircraft gets SelfRepair only when the host is completed and has both isairbase (flags1 bit 9) and builder (bit 6). The aircraft's motion destination is cleared first. These conditions are separate from merely finding a free script piece. Attachment and subsequent climb/detachment use the shared [position system](units-unit-position.md).

## 8. Air pickup and terrain unload

VtolPickup (`0x4111b0`) requires a target, no event 0x10048, no current cargo and a target whose model top plus world Y is strictly above sea level. A fully submerged target fails even if its owner is allied. Initialization checks motion/canfly and compares target footprint X to transport capacity, despite speech describing weight.

It approaches a linked target at cruise altitude within 48. QueryTransport supplies an attachment piece, initially -1. BeginTransport receives target model-max-Y both locally and through SendScriptCallByName. A new target-following destination uses negative whole Y of the transport piece offset so the piece reaches the load. Event 0x42 in the final pickup state starts EndTransport and fails; otherwise the target attaches to that piece and loading speech plays.

The success branch allocates a current-position climb destination and sets its altitude but never calls SetAttachedFx. Its allocation is consequently unowned in this body and the previous movement objective remains. This is an observed leak/missing-assignment path. Also, the leading cargo-present guard runs before every state, so a later execution after successful attachment can return failure 8 before reaching nominal state 5 completion. These literal control-flow limits must not be hidden behind a generic successful pickup narrative.

VtolUnload (`0x411560`) ends immediately if the cargo chain is empty. It links the cargo head, approaches the requested position at cruise altitude within 320, then checks the cargo definition's footprint placement using mode 1. Failure reports unable-to-unload and returns retry/yield 9. A valid position gets a second destination with altitude equal to cargo model-top whole height.

It rechecks placement before detach, fails/retries on 0x40, calls EndTransport and detaches the current cargo with flag 1. It then attaches a climb destination at the aircraft's current position with cruise altitude and mask 0xe0. A nominal final state announces unloading completion. The leading empty-cargo test can complete earlier, skipping that speech when detachment emptied the chain. Unlike ground TransportDrop, this action itself performs the detach after placement checks.

## 9. Attack interruption and target recovery

The four attack handlers can append SeekAttack when interrupted and otherwise idle. AirStrike/AirToGround use event mask 0x1000a; AirToAir/AirToGroundHover use 0x10008. This is a real cancellation-policy difference. A target-required action with a lost reference also ends and can append a position-based search. Optional leash compares whole-unit X/Z to the order's stored starting point. The handlers do not all apply the same overflow behavior described earlier.

Commonly they use weapon slot 0, while normal [command dispatch](orders-order-dispatch.md) chooses bombing, air combat, ordinary ground attack or hover attack according to the unit/target/position context. Assignment here is still subject to final weapon eligibility and aiming. Aircraft attacking ships therefore do not bypass water/sensor/weapon tests simply because they can fly to the target.

## 10. Bombing run

[AirStrike (`0x411f50`)](../../../src/orders/vtol_orders_411f50.cpp) updates cached position from a live target but can also bomb a fixed position. After climb it releases weapons then claims slot 0. If initially within 480 world units, it flies 2240 forward toward the target with approach radius 960 to gain run-up room.

It then chooses a randomized half-distance approach, angular spread plus/minus 0x2000, with radius 480. After an intermediate state it derives a release approach radius from cruise altitude, map gravity, motion field +0x22 and attackrunlength: integer `sqrt(2 * cruisealt / gravity) * 30 * motionField22 + 1`, then adds attackrunlength. The float/integer conversion follows the body; this is not a general ballistic simulation or a measured flight-time guarantee. Zero gravity exits to return 7 rather than supplying a fallback.

The release-approach destination follows a unit when present, otherwise uses cached position, and waits one tick with 0x100e8 until a destination event advances it. Firing assigns slot 0 a position target and sends the aircraft forward by attackrunlength plus 960 with radius 960. It then clears the weapon target and flies another 1440 along its heading with radius 128. Below the three-quarter health threshold it can prepend landing; absent a host it resets the run anyway. Otherwise it loops from attack state 3. Projectile release, gravity integration and impact remain weapon/projectile responsibilities.

## 11. Ground strafing

[AirToGround (`0x412710`)](../../../src/orders/vtol_orders_412710.cpp) uses primary weapon range even though its local helper calls that value speed. It releases weapons and approaches a randomized half-distance point within radius 128. Then it claims slot 0, assigns either the unit or fixed-position target and approaches within weapon range.

The escape objective lies three weapon ranges beyond the target along the relative heading, with a random 128..255 acceptance radius and 0x100ea. It can divert to landing below three-quarter health. Otherwise a random left/right quarter-turn waypoint at one weapon range, radius 128, sets up the next run. A final state loops back to target approach. These distances are not aircraft maximum velocity despite provisional local names.

## 12. Air combat and evasion

[AirToAir (`0x412d40`)](../../../src/orders/vtol_orders_412d40.cpp) prepares climb, initializes its pursuit counter and assigns slot 0 each recurring state. IsAhead uses a horizontal dot product of a 20-unit target direction and facing vector, reduced through whole coordinates and a signed-short result.

On a destination event with the target ahead it builds an advancing AirManeuverOrder starting 30 max-velocity steps forward, stepping at maxvelocity, and waits 60..89 ticks. The altitude setter on that maneuver is a no-op. Without a destination event and with pursuit count below 90, ahead resets the count; behind adds 45. At separation over 160 world units it predicts target position 45 target-velocity steps ahead and creates a maneuver stepping by target velocity plus half target maxvelocity along target heading. It then waits 45 with 0x100e8. This assumes a usable target motion component.

An expired/unproductive pursuit clears its goal, prepends VTOL_EVADE, resets count/mask and restarts after the temporary action. Evade (`0x413bc0`) requires a target and no 0x10008, chooses a random side and first flies one primary weapon range perpendicular to current heading, then twice that range, both with radius 128. Its third state completes. Evasion is a temporary geometric maneuver, not a guaranteed dodge or a missile-interception policy.

## 13. Hover attack

AirToGroundHover (`0x413470`) begins with the same randomized half-distance point, then claims slot 0 and approaches the linked target within primary range. It tracks failed reach samples. Two misses allocate a random range-distance destination with radius 128 and add 0x110e8, but do not attach the allocated object. This is a second observed unowned destination/missing-goal-update branch.

Otherwise it alternates angles plus/minus 0x2000 around the target direction, computes a point two thirds of range away and builds a linked-unit-plus-fixed-position PathOrder. That constructor retains target association while suppressing live position refresh. It sets radius 16 and cruise altitude, attaches it and watches 0x100e8. Low health can prepend landing and reset. Because the fixed point is recalculated on later callbacks rather than continuously following position, target association does not make this a perfect live orbit.

## 14. Airborne building

VtolMobileBuild (`0x413d80`) handles event 2 as completion and event 8 as terminated construction, refreshing selection in both cases. Initialization claims weapons, detaches, activates and conditionally climbs. It snaps the requested product footprint, initializes retries and approaches within the builder's build range.

Path failure ends it. Blocked placement announces once, retries every 30 and fails after more than ten prior retries. It snaps full placement and creates an unfinished unit. Unlike ground MobileBuild, allocation failure ends with 8 instead of waiting 300. Success assigns GetBuilt, starts relative-heading build animation and refreshes selection.

State 3 calls WaitIfNotInBuildStance but ignores its return and falls through into work. Its wait helper can still mutate masks/deadlines, but there is no return-based gate before that first work pass. Every global tick divisible by 150, when this state executes, it replaces movement with an offset point at build range and a heading shifted by 0xdb6e. This is a global-modulo check, not a private five-second timer guaranteed to run exactly on schedule.

Work uses integer buildRate / 30 and requests six forward nano particles only on successful AddBuildProgress. It waits one tick with 0x0a while unfinished, then advances to completion speech. It has no factory refund or attachment-to-build-piece production behavior.

[VtolHelpBuild (`0x414380`)](../../../src/orders/vtol_orders_414380.cpp) requires a live target, no event 8, motion/canfly and a nonnull +0x156 builder-list pointer. The provisional canBuild integer here is that list pointer from definition loading, not a separate boolean FBI flag. It prepares climb and approaches target within build range. Path failure ends it and an already completed target finishes immediately.

It starts building with the absolute heading-to-target value, without the relative-heading subtraction used in several other handlers. Its productive state has the same 150-tick modulo offset and conditional particles as mobile air construction, but no dedicated WaitIfNotInBuildStance stage. Cancellation bit 2 refreshes selection and completes; missing target/event 8 reports hostile termination. It ends directly when buildLeft reaches zero.

## 15. Air reclaim

[VtolReclaim (`0x414770`)](../../../src/orders/vtol_orders_414770.cpp) re-resolves a reclaimable feature on every call, requires motion/canfly/canreclamate and prepares climb. It flies to the order position and fails on path failure. Duration is integer `30 + 0.5 * (feature energy + metal)`, compared with the ground handler's base 15.

Every two ticks it subtracts two. While remaining time exceeds 30 it requests reverse nano particles twice, six each, from feature bounds to nano piece. It keeps work deadline tick plus 300. Expiry calls ReclaimFeature once. There is no build-stance wait or worker-rate scaling in this body.

[VtolReclaimUnit (`0x414a80`)](../../../src/orders/vtol_orders_414a80.cpp) requires canfly, canreclamate and CanReclaim, and ends on missing target/0x10008. It computes a 15-tick damage pulse, resets duration and attaches a target-position objective. Work requires horizontal distance within **build range only**, without the ground handler's added target radius. It rechecks CanReclaim, applies kind-5 pulses when duration reaches at least 15, requests six reverse nano particles and increments duration by two per two-tick interval. Out of range it waits 30 and resets. Path failure returns 9. It does not set the same workTime extension as the feature reclaim or ground live-unit loop.

## 16. Air repair and waiting for repair

VtolRepairUnit (`0x414e70`) fails on missing target, exceeds optional starting leash with completion, and requires target mode 1. It refreshes cached target position, verifies CanRepair during initialization and prepares climb. The approach uses cruise altitude and 0xe8. Path failure ends it. A moving target with runtime 0x0c causes a 15-tick wait and restart. Full health advances to completion.

Each productive pass calls AddRepairProgress with integer workerTime / 30 but **ignores its return** and requests six nano particles regardless. It waits one tick watching event 8. Visible particles therefore do not prove funded repair on that pass. This loop does not set the same workTime extension found in ground repair.

VtolGetRepaired (`0x415250`) only waits: it requires its linked host, checks the executing unit's health, waits 30 ticks and sets its started slot to 1 until full health, then announces completion. It does not debit resources, add health or create particles itself. Pad SelfRepair performs actual work through the common repair handler where selected by landing.

## 17. Air repair patrol and resource recovery

[VtolRepairPatrol (`0x4152f0`)](../../../src/orders/vtol_orders_4152f0.cpp) resets after 30 ticks on 0x48. Initialization requires motion/canfly and flags2 0x200. The loader derives that bit from canreclamate while setting canresurrect; it should not be relabeled a standalone repair capability. It remembers linked target position, ensures a return node and prepares climb.

Its recurring state rotates on 0xe0, otherwise flies to the patrol position at cruise altitude, waits 45 and first considers low-health landing. At least 20 percent energy allows a definition-range scan. RepairableUnitVisitor (`0x4158d0`) excludes self, nonallies, modes other than 1, completed full-health units and this owner's last kind-5 reclaim victims. A random candidate then needs CanRepair; completed targets go through IssueRepairOrder while unfinished targets receive VTOL_HELPBUILD.

It also searches reclaimable features within **240 world units**, independently of whether storage is below 20 percent. Priority is metal if metal-low, energy if energy-low, then metal whose full yield fits, then energy whose full yield fits. This differs from ground repair patrol, whose resource search itself is inside the low-storage condition. Accepted temporary work clears movement/mask and prepends the chosen VTOL service action.

## 18. Helpers, ownership and technical limits

AddVec3 (`0x40f790`) adds all three components. DirectionFromAngle (`0x4103a0`) builds negative trig X/Z and zero Y from an angle and fixed scale. CellToWorldPos (`0x414350`) converts footprint-relative cells into X/Z using `(footprint + 2 * cell) * 2^19`, leaving Y untouched. LandingPadList construction (`0x410830`) clears its vector pointers but copies an uninitialized local allocator byte; this does not establish any pad reservation state.

Aircraft action state persists through the generic order record, linked references, destination serialization and unit/COB components. These handlers do not own a separate aircraft save format. Attachment, creation and script-call helpers supply multiplayer events; the action's target geometry and remote motion horizon are serialized in their own modules. Registration supplies overlays, while wake/thrust/smoke effects originate from [script/motion callbacks](units-unit-script-calls.md), not from every aircraft order individually.

All 30 entry points, shared inline behavioral helpers and all 22 registered aircraft actions are covered. Observed limits include unowned pickup/hover destinations, post-attachment pickup guard behavior, unload speech bypass, air-builder stance fallthrough, absolute assistance heading, unconditional repair particles, host selection without pad reservation and differing resource-scavenge conditions. Actual piece choices, animations, weapon eligibility and effects require the loaded assets and downstream modules. This document uses static source inspection without builds or game execution.
