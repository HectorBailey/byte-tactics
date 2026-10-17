# Frontend coordination, main menu, movies and picture cache

Status: finished. The `frontend/frontend` interval is `0x4256d0..0x428c60`, containing 38 owned entries. The implementation is [frontend.cpp](../../../src/frontend/frontend.cpp), with the [checksum stub](../../../src/frontend/frontend_428bc0.cpp) separate. This is the frontend coordinator, rather than the specialized campaign, multiplayer, options or end-mission screen implementations.

## State layers and frame integration

Three byte fields drive the menu machine: frontendMenuState at game offset `0x2bbe`, current frontendSubstate at `0x2bbf`, and requested substate at `0x2bc0`. They differ from the wider frontendState at `0x391f1`, used by the overall game frame/lifecycle machinery. A main-menu state value is therefore not interchangeable with a game mode value.

RunFrontendStateMachine (`0x426e80`) first applies a pending substate change, then handles one current menu state. Most branches perform one initialization or presentation step and return. Changing the state does not recursively execute the new state's branch in the same call. Many stable substates only present a frame; GUI processing and application callbacks are performed through the surrounding [frame machinery](game-main-loop.md) and [GUI runtime](gui-gui.md).

MenuFrame in [main_loop.cpp](../../../src/game/main_loop.cpp) calls this machine, then selects a campaign setup, loading or prebattle frame when the relevant mission type/start flags are present. It finally composites menu layers. The machine's setting of network/game-start bit two is a handoff request, not direct world creation. [Game loading](game-game-load.md) owns world startup and [game state](game-game-state.md) owns the wider lifecycle.

SetFrontendSubState (`0x4257e0`) assigns both current and requested substate. SetFrontendState (`0x425860`) assigns the menu state and resets both substates to zero. ApplyPendingSubState (`0x425930`) makes current match request only when they differ. The machine also contains inlined forms of these same operations. CheckFrontendStateChange (`0x4256d0`) and these setters would show a checksum-error message including call-site file/line when CodeChecksumFailed is nonzero. In this revision CodeChecksumFailed (`0x428bc0`) always returns zero. These checks do not currently verify code bytes, block transitions or authenticate assets.

ResetFrontendState (`0x4259b0`) selects state zero, clears substates, chatMode, address-dialog flag bit zero and pending error text. It does not clear every connection flag. EnterMainMenuState (`0x425a90`) selects state two, clears substates and calls LoadPictureCached with a null name to detach the current background. Neither helper itself opens MAINMENU.GUI.

## Top-level transition table

All numeric state/substate values below are decimal. Only listed cases execute actions; there is no default recovery for an unexpected value.

| Menu state | Substates and resulting work |
| --- | --- |
| 0, initial movies | Disable cursor overlay. In fullscreen, play movie slot one when playMovie is set, then select state 1, clear playMovie and save settings. Otherwise play slot one unless the command-line intro skip is set, then select state 2. Windowed mode goes directly to 2. |
| 1, intro | Play movie slot two, then select state 2. |
| 2, main menu | Pops one key first. Substate 0 attempts a lobbied connection; success marks network/lobby flags and selects state 16/substate 18. Otherwise set mission type zero, clear lobby flag, open main menu, then select stable substate 1 or automatic multiplayer substate 6 according to DAT_00512c80. Substate 1 presents; 5 selects state 7 with bit three set; 6 sets mission type three, clears bit three and selects 15; 7 restarts state 0; 9 selects credits state 3; 8 blanks and quits. |
| 3, credits | Play movie slot five, then select state 2. |
| 4 and 5, ending movies | Substate 0 advances to 1. Substate 1 plays slot three or four respectively, then slot five, clears start bit two, selects menu state 2 and wider game mode 2. |
| 7, single-player root | Pops one key. Substate 0 opens SINGLE and resets player slots, then selects 1; 1 presents. Substates 10 and 14 open the new-game menu and select state 8/substate 1. Substate 11 loads settings, selects mission type two and state 9. Substate 13 opens options in state 10/substate 1. Substate 3 returns to 2. |
| 8, campaign/new game | Pops one key. Substate 1 presents. Substate 15 selects mission type one and state 11; 16 selects mission type one and state 12; 3 returns to 7. |
| 9, skirmish | Pops one key. Substate 0 opens skirmish and advances to 1; 1 presents; 2 sets start bit two; 3 returns to 7. |
| 10, options return | Substate 1 presents; 3 returns to state 7. Other return policies belong to the options caller. |
| 11, 12, 13 and 14, briefing contexts | Pop one key. Substate 0 opens briefing and advances to 1; 1 presents; 2 sets start bit two. Substate 3 follows the context-specific back path described below. |
| 15, provider selection | Pops one key and handles provider/address/service setup. |
| 16, session selection/join | Pops one key and handles network session creation, joining and score-reporting staging. |
| 17, battle room | Handles room updates, unit-sync completion, start handoff and leaving. |
| 20, address dialog | Substate 1 waits for address-dialog completion bit one; attempts the service when set, then presents. |

