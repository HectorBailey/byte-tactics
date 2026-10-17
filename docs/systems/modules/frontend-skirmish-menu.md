# Skirmish setup, player rows, map selection and launch checks

Status: finished. This reference covers all nineteen owned entries of `frontend/skirmish_menu`, `0x479660..0x47bdf0`, in [skirmish_menu.cpp](../../../src/frontend/skirmish_menu.cpp). The setup-screen data is independent of live player state until ApplySlotsToGamePlayers runs. [Settings](game-settings.md) persist the setup, [campaign-menu helpers](frontend-campaign-menu.md) provide shared slot/color queries, and the [frontend coordinator](frontend-frontend.md) responds to start/back requests.

## Setup table and screen lifetime

Each configured setup slot occupies 24 bytes: controller/active type, side, allegiance group, starting metal, starting energy and color index. Controller zero is open, one is the local human and two is computer. Allegiance groups cycle through zero to five; five is the unaffiliated sentinel. The table also stores commander-death, mapping, LOS enable/type, fixed-location rules, a map-name buffer, base gadget count, current edited row and difficulty. The local declaration reserves eleven slot records before these options, while supported typed-history changes select three through ten configured slots. Runtime helpers trust the configured count and do not generally clamp it.

OpenSkirmishMenu (`0x47bbb0`) blanks the screen, loads SKIRMISH.GUI, installs the click handler with game callback data and attaches Skirmsetup4x. These exact names are confirmed by [globals.csv](../../../data/globals.csv). It copies setup difficulty into the game difficulty and sets the Difficulty stage/Easy/Medium/Hard selection for values zero/one/two. It tries the retained map name; failure refreshes the mission list and copies its current mission name back. It builds/refills rows, installs the cheat-text handler, enables keyboard input, redraws and selects cursor mode `0x13`.

The difficulty pointer, layer, options pointer and required gadget names are assumed valid. Map-name copies are limited to 256 bytes without always forcing a terminator. The screen does not allocate/free the setup table. Its main click cleanup notification simply returns. Settings load/save and wider game teardown own the retained setup, while layer resources belong to [GUI runtime](gui-gui.md).

## Dynamic player-row construction and refresh

BuildSkirmishPlayerRows (`0x479c50`) creates six controls per configured slot. Vertical spacing is integer 200 divided by configured count; the first y centers the resulting span within the chosen panel. Zero count divides by zero. Two zeroed record templates are reused for button and hotspot additions, so later controls inherit template fields unless overwritten. The generic [addition helpers](gui-dialogs.md) copy only their declared record prefixes and do not grow storage.

| Row control | X | Initial size | Artwork and role |
| --- | ---: | --- | --- |
| PlayerN | 45 | 112 by 20 | skirmname button, controller label. |
| SideN | 163 | Width 45 | SIDEx sequence, two stages. |
| ColorN | 214 | 20 by 20 | Hotspot later bound to logos32. |
| AlliesN | 241 | 40 by 20 | Hotspot with allegiance help text, later bound to TEAMICONSx. |
| MetalN | 286 | 45 by 20 | skirmmet button, attribute `0x10000`, increase/decrease help. |
| EnergyN | 337 | Width 45 | Reuses resource-button template and skirmmet artwork. |

BindGadgetAnimSequence (`0x479bf0`) clears the template sequence pointer, searches the current screen GAF, and on a found frame replaces width/height with its dimensions. Missing artwork preserves the supplied dimensions. Some reused template fields, including the temporary frame before a later reset, follow the actual assignment order rather than separate construction for every control.

RefreshSkirmishSetup (`0x47a0e0`) captures the existing gadget count as baseGadgetCount and appends all player rows. If every slot is empty, it activates slot zero as human and slot one as computer, without separately checking that two slots were configured. It finds TEAMICONSx and loops its frame count while repeatedly zeroing frame-zero offsets, not each frame's offsets. Each row gets its controller label, side stage, decimal metal/energy text, logo/color frame and team-icon sequence. Open slots leave PlayerN active and deactivate Side/Allies/Metal/Energy/Color. Active rows rely on freshly initialized template state rather than a separate explicit reactivation pass.

It then refreshes allegiance frames and global rule controls, fills MapName and marks changed. This is a row-appending operation, not an idempotent redraw: callers rebuilding an existing screen must first restore the saved base count. Generic count/capacity limits and unchecked translated help-string copies still apply.

RefreshAllyIcons (`0x479660`) counts nonempty slots sharing each row's team. Zero members gives frame ten, one member gives team times two plus one, and multiple members give team times two. The row's own active state is not a prerequisite for calculating that frame, although open-row controls are normally inactive. Team five therefore participates in icon counts even though live alliance creation treats it specially.

## Controller, side, allegiance and color changes

