# Selection flags, picking, groups, overlays and order distribution

## Scope and representation

The `ingame/selection` group owns 29 entries in `0x48bae0..0x48dfb0`: 28 in [selection.cpp](../../../src/ingame/selection.cpp) and group-order distribution in [selection_48cf30.cpp](../../../src/ingame/selection_48cf30.cpp). The group also owns an emitted vector insertion wrapper and the MissionConditions constructor at its final address. Physical adjacency does not make victory-condition evaluation part of selection.

Selection is stored in unit flags, not a permanent single vector. Bit 4 (`0x10`) is selected, bit 5 (`0x20`) is an availability/completion-related gate, and bits 6/7 form a two-bit visit/display state. The game also has unitIndex, hoverUnitId, focusUnitId, an on-screen unit-ID array and a separate minimap hit array. Different functions use localPlayer for commands and playerIndex for observer/presentation work; those indices must not be conflated.

A repeated eligibility predicate requires bit `0x20`, buildLeft exactly zero, postTransferHoldoff zero, and either no carrier or carrier flag `0x40000000`. This is not a generic “order queue empty” test. A function named idle in this module does not inspect current mission names before applying that predicate. [Unit lifecycle](units-units.md), [ownership transfer](units-unit-commands.md) and [attachment](units-unit-position.md) supply the state behind it.

## Visible list and membership queries

CollectVisibleUnitIds (`0x48bae0`) scans the inclusive global unit allocation range. For every nonzero type it obtains whole position and definition bounds, projects an axis-aligned screen rectangle using scroll and the 128/32 viewport borders, and tests inclusive overlap against the current view limit rectangle. Its top combines minimum Z with maximum Y; its bottom combines maximum Z with minimum Y. For units whose low two state bits are not one, a map-cell height can lower the minimum Y used for that bottom.

A rectangle-overlapping unit is appended when it belongs to playerIndex or IsUnitVisibleToPlayer accepts it for that observer. It writes instance IDs into the preallocated list and stores the resulting count. No output capacity is supplied or checked here. This is a maintained on-screen candidate list, not a rebuild of LOS itself or a list of every sensor contact anywhere on the map.

IsUnitVisible (`0x48bcb0`) linearly searches that list for the unit's instance ID at `+0xa8`. A nearby comment calls it a type ID, but different instances sharing one definition remain distinct. This function's name means membership in the collected visible screen list; it is not equivalent to the broader visibility predicate used while building it.

## Clearing and bulk selection

ClearSelection (`0x48bd00`) masks every global unit with `0xffffff2f`, clearing bits 4, 6 and 7 while retaining bit 5. It then requests PopUntilNamedLayout without force. It does not cancel orders, remove squad membership or necessarily dismiss a modal layer that defers the HUD request.

ClearUnitSelectFlags (`0x48c150`) clears only bits 6/7 using `0xffffff3f`; it leaves selected bit 4 intact. MarkLocalVisibleUnits (`0x48c320`) sets those two-bit state fields to one on local-owned entries of the visible list. Several selection/focus flows use this state as visitation bookkeeping independently of membership.

| Entry | Bulk behavior |
| --- | --- |
| SelectAllIdleUnits, `0x48bd50` | Adds every local unit meeting the common eligibility predicate to selection; it does not first clear existing selection or inspect an idle order. |
| SelectUnitsOfSameTypes, `0x48be00` | Builds a 512-type bitmap from current local selected units, then adds eligible local units of those types. |
| SelectUnitsByCategory, `0x48bf30` | Selects eligible local types in the named category; with modifier zero also deselects eligible nonmembers, otherwise retains them. |
| SelectAllVisibleUnits, `0x48c030` | Clears global selected/visit bits and requests HUD restoration, then selects eligible local units in the maintained on-screen list. |

The first three reset unitIndex, select the neutral STOP interface state and request orders-panel refresh. The visible-only selector does those final actions only when something qualified. Category selection uses a definition category bitset, not a LOS bitmask despite its overview comment. The same-type bitmap assumes type indices below 512; the code does not clamp before bit indexing.

DeselectIfIneligible (`0x48c9b0`) revisits a selected unit and clears its selected bit when any common eligibility requirement fails. It resets unitIndex and requests panel refresh. It does not change unselected units or independently check ownership/definition validity. CollectSelectedUnits (`0x48ca20`) clears a caller's vector and appends local-player pointers with bit 4 set, without repeating the eligibility checks. Temporary collection can therefore reflect flags awaiting normalization.

## Rubber-band and click selection

SelectUnitsInBox (`0x48c390`) projects the stored drag's start/end world positions, using each endpoint height, into screen coordinates and sorts the two axes. Parameter-block flag value 4 chooses toggling. Without it, the function clears global selected/visit state and requests the HUD; with it, existing selections remain until individually toggled.

