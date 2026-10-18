# Order instances, queue policy, execution and persistence

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `orders/order_list`, `0x439b30` through the boundary before `0x43cd20`. Its 49 annotated entry points are in [order_list.cpp](../../../src/orders/order_list.cpp) and the destructor/save methods in [order_queue_4384a0.cpp](../../../src/orders/order_queue_4384a0.cpp). Later motion/script functions in the first file belong to the following unit_scripts module. Addresses, rather than physical file boundaries, define this writeup's scope.

## 1. Record layout and two lists

Each unit has a primary list at +0x5c and secondary list at +0x60. An [Order](../../../src/orders/order.h) is 0x56 bytes: type/state, event-wait mask, wake tick, owning unit, linked target reference, world position, cached points, handler-specific integers, persistent flags, creation tick, next pointer, pending events and an optional polymorphic goal.

The +0x36/+0x3a/+0x3e fields have different meanings in different handlers, including build ID/count/progress, durations, radii, retries and pieces. Their shared storage does not mean that every order has a build-progress fraction. Type default flag 0x40000 selects the secondary list; ordinary insertion/deletion uses that classification.

The order's owning unit and its target reference are separate. The constructor's unit argument initializes the linked target; insertion later supplies the owning unit. Confusing the two reverses guard/attack/build relationships and breaks save ownership interpretation.

## 2. Type registry and support functions

The global registry contains 0x19-byte entries with status text, notify callback, draw mask, default flags and name. `RegisterOrderTypes`, `0x43bc90`, appends records and sorts case-insensitively by name. `RegisterAllOrderTypes`, `0x43c050`, registers Ready, ground orders, VTOL orders, AI commands and general unit orders.

Type bytes are indices into the resulting sorted table. Registration/sorting must finish before those indices are relied on as stable live identities. Names are looked up through binary search in the [helper module](orders-order-queue.md).

CompareOrderTypeNames (`0x43c020`), OrderTypeNameLess (`0x43a940`) and LowerBoundOrderTypes (`0x43c6b0`) supply comparison/search. The sort helpers at 0x43c720, 0x43c940, 0x43c990, 0x43ca70 and 0x43cb20 implement quicksort partition/median and insertion completion. Container insert/destroy at 0x43c3a0/0x43c390 handle trivial registry records. The source records a suspected original pair of compensating stack-cleanup discrepancies in the out-of-line median/partition helpers; these do not create a different intended gameplay order policy.

`ClearOrderTypeTable`, `0x43c350`, moves the exposed end to the beginning without a complete backing-allocation teardown in that body. `MissionOrderTable::GetCount`, `0x43c360`, returns zero for a null beginning, otherwise byte span divided by 25. The empty OrderBase notification at 0x43a1e0 is overridden by the order's event-OR method, not a second executor.

## 3. Constructing and inserting an instance

The constructor, `0x43a0c0`, copies type defaults, initializes linked target and listener, optional position, zero caches, caller-specific fields and creation tick. It starts state/event masks at zero, wake tick at 0xffffffff, owner/next/goal pointers null. Missing target clears flag 0x200; missing position clears 0x400. Without target flag 0x200 it unlinks any initially registered target.

`InsertOrderBefore`, `0x43ac60`, searches the selected list for the supplied before node, links the new node, records its owner and inherits following flag 0x4000. It assumes before belongs to that list, or is null for its end.

`AppendOrder`, `0x43acb0`, despite its name inserts before the selected head. `AppendOrderToTail`, `0x43ad10`, is the actual tail operation. `InsertOrderAfterMarked`, `0x43ad50`, inserts after the primary node marked 0x1000, moves that marker to the new node, or appends when no marker exists. Insertion order is therefore policy-dependent, not universally FIFO.

`EnsurePatrolReturnOrder`, `0x43a020`, ensures a return-position order exists when none has flag 0x8000, appending a same-kind node at the unit's current position, then marks the supplied patrol item. `EnqueueOrderType`, `0x43b730`, creates a type-only order marked 0x4000 and inserts at its classified head. That marker distinguishes automatic/default nodes from ordinary requested work.

## 4. Adding, replacing and canceling commands

`AddOrder`, `0x43adc0`, constructs an instance from requested type, target, position and arguments. With removal/queue control zero and a type lacking flag 0x40, it prunes primary nodes lacking preservation flag 4. For a new primary order it also removes the leading run of automatic nodes marked 0x4000.

It marks the new order with flag 1, and with one-shot announcement flag 0x2000 when control is zero. A new node with flag 0x20 or 0x40000 goes to the selected head; otherwise it follows the primary 0x1000 insertion marker. Thus the control argument named remove is not itself a generic delete-all boolean: zero participates in replacement policy, while type flags still preserve or choose different queue behavior.

`IssueOrCancelOrder`, `0x43afc0`, with nonzero control searches primary orders for the same type and, when supplied, the same target and nearby X/Z. The position match permits a difference of 0x100000, sixteen world units, independently on each horizontal axis. It deletes the first match and returns. No match falls through to AddOrder. The secondary list is not independently searched by this toggle helper.

