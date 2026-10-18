# Options pages, live preferences, panel animation and battle exit dialogs

Status: finished. `frontend/options` owns seventy entries in `0x45b800..0x460e20`: 69 annotations in [options.cpp](../../../src/frontend/options.cpp) and [RestoreSoundOptions](../../../src/frontend/options_45c820.cpp) separately. It covers frontend/runtime preferences and the associated help, briefing, settings, restart and surrender screens. Persistent registry values belong to [settings](game-settings.md); device behavior belongs to the audio/display/rendering modules.

## Two contexts and snapshot ownership

Game network flag bit two distinguishes an active battle from the frontend for these UI paths. OpenOptionsLayout (`0x45cfc0`) selects PREFS.GUI in battle and STARTOPT.GUI otherwise, with loader flag `0x80`. Battle use sets the orders-panel loaded bit. It grays MUSIC according to the Sound object's initial device field, enables GUI key commands and pauses a nonmultiplayer battle by setting pause bit zero. Multiplayer options do not set that pause bit. This is local UI policy, not pausing every connected peer.

SaveGameSettings (`0x45cde0`) takes an in-memory undo snapshot: 83 bytes from game offset `0x37ee6`, two LOS bits, current game-speed control, scroll speed, locked CD track and one hundred track categories. Most g_optionsBackup globals are aliases into that packed snapshot, as confirmed in [globals.csv](../../../data/globals.csv), rather than independently allocated objects. Snapshot/undo is distinct from SaveSettings, which persists registry settings.

LoadSavedSettings (`0x45cc50`) restores sound, music, UI and visual subsets using those aliases. It does not simply memcpy the entire snapshot back, and some captured fields/bits have no corresponding restore in this routine. It may also call device/brightness methods repeatedly while restoring subgroups. The snapshot is shared across option pages, not nested per-dialog state; opening another shell can replace the undo baseline.

## Connection setters and generic gadget helpers

SetDirectConnectAddress (`0x45b800`) empties the command-line TCP join-address buffer and appends at most 63 bytes. SetHostGameName (`0x45b820`) stores Boolean host mode and optionally replaces the name with at most 63 bytes; a null name preserves the old string. SetDirectPlayProvider (`0x45b860`) accepts only values one through four into DAT_00512c80, shared with online configuration. None attempts a connection here.

SetGadgetsDisabledByPrefix (`0x45b880`) scans record zero through count inclusive, case-sensitive prefix-matches type-one buttons, sets gray state and marks changed. DeactivateGadgetsByPrefix (`0x45b920`) matches any type and deactivates by name. Empty prefix can match every record, and neither validates the active layer. SetGadgetStatusAndText (`0x45ba60`) sets a control's status, retrieves its name and copies the selected off/on text unbounded into the found record, without a null fallback. ClearGadgetText (`0x45baf0`) writes NUL at record text offset `0xb6`, ignoring its second argument.

SetGadgetTextFromValueTable (`0x45c010`) walks a sentinel-terminated table of format/upper threshold pairs, formats the first row whose threshold is at least the supplied value and updates the named control. Values above all thresholds leave text unchanged. SetGameSpeedLabel (`0x45bf60`) computes a qualitative speed description and a formatted string, but passes the original label to SetTranslatedTextByName, discarding that formatted result. The helper therefore does not actually append the calculated speed qualifier.

## Slider conversion and immediate side effects

SetSliderFromValue (`0x45b9b0`) upper-clamps value at max, scales it to range minus one, and adds one when the floating result has a fractional part before narrowing to short knobPos. It has no lower clamp and divides by max even when zero. ReadSliderValue (`0x45ba20`) returns zero for range at most one, otherwise truncates knobPos divided by range minus one times max. They are quantized conversions rather than exact inverses.

| Callback | Setting and bounds/effects |
| --- | --- |
| HandleGammaSlider, `0x45bd20` | Read GAMMA; apply brightness `0.5 + gamma / 24` approximately, plus both volume levels. |
| HandleEffectsVolumeSlider, `0x45bde0` | Read FXVOL, then the same combined brightness/audio apply. |
| HandleMusicVolumeSlider, `0x45bea0` | Read MUSICVOL, then combined apply. |
| HandleGameSpeedSlider, `0x45c070` | Unless active local observer bit six is set, read GAME, clamp at least one, narrow into speedCtrl and call SetGameSpeed with broadcast argument one. |
| HandleScreenSlider, `0x45c170` | Read SCREEN, clamp at least one, narrow to byte scrollSpeed and mark changed. |
| HandleMaxLinesSlider, `0x45c220` | Read MAXLINES, clamp below zero to zero; show decimal or None. |
| HandleTextScrollSlider, `0x45c330` | Read TXTSCROL; show decimal seconds, without adding the extra second used by HUD expiration. |