It scans the local unit range and applies the common eligibility predicate. A unit qualifies geometrically when its projected center lies within the drag rectangle, inclusive. It does not test full model intersection or require inclusion in the visible-ID array. Matching units are selected or toggled, and the routine counts all eligible units that remain selected, not only newly hit units.

It clears unitIndex, resets visit bits globally and marks local visible units with visit state one. A geometric hit sets the panel-refresh bit even when toggling deselects it. No remaining selected units returns zero; one selected unit queues selection speech kind one; multiple play SelectMultipleUnits and return one. In toggle mode, retained selections can cause a success/audio result even without a newly hit unit.

ClickSelectHoverUnit (`0x48c7f0`) requires a nonzero hovered instance, local ownership and common eligibility. With parameter flag 4 it toggles selection, queues speech only when selecting, clears unitIndex and marks refresh. Without that flag it clears global selected/visit state, requests HUD restoration, marks local visible units with visit state one, selects the hovered unit, queues speech and marks refresh. That non-toggle path does not explicitly clear unitIndex itself, instead relying on its surrounding HUD logic.

These functions change selection flags and hints. SelectStopOrder resets the interface mode; it is not an order that stops each selected ship or erases construction queues. [Key dispatch](ingame-keys.md) and [battle input](ingame-build-placement.md) provide the caller context.

## Screen hull and minimap picking

HitTestUnitScreenHull (`0x48c6a0`) obtains the model bounds with traversal argument zero, makes four X/Z footprint corners at the lower bound's Y, rotates them by the unit's three angles and projects the resulting quadrilateral. Its Z projection subtracts the rotated local Z from the unit/view offset. It invokes PointInPolygon on those four screen points.

This is a rotated bounding footprint, not triangle-accurate visible-pixel/depth picking. Model bounds, heading/pitch/roll and camera projection can therefore determine a hit even where a sprite/model silhouette has transparent space. The [model utility](graphics-pcx.md) and [math](util-math.md) references describe the called bounds/rotation primitives.

PickUnitUnderCursor (`0x48cd80`) gives the battle-view rectangle priority over the minimap if both tests could succeed. In the battle region it tests candidates from the visible-ID list. Among hit hulls it selects the smallest definition-size metric: half sizeY plus sizeZ, multiplied by sizeX with fixed-point products. It does not choose the nearest world center or frontmost depth sample. Strictly smaller replacement preserves the earlier candidate on a tie.

Outside that view but inside the minimap hit rectangle, it searches the hot-radar array for the smallest squared screen distance strictly below four. That is a radius strictly below two pixels, not two map cells. Ties likewise retain the earlier candidate. Elsewhere or with no hit it returns zero. The hot-radar array's population and contact visibility belong to [radar](map-radar.md); this picker does not rerun all sensor policy.

## Cursor aggregation and displayed orders

ResolveCursorModeForSelection (`0x48d220`) collects local selected pointers and removes the hovered target pointer. With no remaining selected actors, argument one plus a local/eligible hovered target returns selection cursor `0x0f`; other cases return `0x13`.

With actors present, it calls GetOrderCursor for each using global orderMode, target and the retained world cursor position. It starts at `0x13` and returns the smallest numeric cursor value. The incoming argument only affects the empty-actor case; it does not replace global orderMode for those per-unit calls. A heterogeneous selection's hint can reflect one capable member, not unanimous ability. [Order dispatch](orders-order-dispatch.md) and [cursor helpers](units-unit-scripts.md) define those per-unit results.

DrawSelectedUnitOrderOverlays (`0x48cc30`) scans the local unit range, accepting flag `0x10000000` and rejecting `0x4000`. A unit that is the caller's focus, unitIndex or hoverUnitId receives overlay mask `0x1f` with final flag one. Other selected units receive that mask with flag zero. If any focus/index/hover definition has a non-null build-ID list, remaining eligible units receive mask one/flag one. This is an explicit context-sensitive overlay policy, not simply “draw all selected orders.”

[Order overlays](orders-order-list.md) interpret those masks and [markers](ingame-markers.md) perform actual drawing, including build footprints, range rings and path trails. These callbacks borrow unit/order/definition state and can update marker caches under their own rules.

## Cycling and commander focus

FindNextSelectedUnit (`0x48c190`) searches the local player's numeric ID range forward or backward, wrapping and testing selected bit 4. It derives the starting ID from the supplied unit or zero and normalizes an out-of-range start to the first index. It can return the same unit after a full wrap when no other member is selected. Its slot lookup maps ID zero to null but dereferences the resulting pointer without an independent guard, so valid nonzero player ranges are required.

FindNextUnmarkedLocalUnit (`0x48d420`) finds the first nonzero-type local unit with visit state zero. If none exists it resets visit state on all global units and searches again. It does not require the common completed/selectable predicate. FocusNextLocalUnit (`0x48d4d0`) uses an inlined equivalent, stores focusUnitId, centers the camera, rebuilds visible IDs and marks local visible units plus the chosen unit visited. It does not set their selected bit despite its overview comment.

