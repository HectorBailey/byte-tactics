# Production controls, work helpers and camera services

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `ingame/control_panel`. Sources: [main controls and services](../../../src/ingame/control_panel.cpp), [selection aggregation](../../../src/ingame/control_panel_41b2e0.cpp) and [forward paging](../../../src/ingame/control_panel_41bde0.cpp). Its address interval begins at `0x419560` and ends before `0x41d0f0`. It includes construction/repair implementation and substantial camera behavior, beyond the name's apparent UI scope.

## 1. Command initialization and lookup

InitCommands (`0x419560`) obtains a side-associated value, zeros message byte totals and both directional counters/byte counts for message types 1..44, clears aggregate sent/received packet counters, registers console/cheat/debug tables and installs DefaultCommandHandler with argument 4. It is not an order-table registration routine.

CopyMenuEntryName (`0x4195f0`) copies up to 32 bytes of a unit definition's internal name and forces byte 31 to zero. MenuEntryNameContains (`0x419630`) obtains a gadget name and performs case-sensitive substring matching. Many controls therefore depend on GUI naming conventions rather than a typed command identifier on each gadget. BuildEntryGuiName (`0x41b230`) formats the bounded internal name plus numeric page and .GUI.

## 2. Placement intent and issuing construction

SetOrderIntent (`0x419bc0`) stores mode and clears input flag 8. UpdatePlacementGhostValidity (`0x4197d0`) converts the current world point to a footprint-relative cell, writes the preview rectangle in world-unit coordinates, and sets input bit 6 from CanBuildAt for the local player. Valid placement gets GetBuildSiteHeight; invalid placement uses GetFootprintHeight. Both preview Y bounds receive that height. The return is the validity bit, not a newly allocated unit.

IssueMobileBuildOrders (`0x419670`) snaps the point to the selected product footprint and uses the preview height. It visits selected local units with builder flag 0x40, issuing MobileBuild or VTOL_MobileBuild according to each builder's canfly bit. Key flag bit 2 supplies IssueOrCancelOrder's queue/remove control. Actual per-builder reach, site checks, resource use and unit creation happen in the [action handlers](orders-unit-orders.md) and [aircraft actions](orders-vtol-orders.md).

QueueBuildOrder (`0x419b00`) plays addbuild/subbuild for the locally controlled player. A name containing MAKENUKE or MAKEANTI adjusts BuildWeapon count for ID zero. Other names resolve a product definition; missing IDs are ignored. Presence of the producer's motion pointer selects MobileBuild versus BuildingBuild for count adjustment. This direct helper does not select the VTOL construction variant itself.

## 3. Order buttons and immediate settings

HandleOrderButtonClick (`0x419be0`) checks gadget substrings in order. Nonzero button status selects Move 2, Attack 3, Blast 4, Defend 7, Repair 8, Patrol 9, Reclaim 12, Capture 13, Unload 5 or Load 6; zero status restores contextual mode 1. Stop restores mode 1 and immediately sends Stop to the selection. UNLOAD is checked before LOAD, preventing its substring from choosing the wrong action. Move/Stop/Attack/Blast/Defend/Patrol/Load use immediateorders sound, while Repair/Reclaim/Capture/Unload use specialorders. Unknown names return zero.

The handler initializes its local gadget pointer only when the indexed entry has type 1, yet recognized branches read that pointer. Valid GUI entry type/index is an interface precondition, not comprehensively validated input.

HandleOrdersPanelClick (`0x41a490`) cycles MoveOrd/FireOrd 0 to 1 to 2 to 0, treating mixed state 3 as reset-to-zero. It issues the named standing-setting order and updates the button. On/off 0 activates, 1 deactivates and mixed 2 activates. Cloak toggles based on whether its aggregate field is nonzero. Status/ONOFF and CLOAK dispatch actual orders rather than directly modifying every selected unit. It refreshes buttons and renders layer mask 0x40. Runtime standing-field naming disagreement elsewhere is recorded in the common-action reference; these controls establish the UI's intended setting labels and numeric cycle.

## 4. Selection aggregation and capability display

RefreshSelectionOrders (`0x41b2e0`) sets refresh and either uses an existing focus ID as one unit or scans selected live local units. An invalid focused slot clears the ID and returns. During a scan, fire/move sentinel 4 means unsupported; differing supported values become mixed 3. On/off sentinel 3 means unsupported, mixed becomes 2. Cloak starts unsupported 3, adopts the first supported request, then becomes 2 for every additional cloak-capable unit without comparing equality. Thus a multi-cloaker selection can display mixed even when their requests agree.