ApplyBrightnessAndVolume (`0x45bcc0`) uploads brightness and calls wave/aux volume with integer levels shifted ten bits. It does not normalize/clamp arbitrary settings itself. UI-openers use maxima 64 for FX/music, 20 gamma, 21 game speed, 65 scroll speed, 30 message lines and 20 scroll seconds. These maxima describe slider setup; direct registry/command values can follow other validation paths. [Message HUD](network-net-chat.md) explains how line/scroll settings are actually consumed.

UpdateVideoModeLabel (`0x45bb00`) finds current width/height in the slider's mode list, positions the knob with the same rounding and formats VIDVAL. HandleVideoModeSlider (`0x45bbf0`) reads the list pointer before its nominal gadget-null test, selects the scaled index, sets VIDVAL and writes configured displayWidth/Height. It does not itself recreate DirectDraw surfaces or validate that index. A zero max, as with one available mode, can divide by zero in initialization. No match leaves prior text/knob unchanged.

## Defaults and undo are explicit subsets

| Routine | Restored/defaulted fields |
| --- | --- |
| ApplyDefaultSoundOptions, `0x45c570` | FX volume 27, sound bits four/five/six set, 3D disabled, low mode one, unit speech amount ten, then combined device apply. |
| ApplyDefaultMusicOptions, `0x45c630` | Music volume 32, CD mode four; set music-enable bit and PlayNextTrack only if previously off; combined apply. It does not itself call SetPlaybackOrder for the new mode. |
| ApplyDefaultUiOptions, `0x45c6d0` | Scroll/message lines ten, requested/effective speed ten, scroll speed 32, interface zero, unit speech ten and unit-chat text five. |
| ApplyDefaultVisualOptions, `0x45c740` | Visual bits one through five set, gamma twelve; outside battle choose 640 by 480 and clear bit six; combined apply. |
| RestoreSoundOptions, `0x45c820` | FX volume, sound bits four/five, low mode and unit speech amount; enables 3D only for saved mode two and reapplies device values. Speech bit six is restored from saved bit **five** shifted left, not saved bit six. |
| LoadSavedAudioSettings, `0x45c950` | Music volume, track-category table, CD playback mode, music-enable bit, locked track and combined apply. Toggle difference can call PlayNextTrack before the saved enable bit is installed. |
| RestoreUiOptions, `0x45ca50` | Message/UI/speed/scroll/chat fields from snapshot; directly sets requested/effective speed without SetGameSpeed in this helper. |
| RestoreVisualOptions, `0x45cae0` | Visual bits one through six and gamma; resolution only outside battle; combined apply. |

These setters preserve unrelated bits. Direct default/undo helpers do not all synchronize every device/state consumer in the same way; page reconstruction often invokes slider callbacks afterward. RestoreSoundOptions' bit-five-to-speech behavior and the limited resolution restore are explicit source behavior, not a claim of a perfect undo transaction.

## Music page, track controls and categories

SetLockedCdTrack (`0x45c3d0`) forwards to Sound. UpdateTrackGadgets (`0x45c3f0`) sets TRACKTYPE category, TRACKNUM text or NO DISC and gray state. Category editing requires music enabled and CD mode four; mode three updates the locked track from selected track. ApplyTrackType (`0x45c510`) stores the TRACKTYPE stage only for mode four. [Sound/CD playback](sound-sound.md) defines mode and track-selection behavior.

OpenMusicOptions (`0x45d7c0`) opens a shell, cleans rendering resources and ensures the panel gadget, then appends MUSICRT.GUI with `0x280` in battle or MUSIC with `0x200` plus optmusic4x outside battle. It installs a click handler and TickMusicOptions as periodic callback, configures MUSICVOL and music tab state, refreshes controls/track state, styles labels and redraws. In category mode it also writes the selected gadget category back through Sound, so opening is not entirely read-only.

