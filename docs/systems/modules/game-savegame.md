# Save/load screens, summary previews and restore handoff

Status: finished. The eight entries of `game/savegame`, `0x491ec0..0x493340`, are in [savegame.cpp](../../../src/game/savegame.cpp). This address group implements the save/load UI, lists and preliminary restore handoff. The actual SaveGameFile serializer, summary-bank helpers and bank opener belong to [settings/campaign persistence](game-settings-campaign.md); live restore belongs to [game loading](game-game-load.md), [unit persistence](units-unit-save.md) and their subsystem serializers.

## Shared UI data and ownership

The screen uses global packed filename/description/side-name lists and one retained radar-preview surface, with list widgets borrowing those pointers. The game holds a save-path buffer and pending HapiBank pointer that can outlive the dialog while world restoration proceeds. The main Gui is aliased as menu/message in local declarations; these names do not establish an independent chat-only or secondary UI context.

All save/load screens use LOADGAME.GUI with different flags, background and active controls. Required named gadgets are generally trusted, and many formatted/copy operations lack bounds checks. The manager does not create independent state for multiple simultaneously open save/load dialogs, so globals and callback ordering define lifetime.

## Discovering saves and filtering descriptions

ListSavedGames (`0x492b10`) resolves the configured save directory's *.SAV pattern, counts directory entries and, if zero, configures GAMES with an empty list and returns null. Otherwise it allocates count times 256 filename bytes, count times 64 description bytes and another filename-sized temporary. It zeroes main buffers, scans names and copies them to the temporary so filtering can rewrite the visible filename list while scanning the original names.

For each scanned name it formats directory plus filename and opens a summary bank. A nonnull Description pointer is accepted, including an empty string. It appends the description to the packed output, increments found and explicitly closes/deletes that bank. Invalid/missing descriptions cause subsequent packed names to shift over the rejected entry. The code does not close/free a successfully opened bank in the missing-description branch. No per-description 64-byte check exists: the aggregate allocation is an estimate, and strcpy can exceed both its 256-byte scratch path buffer and the description storage for long text.

After freeing the temporary, it binds GAMES to the description list and returns the filename pointer only when found is nonzero; the output count becomes found. If all files are filtered, allocations still exist in the globals although the return is null. Repeated ListSavedGames calls replace global pointers without first freeing old buffers. This matters to DELETE refresh. Enumeration ordering and archive/loose-file discovery follow [file lists](gui-file-list.md) and [HPI](util-hpi.md); this function does not separately sort by timestamp or validate every full save account.

## Summary info and radar preview

ShowSavedGameInfo (`0x491ec0`) requires a GAMES gadget. With nonnegative selected index, a nonempty selected description and a successfully opened summary bank, it copies the description into GAMENAME and opens the Radar Image box. It frees the prior preview with FreeSurface, loads a new surface, converts it into a reusable GAF-frame-shaped wrapper and attaches that wrapper to RADAR when successful. RADAR activity reflects new preview success. It does not own the static frame wrapper as a separate allocation.

The displayed fields come from summary data:

| Field | Presentation |
| --- | --- |
| Players and Gametype | Nonzero players plus type one gives Single; other nonzero-player types give Skirmish with player count; zero players gives ???. |
| Campaign and Mission | Campaign type shows Campaign and Mission strings, enabling campaign labels when a campaign string exists. |
| Map | Other types hide campaign labels and show Map as mission text. |
| Game Time | Thirty-Hz ticks formatted as hours, minutes and seconds using divisors 108000, 1800 and 30. |
| Side | Index into the separately built packed side-name list, otherwise ???. |
| Difficulty | Direct index into Easy, Medium, Hard. |

Summary string copies use a 52-byte local name field without length guards; side/difficulty indices are unchecked. Missing optional campaign/mission strings can leave prior displayed values rather than clearing every field. If selection/description/open fails, the fallback clears textual fields, but does not independently clear/deactivate the old radar preview or campaign-label activity. A stale image can therefore remain alongside cleared text. It marks the GUI changed at the common tail and frees a successfully consumed summary through FreeSummaryBank.

SavedGameSelectHandler (`0x492de0`) ignores its callback arguments and invokes this refresh. Summary display is a preview, not a promise that full restoration will succeed. An empty description accepted by listing does not enter the nonempty-description preview path.

## Opening the save screen

ShowSaveGameScreen (`0x493060`) sets game flag_38a51 bit zero, loads LOADGAME.GUI with `0x880`, installs SaveGameScreenHandler and game callback data, and attaches DSAVEGAME2. It creates the save-directory path, lists files, changes TITLE to Save Game and deactivates DELETE when count is zero. It installs the list-selection callback, applies input attribute two to GAMENAME, builds the side-name list, refreshes preview and begins editing GAMENAME.

It enables keyboard input and key commands, ORs attribute eight into labels, disables the surrounding LoadGame control by name and redraws with `0x40`. Input attribute two uses the [text-edit filter](gui-dialogs.md) for alphanumeric/underscore/space/apostrophe on typed input; it is not complete filename validation or a filesystem sandbox, and clipboard replacement has different filtering behavior. Required GAMENAME indices/layer results are not guarded. This opener does not explicitly pause/restore a timer by itself; the surrounding game flag and options/UI machinery own that policy.

## Save actions and deletion

SaveGameScreenHandler (`0x492df0`) treats cleanup index minus one as release: it restores the context field written by SetDescListCleanupFlag to one, frees/nulls filename, description and side lists, and frees/nulls preview through GameFreeThunk. That differs from preview replacement's FreeSurface call. It does not clear every widget's borrowed pointer before generic layer cleanup, so ordering matters.