CycleSlotController (`0x4797e0`) cycles open to computer; human to open; computer to human only when no other human exists, otherwise computer to open. It updates the controller label and active state of all row controls. Newly occupied rows whose color conflicts with another active slot receive FindFreeColor's first unused color. That helper tests all configured slots, including open slots, whereas the conflict test checks active slots only. If no free color is found, `-1` is stored and narrowed into the displayed unsigned frame, without an additional fallback.

CycleCurrentPlayerSide (`0x47a8e0`) increments the current row's side modulo configured sideCount. A zero side count divides by zero. CycleCurrentPlayerAllyGroup (`0x47ae30`) increments team modulo six and refreshes icons. The main handler contains equivalent inline changes. Neither helper sends a packet; this is local skirmish setup.

CyclePlayerColor (`0x47acd0`) advances or reverses modulo the logos32 sequence count, maps `-1` to the last frame and repeats while another active row uses that color. It updates ColorN's sequence/frame and marks changed. A zero sequence count divides by zero, and a fully occupied palette can make the loop never terminate. Reverse is selected by the caller's nonzero argument. SelectLogosGadget (`0x47acb0`) merely looks up LOGOS and assigns its result to hotGadgetIndex, allowing existing GUI activation machinery to treat that list as selected.

## Rule values and displayed stages

Refresh maps stored rules to display stages as follows; click handling changes the underlying rules and updates associated help text:

| Rule | Stored value and meaning | Display stage |
| --- | --- | ---: |
| fixedLocations | Nonzero: predetermined starts; zero: random placement. | 0 or 1 respectively. |
| commanderDeath | Nonzero: commander destruction ends the game; zero: continue afterward. | 0 or 1. |
| mapping | Nonzero: black out unexplored terrain; zero: terrain visible. | 0 or 1. |
| lineOfSight/losType | LOS disabled: all mapped terrain visible. | 0. |
| lineOfSight/losType | LOS enabled with losType one: terrain elevation affects view. | 1. |
| lineOfSight/losType | LOS enabled with losType other than one: elevation does not affect view. | 2. |

CommanderDeath, StartLocation and Mapping clicks XOR their stored integer with one. LOS clicks cycle disabled to enabled/elevation-aware, then enabled/elevation-independent, then disabled with losType reset to one. The GUI button stage and stored flag are separate representations; the generic button handler manages stage cycling while this callback adjusts setup and help text. [Line of sight](map-line-of-sight.md) documents resulting simulation visibility.

Difficulty cycles zero, one, two and writes both game difficulty and retained setup difficulty. Unexpected values are unchanged. Start locations, commander rules and resources are consumed downstream; changing their labels here does not immediately affect a running battle.

## Click decoding and resource amounts

HandleSkirmishClick (`0x47ae60`) ignores cleanup index `-1`. It obtains the selected gadget name, converts its final character using atoi, stores that as current row, then removes the final character from its scratch copy. The original gadget name remains unchanged for IsCurrentGadgetNamed tests. Row controls are therefore decoded as a text prefix plus one final decimal digit, with no general multi-digit index support or empty-name validation. Non-row controls still perform that preliminary parse, typically yielding zero, before their named branch executes.

Player, Side, Allies and Color prefixes invoke the changes above. Color uses layer clickMode one for forward and two for reverse; a fetched mouse snapshot is not itself the direction decision. Metal/Energy use the same clickMode convention:

- Increase adds 500 and clamps at 10000. If the resulting value equals 700, it is changed to 500, making an initial 200 advance to 500 rather than 700.
- Decrease subtracts 500 and clamps at 200, so 500 decreases to 200.
- Display text is refreshed as decimal and translated-text helper is called with argument ten.

Arithmetic is ordinary signed integer arithmetic without overflow defense for externally corrupted settings. Standard UI values therefore follow the sequence 200, 500, 1000 and subsequent 500 increments. These are retained setup amounts, not transfers from the current economy. Other branches toggle rules, open SelectMap, change Difficulty or request PrevMenu substate three. Most edits end by clearing selection to prevent generic automatic closing. Opening a map dialog also clears the shared activation afterward.

## Map selector construction, preview and acceptance

OpenSkirmishMapSelector (`0x47aaf0`) first counts maps through LoadMapList with zero filters. A zero count opens a no-maps message and returns. Otherwise it loads SELMAP.GUI with flags `0x880`, allocates a 32-byte SELECT MAP DATA object, installs the map callback/data, attaches DSELECTMAP2, obtains and sorts the packed map-name list, and configures MAPNAMES. The count and actual list come from separate calls and are not reconciled if discovery changes between them.