UpdateMusicGadgets (`0x45d130`) sets NOTRAK from music-enable bit, TRACKMODE from cdMode minus one and grays transport/volume/mode controls when disabled, with TRACKTYPE separately gated on category mode. TickMusicOptions (`0x45d0c0`) parses TRACKNUM and refreshes selected/current track when they differ.

HandleMusicOptionsClick (`0x45d280`) implements:

- NOTRAK copies button stage bit zero into music enable, calls EnableCdAudio and refreshes gates.
- TRACKMODE stores stage plus one and calls SetPlaybackOrder. Mode three selects saved locked track; mode four updates category state. TRACKTYPE changes the selected track category.
- CDPLAY plays the selected track; CDNEXT/CDPREV wrap through `1..GetTrackCount`, pass selection through SelectTrack and refresh; CDSTOP stops playback then selects track one.
- UNDO restores the snapshot music subset, closes/reopens the page. RESTORE applies music defaults and also reconstructs the page.
- Other type-one buttons close this subpage and forward the saved selected index to the underlying layer's handler. Nonbuttons clear selection.

Cleanup index minus one stops CD audio outside battle or calls PlayNextTrack in battle, and clears the loaded bit. Because cleanup can itself change audio, closing/reopening after undo/default is an additional side effect. Missing control/track count assumptions are delegated to assets and Sound, not fully checked here.

## Sound page and speech controls

OpenSoundOptions (`0x45de30`) uses SOUNDSRT.GUI or SOUNDS plus optsound4x, configures FXVOL, and invokes every type-four slider callback after setup without checking callback nonnull. It initializes SPEECH stage from speech-enable and unitChat divided by five, configures MODE and gray states, enables keyboard input and redraws. UpdateSoundGadgets (`0x45d9d0`) provides the same mode-dependent gray/activity setup separately.

HandleSoundOptionsClick (`0x45da90`) stores SPEECH stage as both a nonzero-enable flag and unitChat equal to stage times five. MODE copies the low three bits, stops all sounds for zero, enables 3D only for two, disables it otherwise, and restarts looping BGM for frontend mode one. It then refreshes state and plays the option sound. TEST plays sounds/explode.wav. UNDO/default close and reconstruct the page, with default matching ApplyDefaultSoundOptions. Other type-one controls forward to the underlying shell after closing; cleanup only clears the loaded bit. It does not allocate a new audio device for every slider change or prove 3D support merely from stage two.

## Visual page and display-mode selection

SortDisplayModes (`0x45e4c0`) orders by ascending width then height, preserving no special refresh-rate ordering, and removes entries narrower than 640 or shorter than 480. It does not deduplicate equal resolutions. OpenVisualOptions (`0x45e5e0`) with nonzero argument opens standalone SELVMODE.GUI with fade; zero opens/appends VISUALRT.GUI in battle or VISUALS.GUI plus optvisual4x outside it.

Outside the loaded/runtime path it allocates a 32-byte selection object and 1200 bytes of mode records, asks [display](graphics-display.md) to fill them, sorts and allocates count times 256 name-storage bytes. That name buffer is only initialized at its first byte here; actual slider uses mode records. It binds VIDSLDR with maximum count minus one and current resolution positioning. Missing/zero/one-mode results have no complete fallback, and a failed GetDisplayModes path can leave the list buffer field uninitialized before cleanup. Runtime path uses null data and deactivates MAP/VID-prefixed controls, disallowing resolution choices through this page.

For the full visual page it initializes ANTI, BSHADOWS, SHADING stages and gamma max twenty. HandleVisualOptionsClick (`0x45e100`) copies ANTI to bit one, BSHADOWS to bits four/three/two together and SHADING to bit five. In a loaded battle those changes flush the unit bitmap cache; they do not rebuild source models. UNDO/default restore values and reconstruct the full visual page. Standalone video OK plays a sound and permits normal closing; other type-one controls forward to the lower handler after closing.

Cleanup frees selection-name buffer and mode array only outside the loaded path, then frees selection object, clears layer data and loaded bit. It relies on the object fields being valid and does not independently recreate an applied video mode. [Model/rendering](graphics-model-render.md) and surface lifecycle consumers interpret resulting configuration.

## Speed/interface page

OpenSpeedOptions (`0x45ed50`) appends SPEEDSRT.GUI or SPEEDS.GUI plus optinterface4x, binds the four slider families and maxima above, sets UNITCHAT to unitChatText divided by five and LEFTCLICK to interfaceType, then invokes all type-four callbacks before final redraw. Opening can therefore normalize quantized values and invoke SetGameSpeed, not merely display them.