Briefing back paths preserve why the briefing was opened. State 11 reopens new-game menu with argument zero, state 12 with one, both returning to state 8/substate 1. State 13 hides the cursor, opens the end-mission screen and selects wider game mode/endgame state seven before restoring the cursor. State 14 selects wider game mode two and returns to single-player state seven. These are distinct paths despite sharing the same briefing UI.

## Provider setup and session transitions

ConnectToService (`0x426d20`) calls InitNetConnection. Success sets network flag bit zero and returns one. Failure retains the service error text, selects menu state fifteen with zero substates and returns zero. The machine's inline UseService variants have the same policy. They do not implement transport I/O here.

State 15/substate 0 clears address-dialog bits zero and one, retries lobbied initialization, and on success enters 16/18 with network bit zero set. Otherwise it fills the provider list and calls SelectConnection with `-1`; a selected provider advances to substate two, otherwise stable substate one. Substate thirteen opens options in 10/1; substate three returns to the main menu. In substate two, exact sixteen-byte GUID comparisons distinguish modem, serial and TCP/IP, each opening its address dialog in state 20/substate one. Other providers attempt InitNetConnection directly and enter 16/0 on success.

State 20/substate one waits for address-dialog bit one. Successful service use enters 16/0. Failure clears both address bits and returns to 15/0. The frame is presented even after those transitions. Completion signaling is owned by the address-dialog callbacks, so this polling branch has no timeout.

State 16 uses these stages:

| Substate | Behavior |
| --- | --- |
| 0 | Enable guaranteed packets, select substate one and reset players. For modem/serial, address bit zero selects hosting substate seventeen directly; otherwise reset the selected session identifier. Open the game-selection dialog and show retained errors. |
| 1 | Present and show retained errors. |
| 17 | Create a network game; if local-player creation succeeds, add its ID. Select state seventeen regardless of local-player creation success. |
| 19 | Set local player-info bit six, then fall through to the join logic of eighteen. |
| 18 or fallthrough from 19 | Join the selected session for the local index. Failure resets substate zero. Success from actual substate eighteen blanks/resizes the screen and attempts score reporting, selecting twenty on success or twenty-one on failure. Entry from nineteen skips that reporting attempt and selects twenty-one. |
| 20 | Present while the reporting flow supplies further progression. |
| 21 | Set the local host field from the lobbied connection flags, or zero without a connection. Copy flags_2b4c bit four into a player flag. If that bit is set, load the named mission, add active type-one/type-two players, and set start bit two. Otherwise select battle-room state seventeen. |
| 3 | Close the network session and return to provider state fifteen. |

The substate-twenty-one body additionally tests whether the current substate equals nineteen, but direct execution of that switch case has value twenty-one. The earlier substate-nineteen branch already sets bit six before joining; the later test does not by itself preserve a hidden origin value. Host assignment shifts the connection flags right by one before storing to the host bitfield.

State 17/substate zero opens the battle room when start bit two is clear, reports events one and two, selects stable substate one and broadcasts player info. If start is already set, it finishes unit sync, selects substate seventeen and broadcasts. Substate one updates the room and presents; when start becomes set it finishes sync, closes the top layer and selects seventeen. Substate seventeen removes player slots of type four, finishes sync again and sets start bit two.

Leaving through substate three keeps network bit zero set, quits the HAPINET game, reinitializes the packet manager with `(2, 100)`, deletes unit sync, reports event eight and shuts down score tables. A lobbied connection calls LeaveNetGame. Otherwise service-provider indices zero or three return to state fifteen; other indices return to state sixteen. Creation/join failures and deferred reporting are therefore distinct from a complete transport teardown. Packet semantics, synchronization and provider implementation remain responsibilities of the network groups.

## Main-menu construction and activation

OpenMainMenu (`0x4263b0`) closes every existing GUI layer, hides the cursor, blanks/presents, and forces 640 by 480 surfaces. It loads MAINMENU.GUI with flag `0x80`, installs HandleMainMenuClick plus UpdateMenuSparks as the periodic callback, and loads FrontendX as its background with palette activation. It starts looping BGM, selects CD music category four, enables key commands, loads palettes/guipal.PAL, builds the GUI color remap against the cached background palette, frees the temporary palette, and redraws. Font Comix and keyboard input are selected.