It installs UpdateSkirmishMapSelection, finds the retained map name by exact strcmp and scrolls to its row, then loads the selected mission for metadata display. It enables keyboard input, redraws and returns cursor mode `0x13`. Preview selection loads the mission immediately into the shared Mission object before acceptance. UpdateSkirmishMapSelection (`0x47aaa0`) calls ShowSelectedMapInfo only on successful metadata load; it does not update MAPPIC itself. That display helper is defined in [multi.cpp](../../../src/frontend/multi.cpp), a shared frontend dependency rather than an owned skirmish body.

HandleSkirmishMapClick (`0x47a910`) accepts MAPNAMES or LOAD activation, with a sound only for LOAD. It copies the selected packed string into setup mapName, calls LoadMissionByName and sets MapName text through the supplied context. The load result is ignored in this acceptance path. PREVMENU plays Previous and permits generic closing; other selections clear activation. Cleanup looks up MAPPIC, frees its items pointer when present, then frees the list-name buffer and data object. It does not restore the previous Mission preview on cancellation. Actual GUI layer closing, surface lifetime and callback notification order follow [dialogs](gui-dialogs.md) and generic GUI.

## Launch validation and failure flow

Start activation first plays BigButton. If FindGameCdDrive(1) fails it shows the multiplayer-disc prompt and clears selection, **but does not return**. RegisterDataArchives and all later launch checks still run. Therefore the displayed missing-disc warning is not an unconditional launch veto in this body. Nested message creation can also change the top GUI layer while the callback retains its original context/entry pointers.

It counts computers and provisionally sets numPlayers to computers plus one, then loads the configured map. Failure shows the missing-terrain message and returns. At least one computer and one human must exist. Only then does it compare the provisional numPlayers with CountStartPositions, reject a single shared allegiance group, recount actual human/computer slots, assign their total to numPlayers, apply the slots, initialize mission status, save settings and request frontend substate two with cursor mode `0x14`. Missing human/computer shows a separate warning. The ordinary controller UI enforces at most one human; if retained data contains more, the capacity check uses computers plus one before replacing that count with the actual total.

AreAllPlayersInOneAllyGroup (`0x479760`) finds the first active slot whose team is not five. If none exists it returns zero. It then rejects sameness when any active slot has a different team, including a team-five slot. Thus all active slots on the same non-five team returns one; all slots on team five returns zero; a mix with a five slot also returns zero unless every active team equals the chosen non-five team, which cannot include that five. The source's provisional comment referring to controller type is not the actual tested field.

## Applying setup to live players and alliances

ApplySlotsToGamePlayers (`0x47a760`) iterates every configured slot. Human/computer slots copy color and side into live PlayerInfo, call SetupPlayerSlot with kind one/two, and humans update both localPlayer and playerIndex. Empty/other controllers call SetupPlayerSlot with zero. If multiple humans are present, the final one wins those local-index assignments.

For each active human/computer it marks allied entries for matching active team members when the team is not five, and always includes self through a separate fallback check. The helper does not explicitly clear all other allied bytes in this body; SetupPlayerSlot/player initialization owns any baseline reset. It does not copy starting resource amounts here, relying on the retained setup and [loader](game-game-load.md) for those values. Rules likewise remain in the table for their downstream consumers.

FindNextAllySlot (`0x47a700`) exposes the same search: from start through configured count, return the first active same-team non-five slot or self when reached; otherwise `-1`. Self can be returned by its fallback independent of that record's active state. It does not wrap, and invalid self/start values are not guarded.

## Typed slot-count controls and destruction messages

HandleSkirmishCheatText (`0x47b9f0`) checks fixed suffix positions in the retained GUI key history for literal commands confirmed in globals.csv: `*III`, `*IV`, `*V`, `*VI`, `*VII`, `*VIII`, `*IX` and `*X`, selecting three through ten slots. Its ordered comparisons use lengths two through five and distinct offsets rather than token parsing. For three/four/eight/nine/ten it clears the fifteen-byte history; five/six/seven retain it.

On recognition it saves current settings, changes numSkirmishPlayers, saves that count, reloads settings, restores the gadget count to the prior base, rebuilds setup rows, plays the configured cheat sound, advances focus and marks changed. This is the path that deliberately discards old dynamic controls before refresh. Reload behavior, default slot values and registry storage are defined by [settings](game-settings.md); no build or dependency change occurs.

AnnounceForcesDestroyed (`0x47bd70`) is gameplay-facing support located in this frontend range. It chooses Arm for side zero and Core for every other side, formats that name plus one of three randomly selected translated phrases into a 200-byte buffer, and calls AddMessage with category four and the player's index. It does not determine defeat, destroy units or broadcast a victory result itself. Long translated text can exceed its unchecked sprintf buffer; additional configured sides still receive the Core label.

## Coverage and source limits

All nineteen owned functions, their locally inlined shared helpers, row templates and global command/resource names have been traced. GUI data, logos count, settings bounds and valid pointers are assumptions at several entry points, not guarantees provided by these helpers. No source listing, build, dependency installation or game execution was used to document this module.