HandleSpeedOptionsClick (`0x45ead0`) stores LEFTCLICK's stage directly and UNITCHAT as stage times five narrowed to byte. UNDO/default restore UI fields and reconstruct the page. Other type-one controls close/forward to the underlying handler, while cleanup clears the loaded bit. Requested/effective speed snapshot restoration is direct assignment; a subsequent opener callback supplies whatever SetGameSpeed/network side effects occur. Mouse-interface semantics are documented by [battle input](ingame-build-placement.md), not inferred solely from the stage name.

## Shell panel surfaces and animation

EnsureOptionsPanelGadget (`0x45ce80`) adds 150 pixels to screen width whenever loaded bit is set. If PANEL is absent it appends a zeroed type-eleven record at x 128, with width screen minus x, screen height and the header background value. Repeated calls can increase width again even when PANEL already exists. It does not check record capacity.

OpenOptionsPanel (`0x460160`) assumes an existing top layer. It optionally blanks outside battle, allocates a same-size flip surface and copies width times height pixels directly, assuming contiguous rows rather than separate pitch. It creates a frame wrapper, initializes lightbar state, allocates a 300 by 480 backup surface and draws the current default surface into it. It opens the options shell, optionally attaches options4x, installs HandleOptionsPanelClick and takes the shared snapshot described above, then enables keyboard input/redraw. Runtime opening plays Panel.

DrawOptionsScrollBar (`0x45ffb0`) draws only while shellActive. It advances lightbar X by 21 per call toward 277, plays a sound on reaching that limit and can add a LIGHTBAR GAF frame when crossing the saved panel-width boundary. It increases/decreases a six-unit warp amount according to that boundary, constructs source/destination quadrilaterals using fixed coordinates including x 127 and y 479, and calls DrawFrameQuad. It marks GUI/view dirty and field_37e98. This is call-driven panel artwork, not a widget scrollbar measuring list content or a time-scaled animation.

HandleOptionsPanelClick (`0x45fc60`) selects a tab and, outside battle, immediately renders/composites/presents. SPEEDS/VISUALS/MUSIC/SOUND open their pages and then release the captured backup/flip surfaces and clear shellActive through a common cleanup tail. PREV saves persistent settings and sets shellClosing one; CANCEL restores snapshot subsets and also sets shellClosing one. Those two branches return before that common tail. Cleanup index minus one runs the release tail. Unknown controls clear activation and return. The source shows writes to shellClosing but no direct named read elsewhere in the current C++ search, so the flag alone should not be described as an implemented timed close animation.

BlitOptionsPanel (`0x45cf60`) renders/composites/flips only outside battle. CloseOptionsPanel (`0x45fc40`) clears loaded bit and requests frontend substate three, without itself closing every layer. ClearScreenWithHudPalette (`0x45fbc0`) uploads an all-zero 1024-byte palette, locks/fills the offscreen surface with GUI color zero and flips after successful unlock; lock failure leaves the palette change but skips the fill/flip.

## Read-only game settings, help and briefing

ShowGameSettingsDialog (`0x45f1d0`) loads GAMEOPTIONS.GUI with `0x1881`, attaches GameSettings and appends paired labels. It reads host PlayerInfo and local setup resources: commander death has Game Continues/Game Ends/Deathmatch; starts are Random/Fixed; mapping Mapped/Unmapped; LOS True/Circular/Permanent; multiplayer adds cheat/watching permissions, otherwise difficulty. It shows map name, starting metal/energy and max units. Multiplayer resource shorts are scaled by one hundred; nonmultiplayer values come from the local 24-byte setup record. Rule-table indices/host lookup are not bounded here. Its OK handler (`0x45f190`) only plays Options and permits generic closing; the dialog does not change match rules.

OpenBriefingDialog (`0x45f800`) loads BRIEFING.GUI, clears attribute `0x10` on text/pagination controls, attaches igmbrief, allocates fifteen blink slots and delegates text/narration initialization to [campaign briefing](frontend-campaign-menu.md). HandleBriefingClick (`0x45f770`) frees blink slots on cleanup, permits OK closing, or advances page on TextRegion/MOREBAR and clears activation. It does not free the global wrapped briefing buffer in this handler, unlike the mission-briefing cleanup, so repeated initialization can retain/leak that separate allocation.