It sets DebugString to the literal version v3.1, shifts that control's x by half the measured text width, restores the cursor, clears queued mouse events and allocates/zeros 1300 bytes for 100 spark records. Required GUI, palette and gadget lookup results are trusted; the function has no complete missing-asset unwind.

Startup notices have different latches. The Windows CD-player prompt latch is set only when a player window is found and the prompt opens. The DirectX check latch is set before checking version arguments `(4, 5, 0, 0x9b, 3)`, so the warning is not retried on each menu opening. The no-sound latch is set only when a missing-driver message is shown. The GPF check runs once and sets its latch afterward. These globals persist independently of the individual layer.

HandleMainMenuClick (`0x425d80`) receives the Gui-shaped selection context. A selected index of `-1` is the layer cleanup notification and frees g_menuSparks without clearing that global. Other activations are:

| Name | Action |
| --- | --- |
| SINGLE | Play BigButton, set cursor mode `0x14`, request substate five. |
| MULTI | Play BigButton, set cursor mode, register archives and attempt maps/multiplay.tdf. Success requests substate six and blanks/presents. Failure shows the multiplayer-CD prompt and clears selection to retain the underlying screen. |
| INTRO | Play smlButton. Require fullscreen and a found disc; on failure show a prompt and clear selection. Otherwise set cursor mode, set introMovieLoop from Windows Shift state, drain keys and select menu state one with zero substates. |
| EXIT | Play exit, set cursor mode and request substate eight. Actual quitting occurs in the state machine. |
| Credits | Play smlButton, enforce the same fullscreen/disc checks, set cursor mode and request substate nine. |
| Anything else | Clear selected gadget to prevent automatic layer closing. |

The CD requirements above are frontend gates using the existing [CD-check](game-cd-check.md) policy. They do not establish that every movie asset can later open.

## Windows CD-player prompt

OpenCloseCdPlayerDialog (`0x426200`) loads YESORNO.GUI with flag `0x100`, enables keyboard input, finds type-one CHOICE1/CHOICE2, installs these names as accept/cancel metadata, translates Yes/No and the title, selects CHOICE1 and installs HandleCloseCdPlayerChoice. It redraws with flag `0x40`. This layout uses different control names from the generic CHC1/CHC2 constructors; the asset must match its caller.

HandleCloseCdPlayerChoice (`0x426190`) ignores cleanup index `-1`, otherwise plays SmallButton. CHOICE1 closes the external Windows CD-player window through Sound. CHOICE2 leaves it running. Unknown choices clear GUI activation. Closing the external player is the only external-window action in this callback; it does not close the game window, despite an older source comment.

## Menu sparks operate on pixels

UpdateMenuSparks (`0x425b80`) is a 100-record artwork animation, separate from [battle particles](graphics-particles.md). Although provisional local structure names refer to terrain, cells and smoke, their fields alias GUI surface pointers: layer surface is the original background, and gadget-header offset `0xbc` points to the rendered screen surface. [Surface layout](../../../src/graphics/surface.h) puts pixels at offset `0x0c`, the exact pointer dereferenced here. The low-nibble tests therefore classify image pixels, not terrain features or live wrecks.

Each thirteen-byte record stores short x/y, active byte, signed-byte velocity, byte life/fuel and a linear pixel index. An active spark restores its old pixel from the original background, moves by its velocity and dies outside a fixed 640 by 480 extent or when life was already zero. Otherwise it decrements life and updates the index using a hardcoded 640-byte row. A destination pixel with low nibble below thirteen kills it; other pixels become palette index `0xaa`. Fuel exhaustion changes horizontal motion to vertical or vertical to horizontal, with parity choosing a direction of magnitude three, and reloads fuel to `1..16`. Other active steps decrement fuel.

An inactive spark chooses x in `0..639`, y in `0..219`, and activates only when the current destination pixel's low nibble exceeds twelve. Life receives rand plus one narrowed to a byte, so wrap can produce zero; fuel is `1..32`. Life/x/y parity chooses its initial horizontal or vertical direction. The callback marks the layer changed after all records whenever a background surface exists. It assumes the fixed image dimensions, pitch and allocated spark table; it has no resolution/pitch adaptation. Painting `0xaa` changes the destination low nibble to ten, so overlapping sparks can encounter a value that fails the admission test until restoration. This is a pixel-animation interaction, not destruction of a map feature.

## Movies, looping and warp/debug controls

