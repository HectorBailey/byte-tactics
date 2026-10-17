# Camera centering, bookmarks, persistence and launch flags

## Scope and coordinate state

The `ingame/camera` group owns eight entries from `0x41d0f0` to `0x41d4c0`, all in [camera.cpp](../../../src/ingame/camera.cpp). It centers views from radar/start records, serializes actual scroll position, stores/restores four bookmarks and exposes two launch-flag setters. Continuous scrolling, follow tracking, shake and clamping are owned by [control_panel](ingame-control-panel.md), despite their camera responsibilities.

The shared state includes actual scroll X/Y, desired scroll X/Y, minimap gadget origin/dimensions, map pixel dimensions, visible-view dimensions, a dirty byte, map flags, follow pointers and snap ticks. X/Y in the screen-scroll pair correspond to map X/Z presentation coordinates; they are not the three-component physical unit position. Four parallel integer arrays retain bookmark coordinates and four bytes retain validity.

## Radar-click centering

CenterCameraOnRadarClick (`0x41d0f0`) snapshots the screen position at game `+0x2c76`. It subtracts minimap gadget origin, scales each coordinate by map pixel size divided by gadget size, and subtracts half the visible view size. The resulting scroll puts the clicked map location approximately at the battle view's center.

It writes actual scroll, sets dirty flag value 2 and calls ClampCameraPosition. It then copies the clamped actual position into the desired position, clears map flag value 8, sets snap ticks zero and clears both follow pointers. Radar navigation therefore cancels retained unit/object tracking and synchronizes current/desired scroll rather than allowing an old target to pull the camera back immediately.

There is no zero-divisor guard for minimap gadget dimensions, no local hit-test to prove the point was inside the minimap, and no local bounds check before scaling. Callers establish those conditions. The shared clamp bounds actual scroll by map dimensions minus viewport and refreshes the radar viewport rectangle; it assumes sensible map/view sizes. [Radar](map-radar.md) and [world picking](game-game-load.md) explain the related transforms without making this function a unit-order operation.

## Mission start centering

CenterCameraOnStartPosition (`0x41d1f0`) scans the mission entry array in order and accepts the first entry whose type equals one and ID equals zero. It uses that entry's signed-short X/Z, subtracts half the view dimensions, writes actual scroll, marks dirty, clamps, copies to desired scroll and clears map flag value 8. If no record matches, it leaves camera state unchanged and reports no error.

This function does not choose the local player's numbered skirmish start. [Commander spawning](game-main-loop.md) centers on the assigned position separately. It also does not clear snap ticks or follow pointers, unlike radar clicking and bookmark restore. Matching the final coordinates does not make their side effects identical.

The source of the entry list is [mission/map metadata](map-map-load.md). This camera helper does not create a commander, validate the start as passable terrain or change the map's mission records.

## Save-bank position contract

WriteCameraPosition (`0x41d360`) selects the HapiBank account Camera and stores integer items X Position and Z Position from actual scrollX/scrollY. It writes neither desired scroll nor a unit-follow ID, bookmark validity, shake phase or snap counter. The surrounding [save orchestration](game-settings-campaign.md) supplies the bank and decides when this account is written.

ReadCameraPosition (`0x41d2b0`) selects the same account, obtains Z then X with current scroll values as per-item defaults, writes actual scroll, marks dirty, clamps, copies actual into desired and clears map flag value 8. Missing items therefore preserve the corresponding current coordinate before clamping rather than forcing a zero origin.

This is binary-bank account/item access through [HapiBank](util-hapibank.md), not a parsed text-file Camera section despite the overview comment. Account selection can create a missing account under the bank API's rules and changes its active-account cursor. Neither helper restores that cursor for the caller or exposes write/read failure status through its void result.

Loading camera position does not clear follow pointers or snap ticks in this body. The surrounding restoration order must establish whether those are already reset. The record is local presentation state; these helpers send no camera packet or unit position update.

## Four camera bookmarks and the validity check

SaveCameraPosition (`0x41d3b0`) stores current actual scroll into the X/Y arrays at its supplied index and sets that validity byte to one. It does not clamp or validate the index. Ordinary [game-key callers](../../../src/ingame/keys.cpp) map four save events `0xd2..0xd5` to slots zero through three.

RestoreCameraPosition (`0x41d3f0`) contains a crucial source-level distinction: its condition compares the validity array itself to zero, not validity[index]. An embedded array's address is non-null for a valid Game object, so this check does not reject an unsaved slot. It clears snap/follow state, loads the indexed coordinates, marks dirty, clamps, copies actual into desired and clears map flag value 8.

The restore events `0xe6..0xe9` supply slots zero through three, but the helper itself does not enforce that range. After an ordinary zeroed/reset camera block, restoring a never-saved slot can therefore move to its zero coordinates instead of doing nothing. [ResetCameraState](../../../src/ingame/control_panel.cpp) clears the camera block while preserving scroll speed, including bookmark storage. Bookmark data is not included in WriteCameraPosition's save record.

## Command-line flag adapters

SetCommandLineUnusedFlagL (`0x41d4a0`) directly stores its integer in g_commandLineUnusedFlagL. SetBypassDriveScan (`0x41d4b0`) directly stores its integer in g_cdBypassDriveScan. Neither normalizes to a boolean, updates the view, saves a preference or broadcasts anything.

[Command-line parsing](game-main.md) calls the former with zero for L and the latter with one for F. [CD identification](game-cd-check.md) tests the bypass value as nonzero and then probes its hardcoded drive instead of enumerating Windows CD-ROM letters. Their placement in the camera address group does not give them a hidden relationship to map navigation.

## Completion and interaction boundaries

All eight entries are covered, with the shared clamp/reset and ordinary bookmark callers traced. View changes affect presentation and later world-coordinate picking; they do not move selected units, reveal terrain, grant LOS or change weapon eligibility. The current map dimensions, minimap layout, mission records and existing follow state determine the displayed result. No camera interaction, save load or CD probe was performed for this static documentation.