OpenHelpDialog (`0x45fb30`) loads HELP.GUI with `0x1881`, attaches dhelp, remembers the initial gadget count and fills page zero with seventeen entries. FillHelpPage (`0x45f8c0`) resets count to that baseline, loads gamedata/help.TDF, selects Help and reads Line(page times lineCount) through the next lineCount minus one into 128-byte values. Found rows split at a pipe and append two translated labels at x forty and 125, y starting fifty with eighteen-pixel spacing. A leading pipe substitutes two blank strings; otherwise scanning for the pipe has no NUL/bounds stop, so malformed help rows can run beyond storage. Dynamic controls inherit generic capacity limits.

HandleHelpClick (`0x45fac0`) permits OK close and rebuilds seventeen rows for the Page button stage, clearing activation afterward. These are asset-authored help pages, not a dynamic reflection of every registered keyboard/console binding. Failed file/record load still resets the old dynamic count and marks changed.

## In-game menu, pause and nested actions

OpenInGameOptions (`0x460cc0`) loads ARMOPT.GUI with fade, installs its handler, grays SAVEGAME/LOADGAME for multiplayer and relabels MISSION to Settings for skirmish/multiplayer. It enables keyboard input/key commands, redraws, pauses nonmultiplayer locally and pauses CD audio for all modes. Required control indices are used without a missing-index guard.

HandleInGameOptionsClick (`0x4609b0`) routes LOADGAME/SAVEGAME to [save/load screens](game-savegame.md), PREFS to the preference shell, HELP to the help setup, MISSION to campaign briefing or game settings, and EXIT to exit menu. OK permits generic closing. Cleanup disables key commands, frees any flip/backup surfaces, clears shellActive/loaded bit, clears pause bit zero only for active nonmultiplayer battle and calls PauseCdAudio(0). It does not restore an arbitrary previous pause/audio-pause state; opening/closing writes this fixed policy.

## Restart, surrender and quit flows

OpenExitMenu (`0x4608b0`) loads EXITMENU.GUI with `0x1800`, activates/relabels Restart for campaign/skirmish, and for other modes hides MAINMENU when lobbied flag four is set. It does not explicitly deactivate Restart in every other case, relying on layout defaults. HandleExitMenuClick (`0x460800`) closes that layer and opens a surrender prompt for MAINMENU intent zero or EXITGAME intent two; RESTART opens restart confirmation. CANCEL permits generic close; unknown controls clear activation.

OpenRestartDialog (`0x4604a0`) loads RESTART.GUI, attaches drestart, wraps the mission name using a temporary font slot, places at most the first two newline tokens, applies difficulty buttons and redraws. The allocated wrapped buffer is not freed in this body. HandleRestartDialogClick (`0x460340`) requires the appropriate campaign/skirmish disc and returns on failure, mounts archives, copies Difficulty stage and sets restartMissionRequest one. It does not reload the map directly. Unsupported mission types return without setting the request. Difficulty clicks only play/clear activation; CANCEL closes normally.

OpenSurrenderDialog (`0x460680`) loads YESORNO.GUI with `0x1000`, labels choices Yes/No and selects CHOICE2. Both accept and cancel metadata names are set to CHOICE2, so ordinary Enter/Escape defaults favor No rather than Yes. Intent zero asks to return to main menu; intent two asks to exit, with a different lobbied title; intent one leaves the asset title unchanged.

HandleSurrenderChoice (`0x4605c0`) on CHOICE1 selects CD category four. Intent zero/one shuts down ingame systems, unwinds layouts, closes a layer, blanks and selects wider game mode one. Intent two sets endgame flag four and calls ShutdownIngameAndQuit. CHOICE2 has no destructive work and permits normal closing; unknown choices clear activation. HandleBattleQuitPrompt (`0x4609a0`) ignores its argument, sets intent two and opens that prompt. These source-defined UI paths are described, not executed during documentation.

## Coverage and validation boundaries

All seventy owned entries, the standalone sound-restore body, shared snapshot aliases and direct animation/caller links have been traced. Asset dimensions/control names, device availability, translations and valid setting ranges remain runtime inputs. The writeup preserves observed undo, lifetime, parsing and callback limits without changing source, building, installing dependencies or embedding source listings.