PlayMovie (`0x426780`) stops all sounds, resolves Data/name.zrb through the CD-path helper and checks length. If present it blanks/presents, disables cursor overlay and repeatedly allocates a MoviePlayer with parameters `(0, 600000, 1, 2000000, 1)`, calls Play, closes and deletes it while introMovieLoop remains set. After the loop it clears the global player pointer, drains keys and blanks/presents again. A missing file does none of the movie/display setup, but sounds were already stopped. The helper does not restore the cursor overlay itself; menu reentry does that.

The [movie implementation](graphics-movies.md) pumps Windows messages. Its WM_CHAR path clears the loop flag; Alt-F4 also clears the flag and posts quit. This is how the Shift-enabled intro loop can end. Movie construction or playback failure does not gain an additional frontend timeout or recovery policy here.

LoadWarpLevel (`0x4268b0`) reads Warp.ini under the start directory: section WARPLEVELS, keys warpNcampaign and warpNmission, defaults default and zero. It selects mission type one, loads the named campaign and, if mission selection succeeds, sets bits three and two. It does not start loading directly or report unsuccessful selection. Fixed path/key buffers use concatenation/formatting without overall bounds validation.

HandleFrontendDebugKey (`0x4269d0`) consumes one key. A/C choose side zero/one; E/M/H choose difficulty zero/one/two; digits repeat the warp selection flow. Other keys return to menu state two and play MAINMENU, except a zero key with no cursor flags before/equal DAT_00512288 waits. Thus the default branch is also a timer/input gate. These shortcuts depend on where this handler is installed; they are not globally available battle commands.

## Ten-entry picture cache and background references

Each cache entry owns a surface, a separately allocated 1024-byte palette and a 32-byte name. FindCachedPicturePalette (`0x4287d0`) and FindCachedPicture (`0x428850`) linearly search ten entries with case-sensitive strcmp; null names return zero and successful lookups do not change order. ClearPictureCache (`0x4287b0`) zeroes all 400 bytes without freeing resources and is initialization/reset, not cleanup.

LoadPictureCached (`0x4288d0`) optionally blanks/presents first when its second parameter is nonzero. A nonnull name is searched and a matching record moved to the front. If its surface is null or no match exists, a palette is allocated and LoadBitmapByName is called. Outside wider frontendState six, entry nine is evicted if its surface is nonnull, all entries shift down and the new record is inserted at zero. In state six the newly loaded surface/palette are not inserted into this cache; ownership must come from the consumer, and this function does not free them.

With fourth parameter zero, the resulting surface is assigned to the current layer's background field, or the Gui fallback when no layer exists. The third parameter uploads the palette only in the layer-present branch. The cached background name is copied on nonnull successful names, using unbounded strcpy; null name detaches the surface, clears the name and returns one. A nonnull failed surface returns zero, but its palette may already have been retained in a failed cache record. A nonzero fourth parameter still performs lookup/loading/cache changes but skips attachment and always returns zero, so that return does not mean no work occurred.

Matching a failed record with null surface first moves it forward and then inserts another copy-name record after a new load, permitting duplicates and stranded palettes. Eviction/free code tests surface before freeing the palette, so palette-only failed records are not reclaimed by that branch. Cache movement copies ownership fields; it is not reference counting, and callers must avoid keeping an evicted surface alive through another pointer.

FreePictureCache (`0x428730`) frees surface and palette for entries with nonnull surface, clears their fields/name and detaches matching Gui fallback or the current top layer's background. It does not search lower stacked layers for borrowed references and does not process palette-only entries. It compares pointers after releasing surface storage but does not dereference the released surface. There is no persistence or network synchronization of cache order.

## Label styling, hooks and coverage

OrLabelAttribs (`0x428b60`) ORs attribute eight into every type-five gadget of the current layer and assumes it exists. It does not render immediately.

The remaining entries are intentional stubs in this revision: EmptyPreFrontendInitHook at `0x428bb0`, three EmptyPostSimStepHook variants at `0x428bd0/0x428be0/0x428bf0`, and three EmptyMainLoopHook variants at `0x428c30/0x428c40/0x428c50` do nothing. MainLoopContinueStub and its two variants at `0x428c00/0x428c10/0x428c20` always return one. Together with the checksum stub, they supply no hidden networking, validation or simulation work.

All 37 main-file entries and the separate checksum entry have been traced. Screen-specific data, network protocols and endgame decisions remain covered by their owning modules rather than inferred from these calls. Asset names and parameters establish the frontend contract; actual artwork, installed providers, movie files and UI behavior were not executed. No source listings or build/dependency operations were needed.