`AdjustBuildCount`, `0x43b0b0`, chooses a list by type classification. A positive amount merges only with the current tail when kind and build ID both match, otherwise adds a new order. Nonpositive adjustment repeatedly finds the last matching order, reduces it when sufficient, or deletes it and carries the remaining subtraction backward through earlier matching nodes. Queue count edits are distinct from resources or construction already committed to an active target.

## 5. Deletion and cleanup effects

`DeleteOrders`, `0x439eb0`, with all false deletes primary orders lacking preservation flag 4. With all true it deletes all primary and secondary nodes. `DeleteOrder`, `0x439f80`, searches the classified list for a specific node and does nothing if absent. Deleted nodes other than the original primary head receive flag 0x10000 before destruction.

The destructor, `0x43a1f0`, can notify the type callback with event 2 when its event mask includes that bit. It stops marked build animation and sends StopBuilding, detaches/deletes its movement goal under the motion-interface relationship, releases all weapon control unless 0x10000 suppresses that operation, and unlinks its target reference.

Flag 0x10000 therefore controls destructor weapon-release behavior. It is not merely an arbitrary deleted-node label. Deleting a queued nonhead must not necessarily release control held by the current head. Motion/goal cleanup also assumes the instance has acquired an owning unit before destruction; partially initialized or never-inserted objects are not interchangeable with valid queued nodes.

`MoveOrderToTail`, `0x439fe0`, removes a known primary node and relinks it at that list's end without destroying it. Its membership search assumes the node exists. Callback result 6 uses this for repeating/patrol-style sequencing.

## 6. Primary executor and event waits

`SetDeadlineTicks`, `0x439e80`, enables wait bit 1 and sets wake tick to current game tick plus the requested interval. `RunOrders`, `0x43b7c0`, rereads the primary head after each callback. A due deadline sets pending bit 1 and resets wake tick to 0xffffffff.

Pending input is the union of the order's subFlags and the unit's network-dirty bits, intersected with the order's wait mask. A nonzero wait mask without matching input returns from the executor. Otherwise consumed bits clear in both sources and the wait mask clears before invoking the type callback. Pending 0x10000 also clears all three weapon targets.

The callback result controls the next step:

| Result | Primary executor behavior |
| ---: | --- |
| 0 | Reset state byte to zero |
| 1 | Increment state byte |
| 2 or 4 | Keep node/state |
| 3 | Wait current tick plus 30 and a random amount below 15 |
| 5 or 8 | Delete this node |
| 6 | Move this primary node to the tail |
| 7 or unrecognized | Delete all orders and return |
| 9 | Set flag 0x800000; delete if another follows, otherwise reset state and wait 30 plus random below 30 |

Results that keep a node do not themselves force a return. The head is reread and may run again in the same pass. A well-behaved handler must install its next wait condition, change state toward a waiting/terminal path, or alter list membership. This loop has no independent fixed callback-count budget.

When the primary list becomes empty, active local controller roles 1/2 can receive the definition's nonzero defaultmissiontype as an automatic marked order. Insertion happens after the processing loop; the new default is not immediately executed through that loop in the same call.

## 7. Secondary executor differs

`RunSecondaryOrders`, `0x43bad0`, visits secondary nodes. A node runs when its wait mask is zero or its wake tick is due. It clears the mask and calls the callback with event input zero, rather than merging unit dirty/subFlags through the primary algorithm.

Results 0/1 reset/increment state, 2/4 retain, and 3 schedules 30 plus random below 15 ticks. Results 5/8/9 delete the node. Results 6/7 delete it and return from the executor, rather than rotating the primary queue or deleting all orders. Unrecognized results delete the current secondary node.

After an executed callback it restarts at the secondary head. A still-waiting node is skipped to its successor, allowing another secondary action to run. These semantics explain why a callback cannot safely assume the same return code means the same queue operation in both lists.

`ReadyOrder`, `0x439ea0`, simply returns 5, supplying a trivial self-completing callback.

## 8. Automatic attack and repair orders

`IssueAttackOrder`, `0x43b1f0`, rejects self-target and, unless overridden, requires nonzero standing move and fire settings. It asks GetOrderType for attack mode 3 and rejects type zero. In its limited-movement setting it inserts a return-position movement node and a target attack node with maneuver leash, storing the starting X/Z. Other accepted cases insert a direct target order. The override bypasses the initial stance restrictions.

`IssueRepairOrder`, `0x43b400`, resolves mode 8 first, then branches on standing move state. Without override, states zero and one construct return-position plus repair nodes with different range/leash parameters; state two inserts direct repair. Other cases return zero. A nonzero third argument does not act as the same general force-success override used by the attack helper.

`FindBestTargetIfFireAtWill`, `0x43b700`, invokes weapon target search only when standing-fire mode equals 2. Neither this lookup nor inserting an attack order bypasses weapon/sensor/aim constraints. The dispatcher and combat handlers still decide what can be shot.