CANCEL disables key commands when the in-game start bit is set, plays Previous and leaves activation for generic dialog closing. DELETE plays SmallButton, constructs directory plus selected filename, calls RemoveFile without checking its result, rebuilds lists, clears selection and refreshes preview. There is no confirmation, selected-index check or pre-refresh release of the previous name/description allocations in this body.

GAMES, LOAD or GAMENAME activation enters the save path; other actions clear selection. It disables key commands when appropriate, plays smlbutton and, for a nonempty GAMENAME string, builds a .SAV path, uses the same text as description and calls SaveGameFile with time(0) as game ID. It ignores SaveGameFile's returned result and provides no local success/failure message, overwrite prompt, atomic-write stage or rollback. Empty text performs no save, but the callback does not clear activation in that path, so normal GUI auto-closing can still apply. The lookup of GAMES in this branch is stored but otherwise unused.

The [serializer reference](game-settings-campaign.md) describes summary accounts, full subsystem saves, BetweenMissions and bank writing. This UI callback does not itself enumerate units or serialize source code.

## Opening the load screen

ShowLoadGameScreen (`0x4931d0`) loads LOADGAME.GUI with `0x980`, installs LoadGameScreenHandler and game callback data, attaches DLOADGAME2 and lists saves. A null list result closes the new screen and opens a no-saved-games message. Because LoadGameScreenHandler's cleanup notification merely returns, that no-result path does not release filtered-list allocations through that callback.

For a nonempty list it builds side names, configures GAMES, disables DELETE and GAMENAME, installs the selection callback, refreshes preview, enables keyboard input/key commands, styles labels, disables surrounding SaveGame by name, redraws and sets flag_38a51 bit zero. It does not first validate full saved-unit state or require a disc before displaying the list.

## Load acceptance, disc checks and destructive handoff

LoadGameScreenHandler (`0x492360`) returns on cleanup index minus one. CANCEL explicitly disables key commands when in-game, frees/nulls all lists and preview, plays Previous and permits generic closing. That explicit CANCEL path is therefore materially different from a forced layer close or generic cleanup notification.

LOAD or GAMES activation constructs the selected filename and probes its summary. Only Gametype one and two are admitted: campaign requires disc lookup zero and skirmish disc lookup one. Missing disc displays the corresponding message, clears activation and **returns**, unlike some endgame/skirmish Start handlers. Unknown types or failed summary open enter the invalid-save path. After the summary type probe it frees that summary bank before opening the full bank.

On passing those preliminary checks it mounts archives, blanks the screen, switches to hourglass animation if necessary, disables key commands for an active battle, plays SMLBUTTON, records saveName and, if in-game, calls ShutdownIngameSystems. This happens **before** full bank/mission validation. A later invalid save therefore cannot simply resume the previous battle: resources may already be torn down, stats captured and game flags modified. There is no rollback copy of the running world in this callback.

It sets endgame bit three, opens the full save into pendingSaveStore, selects summary account and restores mission type, optional campaign, side and difficulty. Campaign side zero assigns first two player sides zero/one, otherwise one/zero. It requires a nonnull/nonempty Mission string and a successful LoadMissionByName, then copies that name to skirmish setup map storage. The full bank's Mission is required even though noncampaign preview reads Map for display. The initial summary type and full bank type are not compared again as a consistency check.

It obtains Thumbs and copies twenty-five bytes to campaignMissionStatus without guarding a null source. If resulting strlen is not twenty-five, it initializes unknown mission results. Destination termination and string validity depend on retained storage, not a forced NUL immediately after this copy. For skirmish it restores player count narrowed to short and CommanderDeath, Location, Mapping, LineOfSight and LineOfSightType rules with defaults one. Values are not comprehensively range-validated here.

The success handoff sets start bit two, wider frontendState two, handler MenuFrame and LeaveNetGameCallback as close handler; frees/nulls UI lists/preview; and unwinds menu layouts. It retains pendingSaveStore for downstream [game loading](game-game-load.md), which reconstructs actual live systems. Merely returning from the UI is not completed unit restoration.

## Between-missions path and invalid-save cleanup

For campaign saves with a BetweenMissions item present, the handler sets campaign bit three, closes/frees pending bank, clears its pointer and start bit two, restores MenuFrame/close handler and selects frontend menu state fourteen. It tests item presence, not the item's Boolean/integer value. This path proceeds to briefing through the [frontend coordinator](frontend-frontend.md), rather than attempting a full in-battle restore from that bank. It blanks at the end.

The invalid path closes/frees any current pendingSaveStore and nulls it, opens Invalid savegame file and clears selection. It does not release all list/preview globals, restore prior mission/side/difficulty fields, undo archive mounts or recreate a previously shut-down battle. There is no isolation of a newly opened pending bank from a previously retained global bank before this path. Callers must respect the wider save/load lifecycle rather than treating this function as a pure validator.

ShowInvalidSavegameDialog (`0x492330`) is a smaller helper that shows the translated invalid-save message at width 320 and clears selection on the supplied context. It does not itself close a bank or change frame handlers.

## Coverage and persistence boundary

All eight owned bodies, callback cleanup differences and existing serializer/restore dependencies have been traced. The module documents user-facing discovery, preview, save/delete actions and loading handoff; the bank schema and per-system serialization remain linked owners. Actual save files, malformed inputs and UI interactions were not executed. No build, dependency installation or source listing was used.