Capability flags accumulate with OR: any selected unit can enable move/stop/attack/guard/patrol/load/repair/reclaim/capture/blast. The command dispatcher and each action still test individual eligibility. A lit button does not mean every selected unit can execute it. The local canRepair bit is flags2 bit 9, derived from canreclamate by the definition loader, rather than an independently loaded repair property.

No selection closes back to the base layout. One selected unit with build pages can open its unit-specific page when the page is positive or definition bit 31 requests that path. Otherwise it opens a side-specific generic panel, with or without a unit focus. The focused-ID branch does not rebuild the aggregate fields through the full selected-unit scan in this body, so prior panel state and caller refresh sequence matter.

RefreshOrderButtons (`0x41a120`) reflects build/orders page, cloak/onoff/standing settings and capabilities. Unsupported cloak/onoff value 3 and move/fire value 4 gray their controls. Missing capabilities gray relevant actions. With no load capability it hides LOAD and grays UNLOAD, then grays BLAST if unavailable; with load capability it hides BLAST instead. Transport and special-weapon presentation therefore share a layout policy rather than all appearing independently.

UpdateOnOffButton (`0x419ac0`) copies activation bit 0 to the ONOFF control. MarkSelectionOrdersDirty (`0x41c110`) requests refresh only for a selected local unit. UpdateBuildMenuIfFocusUnit (`0x41c150`) refreshes count texts only when that ID is the displayed focus. GetBuildMenuFocusUnit (`0x41b2a0`) returns a nonzero-ID live slot or null.

## 5. Counts, menus and click routing

SetBuildCountText (`0x419940`) finds a product-named gadget and displays +count, or blank for zero. RefreshBuildCountTexts (`0x4199b0`) walks type-1 entries. Attribute 4 means a named product whose queued count is summed; attribute 8 means primary-slot stockpile followed by queued ID-zero count. It can display stockpile alone, queued count alone or both. This is count display, not construction percentage.

HandleBuildPanelClick (`0x41aa00`) resolves a product from a gadget name truncated to 16 characters. PREV/NEXT/ORDERS/BUILD set deferred page flags. A resolved product with definition field +0x22f equal to zero selects placement mode 14. Other entries try standing-setting handling, then action-button handling, then production count adjustment for a selected focus unit. Key 0xf9 changes the delta from one to five; click mode 1 adds and other click modes subtract. It refreshes counts for factory runtime bit 29 or primary weapon stockpile flag 0x10000000, marks the GUI changed and clears selected gadget at the end.

OpenBuildMenuGui (`0x41ace0`) refuses unfinished focus units. It looks for the requested GUI asset and falls back to side-name plus DL if file length is zero. A loaded layer installs HandleBuildPanelClick. Downloadable build entries matching producer definition ID and page-minus-one populate configured gadget slots plus four, replacing names, attribute and visibility state. It refreshes paging/order buttons/counts, factory ONOFF and later-page availability, renders and records focus IDs. Asset availability, page data and fixed slots determine the displayed build list.

DisableUnavailableBuildMenuEntries (`0x41ac90`) grays unit-name entries whose definition cannot resolve. SetPrevNextGadgetNames (`0x41a920`) disables side PREV/NEXT when page count is below two. OpenGeneratorDialog (`0x41b0f0`) loads the local side's GEN.GUI, installs the same handler, refreshes controls and records the optional unit IDs. GUI load failure leaves that setup incomplete without a new fallback layer in these bodies.

## 6. Paging state and deferred requests

GetBuildMenuPage (`0x41b200`) returns the stored page only when build-page bit 22 is on, otherwise zero. Forward StepBuildMenuPage (`0x41bde0`) supports toggling through orders/build pages or staying within build pages. Toggle from closed opens page 1; at the last page it closes; otherwise increments. Nontoggle wraps last page to 1 and keeps the build menu open.

StepBuildMenuPageBack (`0x41bf10`) performs the reverse using packed three-bit pages. Opening backward chooses pageCount minus one; toggling backward from first page closes. Nontoggle wraps first/earlier values to the last page and keeps build open. Both mark panel dirty for a valid focus and play nextbuildmenu even if no focus exists. Page field width constrains representable page values.

