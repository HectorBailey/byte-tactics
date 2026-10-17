# Order helper contracts, script waits and movement goals

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `orders/order_queue`, eighteen annotated functions from `0x4384a0` through `0x438be0` in [order_queue_4384a0.cpp](../../../src/orders/order_queue_4384a0.cpp). Later constructors, destruction and save methods physically retained in this file belong to `orders/order_list`. This module supplies carried-state orders, build-animation coordination, script wait gates, order-type lookup, status flags and movement-goal attachment.

## 1. Orders and types are separate state

An [Order](../../../src/orders/order.h) is a 0x56-byte instance with type byte, state byte, wake/event mask, owner, linked target reference, position, handler-specific fields, persistent flags, next pointer, pending status and optional polymorphic goal. [MissionType](../../../src/orders/mission_type.h) is the one-byte type identifier used to select the registry entry.

`MissionType` name construction, `0x438760`, binary-searches the sorted registry case-insensitively. A match stores its registry index narrowed to a byte; a missing name stores zero. It does not register a new name or throw an error. Type zero must be understood through the initialized registry rather than assumed to mean a new dynamically generated handler.

`OrderType::GetTableEntry`, `0x438830`, and `GetTableEntryByTypeByte`, `0x438850`, calculate registry base plus type times 0x19 bytes. These accessors rely on an initialized table and valid type value; they do not independently range-check it. Handler callbacks, defaults and type flags live in that registry, while per-unit progress lives in Order.

## 2. Installing the carried-state order

`AddBeCarriedOrder`, `0x4384a0`, acts only when the unit has a carrier. It walks primary orders and deletes those without persistent flag 4, retaining flagged nodes. Before deleting a nonoriginal-head node it adds flag 0x10000, preserving the cleanup distinction used by the order-list module.

It constructs BECARRIED targeting the carrier and inserts it at the head of the primary or secondary list chosen by the new order's flag 0x40000. Insertion records the owning unit, connects the previous head and inherits flag 0x4000 from that following order when present. This is an order-state transition accompanying attachment, not the physical act of linking cargo to a model piece.

The [attachment handler](units-unit-position.md) invokes this service under its ownership/target guards. Existing preserved orders and the carried-state order can consequently coexist; cargo does not imply every queued command was blindly discarded.

## 3. Starting and stopping build animation

`StartBuildingScript`, `0x438590`, finds StartBuilding in the unit's COB interface, starts it with the supplied unsigned heading argument, sends the same script call over the script-call event path and marks order flag 0x400000.

`StopBuildingScript`, `0x4385f0`, performs StopBuilding only when that order flag is set. It starts the no-argument script call, sends the no-argument network event and clears the flag. Repeating stop without a corresponding marked start does not repeatedly invoke the script through this helper.

The flag belongs to the order, coordinating cleanup when work ends or the order is destroyed. Starting the animation does not itself create a construction target, debit resources or guarantee that the script has reached build stance. The actual COB assets determine piece motion and stance signaling.

## 4. COB readiness gates

`WaitIfNotInBuildStance`, `0x438700`, tests bit 0 in the unit's COB state byte at +0x10f. When absent, it replaces the order's wait mask with the supplied event mask OR 4 and returns scheduler result 2. When present, it returns 1.

`WaitIfCobBusy`, `0x438730`, uses the opposite readiness sense for COB bit 1: when that busy bit is set it installs the same mask-plus-4 wait and returns 2; otherwise it returns 1. These gates expose script readiness to the order state machine without blocking the application thread.

A script that never supplies the expected state can leave the work handler waiting even while a construction model or nanolathe effect is visible. Event mask, pending status and callback return handling belong to the order executor, so elapsed wall time alone does not prove this gate advanced.

## 5. Reclaim damage pulse

`ComputeReclaimDamagePulse`, `0x438650`, derives a live-unit reclamation pulse from actor worker time, integer experience multiplier (kills plus five divided by five), target maximum health and caller's pulse factor. The denominator is maximum of target metal cost and 10, multiplied by 300. The result is truncated to integer and has minimum 1.