FocusCommander (`0x48d630`) uses playerIndex and the selected side definition's commander unitname. It finds the first eligible unit whose definition name matches case sensitively, clears follow state and centers the camera. Argument zero returns after centering. A nonzero argument also resets STOP interface mode, clears global selection/visit bits, requests HUD restoration, selects that commander and marks panel refresh. This is exact unitname matching, distinct from the broader Commander category used by some other systems.

CycleSelection (`0x48d790`) scans the playerIndex range for eligible units, remembers the first, and stops at the first already selected eligible unit after clearing global selection/visit state. It then selects the next eligible unit, wrapping to the remembered first when necessary, and marks refresh. If none was selected it chooses the first eligible unit. It does not center the camera or queue selection speech in this body.

## Replacing and selecting squads

CreateSquad (`0x48d920`) walks nonzero-type local units. Selected units are assigned to the requested group; unselected units already in that group are reassigned to group zero. It therefore replaces that group's members but does not set removed units to the no-group value -1. [Squad storage](units-squads.md) owns vector/field consistency and index preconditions.

SelectSquad (`0x48d9a0`) has an additional CTRL_F category policy. It first checks whether the requested group has an eligible member in CTRL_F and an eligible member with flag `0x80000000`. If both conditions hold, group selection excludes CTRL_F members and selects eligible members outside that category. Otherwise it selects all eligible group members. A zero modifier deselects eligible units outside the group; a nonzero modifier preserves them.

It resets STOP interface state/unitIndex, marks refresh and returns whether it selected any eligible members. The two presence conditions can be satisfied by different units. They are not a requirement that one unit simultaneously has both traits. SquadHasCtrlFMember (`0x48dc30`) exposes the category predicate, and SquadHasReadyMember (`0x48dd10`) exposes the flag-`0x80000000` predicate, each with common eligibility and matching group ID.

The exact unit types in CTRL_F are asset-defined. The function does not hardcode ships, factories, aircraft or constructors, and it does not change a member's command queue merely by selecting it.

## Distributing a command over the selection

IssueOrderToSelection (`0x48cf30`) first obtains queue/cancel control from bit 2 of its argument block's flags at offset eight. It determines whether the current mode or explicit mission type takes a target, using OrderModeTakesTarget for a nonzero mode or mission flag bit 9 otherwise. When target-taking, the hovered unit becomes the target/excluded actor.

It counts local selected actors excluding that target and averages their signed whole X/Z positions. Integer division happens before converting the average back to 16.16, so fractional average placement is discarded. With no actors it returns. For each actor, a nonzero mode resolves a per-unit mission using GetOrderType, the hovered target and global cursor position. A zero mission ID skips that actor.

Standing_FireOrder requires the definition's firestandorders flag, and Standing_MoveOrder requires mobilestandorders. These are flags2 bits one/zero, not the broader canattack/canmove bits. Other mission types pass those specific gates.

For an order with flag value two and a supplied destination, it can preserve the actor's offset from the group average. The decision adds the high 32-bit portions of squared fixed-point X/Z deltas and compares the result to actor count times 3000. In world terms this is approximately squared horizontal distance compared with count times 3000, not distance compared with that number. The effective radius therefore grows roughly with the square root of group size.

An actor inside that criterion receives destination X/Z plus its offset from average, retaining the supplied destination Y. Others receive the unmodified destination. The target remains excluded even if it was selected. Every accepted actor is passed to IssueOrCancelOrder with the queue/control flag, target, destination, type ID and amount. This is a coarse formation-offset policy; it does not verify that each shifted destination is reachable, prevent units colliding, or force every actor to receive the same interpreted mission.

## Generated/container and adjacent lifetime entries

The owned vector insertion at `0x48ddc0` is MSVC's single-element insert wrapper. It saves the insertion iterator's offset from begin, delegates to count insertion with one element, then returns the new begin plus that offset after possible reallocation. The existing toolchain VECTOR implementation was read to establish this behavior. It copies borrowed Unit pointers; destroying a temporary vector does not destroy the units.

MissionConditions::MissionConditions (`0x48df90`) sets active to one and both victory/defeat counts to zero. The [shared declaration](../../../src/game/mission_conditions.h) contains sixteen pointers per condition array; the constructor does not initialize every array element. Zero counts control which pointers are live. Evaluation, registration, teardown and persistence belong to the adjacent victory group, which is not completed by documenting this constructor.

## Completion and limits

All 29 owned entries are covered, including the companion group dispatcher and adjacent constructor/container support. Instance IDs, definition IDs, selected flags, visit states and on-screen lists have distinct roles. Their capacities and many source-pointer/index assumptions rely on the surrounding allocation/lifecycle contracts rather than local validation in every selector. No unit selection, command, game execution or runtime picking test was performed for this reference.