OpenBuildMenuPage (`0x41c060`) accepts a requested page strictly below definition page count, turns build-page off for zero or on otherwise, stores nonzero page, marks dirty and plays sound. It has no explicit negative-page rejection before that comparison and packed assignment.

DispatchOrdersPanelPageFlags (`0x41c180`) handles at most one request per call, prioritizing next, previous, build, then orders. It clears the consumed request; build/orders change bit 22 on a live focus and mark dirty. This deferred routing prevents assuming a click immediately reloads every menu in its callback.

## 7. Construction progress and health

AddBuildProgress (`0x41ba60`) returns zero immediately for already completed buildLeft zero. A nonnegative work amount sets activity bit 0x80 at unit +0xbb even when the amount is zero or later funding fails. Zero then returns without progress. Otherwise next remaining fraction is previous minus amount/buildtime, clamped to 0..1. The actual fraction difference determines proportional energy and metal charges.

Health change is the difference between truncated previous-fraction-times-maxHealth and next-fraction-times-maxHealth. Positive work advances only if the builder's paired resource request accepts both costs. Health caps at maxHealth, remaining fraction updates and runtime bit 0x2000 marks visual/state change. The helper returns one for accepted positive work. A newly zero fraction invokes FinishConstruction.

Negative work increases remaining fraction, lowers health with a floor of zero, sets 0x2000 and credits metal into the **target unit's account**. Active AI type 2 receives half at difficulty 0, 70 percent at difficulty 1, otherwise full; other owners receive full. Energy is not refunded here. Fully unbuilt fraction one receives self-damage 30000 kind 9. The return stays zero for dismantling, so callers must not equate zero with no mutation for negative input.

Build time is an unguarded divisor, and malformed/nonfinite input is not comprehensively checked. [Action handlers](orders-unit-orders.md) normally supply integer workerTime / 30 as float; a nominal small positive workerTime can therefore request zero work. Accepted deferred demand is not immediate balance debit; [economy settlement](../03-economy.md) explains backlog.

ApplyUnfinishedBuildDecay (`0x41bcd0`) supplies negative work `-(buildtime * ticks / energyCost)` against the unit itself. This makes remaining fraction grow proportionally to ticks/energyCost before clamping. Its division is by configured energy cost, not build time, and zero energy cost is not locally repaired. Decay is triggered by GetBuilt's waits, rather than this helper independently scanning every unfinished unit.

## 8. Construction completion and factory release

FinishConstruction (`0x41b8d0`) requires both live runtime 0x10000000 units and a nonnull builder-list pointer on the source definition. It resets target drawFrame and buildLeft, sets 0x2000 and, for active role 1/2 target ownership, either marks BUILDER.GUI dirty for runtime bit 29 or detaches a carried nonfactory product with flag 1.

Activatewhenbuilt flags1 bit 18 activates selector 1. A focused source refreshes build counts. An isfeature target, flags1 bit 24, receives last damage type 7 and runtime 0x4000. Active role 1/2 ownership broadcasts the builder link. Selection of either source or target marks panel dirty. The function does not itself copy rally orders; GetBuilt on the product handles those separately.

This drawFrame reset and buildLeft transition are concrete inputs to [construction rendering](../../naval-systems.md). Completion can be invoked by the progress helper and again by a factory action. Its guarded state mutation is distinct from placing the model, paying a whole unit price upfront or forcing every kind of product to detach.

## 9. Repair's literal arithmetic

AddRepairProgress (`0x41bd10`) returns zero at full health. It computes integer health and energy values from `(maxHealth * work - 1) / buildtime + 1` and `(energyCost * work - 1) / buildtime + 1`. Each value **at least one is set to one**, an upper cap rather than a minimum-one floor. Values zero or below are not separately clamped upward.

It requests only that energy amount from the builder account, then calls DamageUnit with kind 10 and the computed health value. Accepted request returns one. No metal is requested and no particles are emitted here. Ground/air handlers decide their own particle policy, including air repair's unconditional particle request after this call. Negative/malformed work can propagate unexpected values because there is no nonnegative-input guard. Buildtime zero is likewise unguarded. Health-restoration semantics of kind 10 belong to [damage handling](units-unit-targets.md).

## 10. Camera ownership and coordinate projection