## 9. Queries, progress and order overlays

`GetOrderTarget`, `0x439dd0`, returns the primary head's target or zero. `GetOrderTargetIfFlagged`, `0x439cf0`, requires primary flag 8 before returning that pointer. `GetOrderName`, `0x439df0`, returns primary type status text or type-zero status text when absent. `FindOrderByType`, `0x439e30`, searches the list selected by that type's default classification, returning the first match.

`SumQueuedBuildCount`, `0x439d80`, sums matching ID counts on both lists where flag 0x100 is set. `GetBuildWeaponPercent`, `0x439d20`, finds the first secondary node marked 0x80000 and divides its progress times 100 by the selected weapon's +0xe4 timing field. The body does not independently guard a zero denominator. This percentage is not generic unit buildLeft.

`DrawOrderOverlays`, `0x439b30`, walks primary nodes and intersects caller mask with each type's draw mask. Bits choose build footprints, path animation, order range ring, weapon coverage and unit range rings. The last category is drawn only once. Helpers receive a running position seeded by the unit position and updated across orders, connecting queued destinations visually.

Overlay eligibility and position routing are defined here; exact marker pixels belong to ingame/markers. Model construction wireframes and shipyard-built hull clipping belong to the renderer, not this overlay loop. A visible queued footprint does not mean a live construction target already exists.

## 10. Order save contract

`SerializeToSave`, `0x43a970`, requires matching owner ID, file/name and a name of at most 31 characters. It writes a raw 0x3a-byte descriptor containing owner/target IDs, goal type, type/state, wait/deadline, position/caches, handler integers and persistent/pending flags. Only a live linked target supplies a nonzero target ID.

It also writes name plus _name as the order type's string. For specific MobileBuild, VTOL_MobileBuild and BuildingBuild names it records a UTYPENAME key for the referenced build type. An attached goal serializes under name plus g. The writer checks IDs before some optional-pointer tests, so arbitrary null owners are not a supported general input.

`WriteUnitTypeNameKey`, `0x43a2d0`, supplies the same missing-key write for valid type IDs. Keys use decimal width four, including padding spaces. `ResolveUnitTypeKey`, `0x43a360`, resolves a present name through current definitions; without one it uses a legacy ordinal mapping over definitions not marked downloadable and returns the corresponding table-position-minus-one value. This fallback is not ordinary identity mapping of all modern IDs.

## 11. Restoring an order and its goal

The save constructor, `0x43a420`, initializes reference/list/goal basics, validates file/name/length and requires a full descriptor read. It resolves kind by stored type-name string when present, otherwise a legacy ordinal scan skipping registry entries with the relevant high flag. It requires descriptor owner ID to identify the supplied unit and kind to be nonzero.

It recursively loads the target unit through [LoadUnit](units-unit-save.md), restores state/wait/handler fields and remaps build IDs for the named build kinds. Goal types 2..6 create the corresponding saved polymorphic geometry/path objects; 0/1/unknown leave no goal. The goal constructors and record meaning belong to order_targets. Creation tick is initialized to the current load tick rather than restored from a separate descriptor field.

Early validation exits leave an object that has not completed normal field population; this constructor does not return a separate success status for the caller to test. The surrounding loader must therefore provide consistent records. It is not a comprehensive validator for malformed save content or arbitrary unit/goal IDs.

## 12. Adjacent movement helpers in this module

`UnitMotion::ApplyClampedTurnDelta`, `0x43cbb0`, limits a nonzero requested heading change to the definition's turn-rate bound, adds the clamped turn and marks the unit moved. A zero request clears stored turn without changing heading.

`UpdateVelocityFromHeading`, `0x43cc20`, adds a speed change, clamps below zero, chooses a pitch-dependent percentage factor using pitch shifted by eleven and limited to minus five through five, and caps speed against that factor times definition maximum velocity. Below sea level it halves the cap unless canhover or floater is set. It converts resulting heading/speed into horizontal velocity with zero Y.

These two helpers are inside order_list's address range despite operating on UnitMotion. Ground steering, flight behavior, obstruction handling and final position updates continue in the next unit_scripts module. Amphibious permission alone is not the particular underwater speed-cap exemption tested here.

## 13. Completeness and diagnostic use

All 49 annotated entry points in the range are covered, including registry/container support, save methods and adjacent motion helpers. Runtime effects come from the callbacks registered by the individual order modules. The queue provides sequencing, state/events, cleanup and persistence; it does not silently supply every action's range/resource/target eligibility.

For an order that repeats in one tick, inspect callback return plus the next installed mask. For an unexpected replacement, inspect control zero, preservation flag 4 and the 0x1000 insertion marker. For a target disappearing on cancellation, inspect head-versus-queued destructor weapon release. For a restored action of the wrong type, inspect name remapping versus legacy indices. These mechanisms explain the command lifecycle without reproducing the handler source listings.