Thus more experienced workers can produce a larger pulse, and the same worker factor scales against both target health and metal cost. This computes damage progress, not the final refunded metal. [Unit death/reclaim](units-units.md) credits completed fraction times cost on the corresponding kill path.

The numerator is formed as a 32-bit multiplication chain placed in the low half of a 64-bit value with high half zero. Very large or unexpected inputs therefore do not behave like unrestricted exact signed arithmetic. The body applies no independent cap corresponding to remaining target health; the caller/damage path handles that result.

## 6. Status delivery and speech

`Order::OrStatusFlags`, `0x438870`, ORs events into subFlags without replacing existing pending events. Listeners can use it to wake or inform an order. It does not immediately invoke the handler.

`AnnounceStatusIfFlagged`, `0x438880`, checks persistent flag 0x2000, clears it and queues unit speech of kind 5 with the supplied text. This is a one-shot announcement gate for that flag, not an unconditional error message on every callback.

`GetOrderFlags`, `0x438be0`, returns zero for a null unit or absent primary head, otherwise that head's persistent +0x42 flags. It does not merge all queued flags or inspect the secondary list. An apparent missing flag can simply belong to a later/secondary order.

## 7. Goal attachment and ownership

`SetAttachedFx`, `0x4388d0`, operates when the owner has motion. It detaches a previous goal from the motion interface, deletes the goal and clears the pointer. Installing a new one clears pending event bits 0x3e0, attaches through the motion interface and stores it on the order. The order owns the goal object's lifetime.

`ReattachFxToUnit`, `0x4388b0`, attaches an existing goal back through the owner's motion-interface chain. It is used after reconstruction, without creating a second goal. It checks that the goal exists but relies on the required motion/interface pointers being available.

The three geometry helpers are:

| Helper | Goal constructed |
| --- | --- |
| AttachApproachRadiusGoal, 0x438930 | A radius region around world X/Z |
| AttachRingApproachGoal, 0x438a00 | An inner/outer-radius region |
| AttachBuildFootprintMarker, 0x438ad0 | A cell-origin/size rectangle |

For nonflying owners they create the selected goal, remove an existing one and install it through the motion interface, clearing the same event bits. For flying definitions they remove an existing ground-style goal rather than creating this replacement. VTOL orders use their own approach logic.

These helpers expect compatible motion state. Some construct before checking the motion pointer; a missing motion object is not handled as a complete rollback/free of the new argument in every branch. SetAttachedFx also does nothing with the supplied object when no motion exists. Correct callers must observe the definition/runtime contract.

The goal tests and their serialization belong to the order-targets module. Attaching a radius does not guarantee a successful path, and marking a build footprint does not grant build permission or allocate its unit.

## 8. Changing an order's type

`MergeFlagsFromTable`, `0x438b90`, writes a new type byte and replaces persistent flags with that registry entry's defaults while retaining the instance's bits 0x600. It is not a complete reconstruction of the order's state, position, linked target or attached goal.

Changing type therefore carries a specific limited flag-preservation policy, while handlers remain responsible for interpreting/resetting their state fields. Arbitrary registry changes during a live order sequence would violate the index/table assumptions established above.

## 9. Boundaries and completeness

All eighteen entry points are covered. This module sends build-script calls and assists attachment/order execution, but defines no separate HapiBank record format or packet transport implementation. Order creation/deletion, queue policy, callback-result semantics and registry initialization belong to order_list; goal geometry belongs to order_targets; build/reclaim progress belongs to the individual handlers and economy.

For work that stalls, inspect COB stance/busy bits and the order's event mask before changing resource assumptions. For movement that ignores a new radius, inspect flying classification and motion-goal ownership. For repeated TargetCleared/build events, distinguish these marked start/stop transitions from activation changes and weapon-target cleanup in other modules. Each path has its own event and ownership contract.