This module also owns camera setup, follow, scroll, shake and clamping. ResetCameraState (`0x41c2b0`) clears a 0x5c-byte camera-state range while preserving scrollSpeed. CycleCameraFollow (`0x41c2e0`) uses FindNextSelectedUnit and stores the result. FindLocalCommander (`0x41c310`) scans local slots against the Commander category and retains the last matching unit; no match leaves the prior pointer unchanged in this body. ClearCameraFollowState (`0x41c390`) clears snap countdown and both unit/object following.

ClampCameraPosition (`0x41c3c0`) limits actual scroll to map dimensions minus viewport and refreshes the radar viewport rectangle. ClampCameraTarget (`0x41c450`) applies the same limits to desired scroll without that radar update. The functions assume maps at least as large as the viewport; negative maxima are not normalized to zero first.

SetCameraPosition (`0x41c4c0`) with nonzero third argument changes/clamps the desired target only. Zero changes actual scroll, marks view dirty, clamps and copies actual into target. CenterCameraOnPoint (`0x41c7c0`) subtracts half viewport before the same branch behavior. CenterCameraOnMapPosition (`0x41c8e0`) additionally projects world Y by using screen vertical coordinate whole `(Z - Y/2)` and horizontal whole X, with signed-short truncation. The provisional parameter name instant does not reverse the literal target-only branch.

All these positioning paths clear map flag 8. Camera projection can make an aircraft or elevated ship part appear above the terrain-relative point while its route/occupancy uses horizontal X/Z.

## 11. Follow smoothing and shake

UpdateCameraFollow (`0x41ca10`) prioritizes a pending snap position while its countdown decrements, then a tracked object's position, then a followed unit with live bit 0x10000000. A dead followed unit clears follow state. It projects/centers/clamps the desired target, approaches it by half the remaining difference per axis with a maximum step of 320, applies shake and clamps actual scroll. Integer half-difference can leave a one-pixel residual rather than exact mathematical convergence.

StartScreenShake (`0x41c5e0`) is disabled by UI option 0x10, stores initial/current duration, replaces each directional amplitude only when that argument is nonzero and enables shake for positive duration. AccumulateScreenShake (`0x41c640`) zeros amplitudes if previously inactive, averages old/new duration, adds amplitudes and enables when positive. It does not simply append independent shake events.

UpdateScreenShake (`0x41c6f0`) scales amplitude by remaining/initial duration, selects signed random offsets around half that span, adds to actual scroll and decrements remaining. Expiry clears active. It depends on positive initial duration when active; normal start/accumulate paths establish that. Camera position later clamps map edges, which can clip visible shake.

## 12. Drag, edges and keyboard scrolling

BeginMouseScroll (`0x41cc60`) clears follow/snap, hides software cursor, saves mouse and scroll divided by 16, sets screen-center anchor and warps OS cursor there. EndMouseScroll (`0x41cd20`) clears drag flag, restores mouse position and shows software cursor.

UpdateMouseScroll (`0x41cd50`) takes one quarter of mouse displacement from center, adds it to saved scroll in 16-pixel units, multiplies by 16 and clamps/applies actual/target scroll. It updates saved scroll and recenters the cursor. Releasing keyFlags bit 1 ends scrolling and restores the cursor inline. Quantized saved coordinates explain the distinct drag-scroll feel.

UpdateEdgeScroll (`0x41ce90`) multiplies scrollSpeed by simulation-budget delta and caps at 128; zero returns. It reconciles cursor position near window boundaries according to display flags/focus. Arrow key codes 0xf4..0xf7 move left/up/right/down unless TALK.GUI is open. Mouse-edge movement remains available during that chat check. Left/up branches take priority over right/down on their respective axes. Any move clamps actual scroll, copies desired, marks view dirty, clears map flag 8 and cancels following/snap. This routine does not itself validate negative speed.

## 13. Persistence and coverage boundaries

Panel and camera services do not define independent save records here. Unit packed flags retain paging/standing settings through unit serialization; order queues, construction fraction, model state and resources persist through their own components. GUI assets and current focus are presentation state rebuilt by the surrounding lifecycle.

Every annotated entry in the module, including both separate functions and camera services, is covered through static source inspection. Important limits include substring command routing, mixed-cloak aggregation, focused-selection state reuse, integer work rates, upper-capped repair values, energy-cost-driven unfinished decay and nonzero-size/divisor preconditions. No builds, game runs or asset-dependent visual claims are implied.
