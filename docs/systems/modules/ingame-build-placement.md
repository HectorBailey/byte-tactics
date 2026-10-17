# Battle-frame input, placement clicks and battle exit

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `ingame/build_placement`, four functions in [build_placement.cpp](../../../src/ingame/build_placement.cpp), `0x498f70` through `0x499880`. Its scope is battle input routing and lifecycle transitions. Geometric placement validity and preview bounds are implemented by [control-panel services](ingame-control-panel.md), while [terrain queries](map-terrain.md) and action handlers recheck real placement later.

## 1. Shared input state

BattleFrame copies the current 24-byte mouse record, updates its world position, reads the input-flags byte and chooses cursor/placement behavior. Order mode 1 is contextual; 14 is product placement. Relevant input bits are 1/2/4 for region/interface routing, 8 for selection drag, 0x10 for radar dragging, 0x20 for retained command mode and 0x40 for build validity. Their producer lives outside this module, so they are described by their observed routing effects rather than invented universal button names.

Windows messages are left down/up 0x201/0x202 and right down/up 0x204/0x205. Cursor mode 15 selects a hover unit; modes 17 and above take the no-action branch in the left-click handler; 19 is the neutral/selection cursor used throughout these paths. Cursor animation changes only when the numeric mode changes.

Input, queue insertion, unit movement and construction rendering are separate stages. A valid preview is not a reserved factory slot or funded build. Another unit can obstruct the site before a builder reaches it, and the [construction handlers](orders-unit-orders.md) then apply their own retries/failure policy.

## 2. Left-click behavior

HandleLeftClick (`0x498f70`) prioritizes placement mode 14. Validity bit 0x40 permits IssueMobileBuildOrders and oktobuild sound. Key flag 4 sets retained-mode bit 0x20 and returns, enabling additional placements. Without that key it restores contextual mode 1, clears retained mode and clears the STOP gadget's group status when present. Invalid placement only plays notoktobuild; it neither creates an unfinished object nor exits placement mode.

Outside placement, cursor mode 15 delegates to ClickSelectHoverUnit and returns. Cursor mode at least 17 issues no action; interface type 1 in contextual mode additionally clears selection and pops to the named base layout. Other cursor modes call IssueOrderToSelection with current intent, current world point and explicit zero type index, allowing normal contextual dispatch.

The ordinary command path has the same retained-key behavior as valid placement. Without key flag 4 it restores mode 1, clears 0x20 and resets STOP group state. Selection-only/no-action branches return before that ordinary command reset.

## 3. Right-click behavior and interface variants

HandleRightClick (`0x499100`) first cancels any noncontextual intent: restore mode 1, clear retained mode and reset STOP group state. It does not send a Stop order in that cancellation branch.

In contextual mode with interface type 0, input bit 2 plus key flag 8 starts mouse scrolling. Without that modifier the same bit clears selection and pops the layout. Input bit 1 starts radar dragging, sets bit 0x10 and switches to cursor 19. For other interface types, input bit 4 instead calls IssueOrderToSelection with mode 1 and current position. The local MissionType is default-constructed; its [shared constructor](../../../src/orders/mission_type.h) initializes index to zero, matching contextual dispatch through the left-click branch.

These branches explain why right-click can cancel intent, clear selection, scroll or issue a contextual action depending on interface and current region. It is not one unconditional move/attack command.

## 4. Frame routing priority

BattleFrame (`0x499200`) first recomputes placement validity when input bit 2 and mode 14 are both active. With neither bits 2 nor 1, it sets neutral cursor 19. Otherwise it picks the hover unit and resolves a cursor for the selection/intent. It then dispatches one deferred panel-page request.

If retained-mode bit 0x20 is set and key 0xf9 is no longer down, it restores contextual mode, clears the bit and resets STOP group status. Thus retaining intent on a prior click depends on continued modifier state, not only a permanently latched placement flag.

The remaining input path uses this priority:

1. Radar dragging: right-up ends it for interface type 0, left-up for other interfaces; other frames center the camera on radar position.
2. Active mouse scrolling updates drag scrolling.
3. Right-down runs the right-click handler.
4. A noncontextual intent accepts left-down through the left-click handler.
5. Active selection drag handles left-up or updates its end point.
6. An otherwise unhandled left-down can start selection drag, start radar drag for interface type 1, or issue the contextual left-click action for other interface types.

Selection drag starts only on input bit 2, records timestamp and all three whole world coordinates as both start/end, sets bit 8 and uses cursor 19. During dragging the end follows current whole world position. On release it clears bit 8 and compares horizontal extents plus wall-clock GetTicks. A gesture within 25 clock units and less than 32 world units on both X/Z is treated as a click. Other gestures call SelectUnitsInBox; a zero result clears selection and pops the layout. These are strict size/time comparisons, not a screen-pixel-only rectangle threshold.

Regardless of which normal input branch executes, MainLoopTick follows. The function then checks the displayed focus ID; a slot whose definition ID became zero causes the panel layout to pop. This ordering lets simulation destroy a focused unit before the post-tick UI validity check.

## 5. Endgame transition

After the tick, endgame bits 2 or 4 request exit. Non-game-type-3 missions proceed immediately. Type 3 requires BroadcastPendingViewState to return nonzero before teardown. This is a lifecycle/network gate; the victory condition itself is evaluated elsewhere.

The transition sets neutral cursor, pops/closes screens and, for type 3, resets chat HUD indices and runs one additional MainLoopTick. It shuts down ingame systems, clears key queue, selects sound-track category 4, sets frontend state 7 and installs EndGameFrame. It installs LeaveNetGameCallback as close handler and resets endgame screen state to zero.

EndGameFrame (`0x499880`) simply calls RunEndGameState. It is an application frame adapter, not a second simulation or score-calculation algorithm. [Campaign selection](game-campaign.md), lifecycle loading and frontend endgame modules supply the next stages.

## 6. Restart transition

RestartMissionRequest is tested after the endgame block and has no else exclusion against that earlier transition. If both requests are set, the restart branch can perform later state/handler changes in the same invocation. This body does not explicitly clear the restart request; surrounding lifecycle state owns that reset.

For game type 1 it shuts down, closes screens, blanks display, remembers mission index/campaign name, reloads the campaign and reselects the mission. Successful selection sets network-game bit 3 and bit 2. It installs frontend state 2, MenuFrame and LeaveNetGameCallback.

Other game types preserve player count across shutdown, close/blank display, set cursor 20, restore count and load the mission named in options at +0x11c. They apply slot data to game players, set network-game bit 2 and install the same menu state/handler. Both branches select sound-track category 4. Loading results in this noncampaign branch are not checked through a return status here.

## 7. Ownership and limits

This module owns no independent serializer, preview model renderer, unit allocator or resource debit. Placement commands flow to selection dispatch and construction handlers; terrain/range/resource state can change between preview, click and work. GUI layout, world cursor picking, radar position conversion, simulation scheduling and shutdown each remain separate dependencies.

All four functions and their input/lifecycle branches are covered by static source inspection. Explicit limits include interface-dependent region flags, modifier-retained intent, strict gesture thresholds, network-gated endgame and potentially overlapping endgame/restart requests. No builds, installation or game execution were used.
