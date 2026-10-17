# Command-mode resolution and unit interaction priorities

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `orders/order_dispatch`, one annotated entry point, GetOrderType at `0x43f0e0` in [order_dispatch.cpp](../../../src/orders/order_dispatch.cpp). The file also physically contains cursor-selection code assigned to the preceding unit_scripts address range. This writeup covers executable order-type resolution and its inline helpers, not every cursor glyph or action handler.

## 1. Inputs and result

GetOrderType accepts command mode, acting unit, optional target unit and optional fixed-point position. It returns a MissionType resolved from the registered name or zero when no branch permits an action. It does not insert an order, reserve a build site, spend resources, acquire a path, attach cargo or fire a projectile.

A supplied nonlive target immediately yields zero before mode dispatch. Otherwise friendliness is read from the acting player's alliance entry for the target's player index. Nonfriendly becomes enemy. The predicate is directional; it does not independently require a reciprocal alliance or derive alliance solely from shared color/side.

The helper Pick chooses VTOL versus ground names from definition canfly, bit 0x800. Motion presence separately selects some queued/static variants. Neither condition alone establishes every downstream action permission.

## 2. Mode overview

| Mode | Main request | Principal selection |
| ---: | --- | --- |
| 1 | Context-sensitive default | Interface-dependent priority sequence |
| 2 | Move/context interaction | QMOVE, movement or capture/reclaim/service/pickup/follow |
| 3 | Attack | Surface, chase/static, aircraft or kamikaze variants |
| 4 | Special attack | ATTACKSPECIAL when candgun |
| 5 | Unload/landing | VTOL_LANDING or transport-specific unload |
| 6 | Pickup | Ground/VTOL pickup after CanLoad |
| 7 | Follow/guard | Ground/VTOL follow for friendly target and canguard |
| 8 | Repair/assist | Repair or help-build after CanRepair |
| 9 | Patrol | Queued, repair-patrol or ordinary ground/VTOL patrol |
| 10 | Stop | STOP |
| 11 | Teleport | TELEPORT |
| 12 | Reclaim/resurrect | Feature resurrection/reclaim or live-unit reclaim |
| 13 | Capture | CAPTURE for another owner's target and capability |
| 14 | Mobile build | Ground/VTOL mobile build when build list and motion exist |

Unknown modes return zero. Even unconditional name choices still depend on registry initialization and successful name resolution.

## 3. Explicit attack resolution

Mode 3 first requires canattack in flags2. Runtime bit 31 admits the weapon-bearing branch. With no enemy target, an anti-air primary weapon rejects the attack. A nonflying actor chooses SUPPRESS; flying actors choose AIRSTRIKE for a dropped primary weapon, otherwise AIRTOGROUND.

Against an enemy, a primary to-air weapon rejects a target outside runtime mode 2. A target whose whole world Y plus model-top height is below sea level requires a water primary weapon or an enabled water secondary weapon. A target top at or above sea level receives an additional hover-actor test: a water primary rejects it, and an enabled water secondary can also cause a zero result in this branch.

The equality boundary matters: below-sea and at/above-sea are separate tests using the target's model top, not just its origin. The secondary-water checks are explicit branch conditions, not a general search across all three weapons for any possible firing solution.

For flying actors, dropped primary weapons against nonflying targets choose AIRSTRIKE. Nondropping attacks against flying definitions choose AIRTOAIR. Nonflying targets otherwise choose AIRTOGROUND or AIRTOGROUNDHOVER according to hoverattack. Some combinations fall through without an aircraft order.

A nonflying actor with motion chooses ATTACK_CHASE. Without motion, the building/immobile-associated runtime flag permits ATTACK_NOMOVE. If the main weapon-bearing branch did not already return or reject to the final zero path, kamikaze capability can select ATTACK_KAMIKAZE according to the remaining control flow.

These are command-selection gates. Range, fire orders, sensor contact, cloak, weapon-category filters, COB aim completion, reload, ammunition and projectile collision still constrain actual shooting. The [naval reference](../../naval-systems.md) follows those later gates; selecting ATTACK_CHASE is not proof a particular gun can hit the target.

## 4. Service and transport modes

Mode 8 delegates to [CanRepair](units-unit-targets.md). A qualifying unfinished target chooses VTOL_HELPBUILD or HELPBUILD; a completed target chooses VTOL_REPAIRUNIT or REPAIRUNIT. This explicit mode does not add a separate friendly test in the dispatcher, so command context and later handler policy remain relevant.

Mode 7 requires canguard and a friendly target, then selects VTOL_FOLLOW or FOLLOW_GROUND. Mode 6 requires a target and CanLoad before selecting pickup. The transport predicate checks capacity, target motion/size/build state and water constraints; attachment happens later.

Mode 5 first allows a flying load-capable actor targeting an airbase to choose VTOL_LANDING. Otherwise canload chooses VTOL_UNLOAD or GROUND_UNLOAD. No target is required for that latter selection. Landing and unloading share this input mode but have different resulting handlers and approach/piece policies.

Mode 13 requires cancapture, a target and different owning player pointers. It does not require the alliance entry to classify the target as enemy. Mode 14 requires a nonnull buildable-ID list and motion, then chooses the corresponding mobile-build handler. It does not validate a particular requested build ID or site because those are supplied/checked in later order construction and execution.

Mode 4 checks candgun and selects ATTACKSPECIAL without charging its shot costs. Mode 10 selects STOP and mode 11 TELEPORT without an additional capability test in this function. Interface exposure or execution restrictions elsewhere must not be attributed to nonexistent checks here.

## 5. Patrol variants

Mode 9 requires canpatrol. Without motion it returns QPATROL. With motion and the repair-capability bit it chooses REPAIRPATROL or VTOL_REPAIRPATROL; otherwise it selects PATROL or VTOL_PATROL.

That repair-capability bit is flags2 bit 9, which the [FBI loader](units-unit-types.md) derives from canreclamate in this revision. It should not be inferred solely from an informal builder/unit class name. The chosen handler later determines when an available service target interrupts patrol and how patrol resumes.

## 6. Feature discovery, visibility and resurrection

Inline Lookup maps a position to a terrain cell, resolves continuation feature markers to their origin and returns a definition for eligible IDs. Direct IDs are checked against feature count; the alias branch has a different high-sentinel check. Empty/reserved values do not produce an ordinary feature definition.

The visibility helper projects whole X and height-adjusted Z into a 32-world-unit grid, bounds-checks using the actor player's grid dimensions and tests the global viewing-player bit in the shared visibility mask. It does not simply assume the position is known because the map has an explored byte. This dependency is relevant when commanding through an observer/shared-information context.

Feature selection requires reclaimable flag 0x80 in addition to position visibility and the appropriate actor capability. Resurrection and feature reclaim therefore depend on asset flags and current information, rather than merely detecting a visually recognizable wreck.

Mode 12 first requires canreclamate. It performs Lookup before testing whether position is null. The source identifies the resulting null-position dereference as a suspected original bug, even though later branches allow a target-unit fallback. Valid callers must supply the expected position contract on this path.

Given a position, canresurrect and a qualifying visible feature select RESURRECT before ordinary feature reclaim. Otherwise qualifying features choose VTOL_RECLAIM or RECLAIM. If no feature choice returns and a target exists, it chooses live-unit reclaim. This final branch does not itself repeat CanReclaim; later command/handler checks must not be conflated with this name selection.

## 7. Move mode prioritizes interactions

Mode 2 requires canmove. Without motion it returns QMOVE. With motion and no target it returns ordinary ground/VTOL movement. With a target it evaluates this order:

1. Capture an enemy when cancapture is set.
2. Reclaim an enemy when canreclamate is set.
3. Assist a friendly unfinished repair-eligible target.
4. Repair a friendly target below maximum health when eligible.
5. Land a flying actor at a friendly airbase.
6. Pick up a target passing CanLoad.
7. Follow a friendly target when canguard is set.
8. Otherwise choose ordinary movement.

Capture takes priority over reclaim in this mode. Pickup is not preceded by a separate friendly test in this sequence. A requested move toward a unit can thus become a service/ownership/transport order, while moving to an empty coordinate uses a different branch.

## 8. Default/context mode and interface variants

For interfaceType 1, mode 1 prefers attack on an enemy when canattack is set. It then considers enemy live-unit reclaim, friendly help-build, friendly repair, friendly airbase landing, pickup, friendly follow, visible feature resurrection, visible feature reclaim and finally movement. A failed attack resolution returns its result directly instead of automatically trying every later fallback.

For the other interface variant, mode 1 first redirects enemy attack and enemy reclaim through modes 3 and 12. It then considers an unfinished repair-eligible target without a separate friendly test. A selectable local completed unit meeting readiness/transfer/carrier conditions causes no order to be selected, preserving the interface's selection behavior. Feature resurrection and reclaim precede movement, which requires canmove plus motion.

The two variants do not apply identical interaction priority or friendly guards. They should not be collapsed into one generic right-click rule. The physically adjacent GetOrderCursor routine returns cursor codes with related but separate tests; cursor appearance alone is not the selected mission type or a guarantee of execution.

## 9. Completeness and boundaries

The single annotated dispatcher entry and all its command cases are covered. It owns no persistent queue record or transport packet. [Order lists](orders-order-list.md) insert, schedule and destroy the returned action; [unit ownership commands](units-unit-commands.md) carry out capture/gift reconstruction; action handlers perform work and movement; weapons determine firing.

For an unexpected contextual action, inspect mode and interfaceType before handler state. For a repair/pickup rejected underwater, inspect the service predicate's height/depth test before pathfinding. For a listed attack that never fires, continue through weapon eligibility and aim/cost gates. For a wreck ignored by a default command, inspect feature visibility, reclaimability and the higher-priority context branches first.
