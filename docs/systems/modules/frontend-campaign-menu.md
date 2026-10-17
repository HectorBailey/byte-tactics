# Campaign menus, briefing pages and animated mission artwork

Status: finished. The `frontend/campaign_menu` group owns 34 entries in `0x476740..0x479660`, all in [campaign_menu.cpp](../../../src/frontend/campaign_menu.cpp). It presents campaign choices and briefing data supplied by [game/campaign](game-campaign.md), requests transitions from the [frontend coordinator](frontend-frontend.md), and supplies setup-slot queries used by skirmish. It does not decide mission victory or serialize campaign progress itself.

## Campaign discovery and returned storage

DrawBitmapBackground (`0x476740`) optionally hides the cursor and blanks the display, constructs a bitmaps/name.PCX path but actually passes the original name to LoadBitmapByName, uploads the returned palette, draws the image at origin on the game offscreen surface and frees it. It restores the cursor only when the lock argument was nonzero. It does not itself flip after drawing, and it does not check image-load failure before palette/draw/free calls. The constructed path is not the argument used by this body.

GetSideCount (`0x4767e0`) returns the configured side count from [side data](game-side-data.md). BuildSideList (`0x476830`) allocates count times thirty bytes and appends side names as consecutive NUL-terminated strings plus an empty terminator. Its subsequent character loop preserves each first character and adds `0x20` to every later byte until NUL, without testing for uppercase letters. This approximates title casing only for suitable uppercase ASCII input; lowercase letters, digits and punctuation are transformed too. Name lengths, zero count and room for the final terminator are not validated against the allocation. The caller owns the result, including savegame side-list consumers.

CountCampaignFiles (`0x4767f0`) resolves camps/*.TDF and invokes CountDirectoryEntries, but its declared void body does not expose that count as an ordinary return contract. HasFewerThanThreeCampaignFiles (`0x4774d0`) explicitly returns whether that count is at most two. CampaignExists (`0x476920`) allocates count times 256 bytes, scans names, compares each with exact strcmp, frees storage and returns whether any matches. ListCampaignFiles (`0x4769f0`) returns the count and transfers the allocated name buffer through its output pointer. Despite allocation in 256-byte units, names are traversed as packed NUL-separated strings, not fixed-width records.

BuildCampaignNameList (`0x476a60`) allocates both a scanned-name buffer and an output buffer, each count times 256. For each entry it loads the campaign TDF, selects HEADER and reads campaignside into a 64-byte field. The filename is included when campaignside exactly matches the selected configured side name or the literal ALL. Matching is case-sensitive. It frees the scan buffer, returns the number accepted and leaves output ownership with the caller. Output is only populated by accepted names, with no explicit initial empty-string or final extra terminator for the zero-match case.

The source-name cursor advances only after LoadFile succeeds. A failed load therefore causes subsequent fixed-count iterations to retry the same name, skipping later files. There is no retry-until-success loop or error dialog, and no side-index check. [Directory scanning](gui-file-list.md), [HPI](util-hpi.md) and [TDF](util-tdf.md) define enumeration precedence, parsing and allocation failure behavior.

## Single-player root and the AnyMsn toggle

OpenSingleMenu (`0x4777a0`) loads SINGLE.GUI, installs HandleSingleMenuClick, stores the game as callback data, blanks the screen and attaches singlebg. It sets the local player's side and the next slot's opposing side for configured side zero or one, then selects mission type one. It enables AnyMsn when allMissionsFlags bit zero is set, installs ToggleAnyMission as the text handler, substitutes Spanish Skirmish quick key `0x73`, enables keyboard input, chooses cursor mode `0x13` and redraws. It assumes valid local/next player slots, required layout and control names.

ToggleAnyMission (`0x477510`) compares the first seven characters of the layer's retained typed history with DRDEATH. On a match it toggles allMissionsFlags bit zero and AnyMsn active state, saves the all-missions setting and marks the GUI changed. It does not consume an arbitrary console command or mark missions completed. The history field is shared with generic GUI key processing rather than a separate input dialog.

HandleSingleMenuClick (`0x4775a0`) ignores cleanup index `-1`. NewCamp and AnyMsn require the campaign disc via FindGameCdDrive(0), mount data archives and request substates ten and fourteen respectively. Skirmish requires FindGameCdDrive(1) and requests eleven. Failures open the appropriate insert-disc message and clear activation, preserving the underlying menu. LoadGame opens the load screen and clears selection; Options opens options and clears selection; PrevMenu requests substate three. Sounds and cursor mode `0x14` accompany transitions. Unknown controls clear selection. The coordinator, not these callbacks, chooses the next top-level state.

## New-game layouts and borrowed list data

OpenNewGameMenu (`0x478240`) stores its parameter in g_anyMissionMode, loads NEWGAME.GUI with deferred initialization flag `0x400`, and installs its handler/data. A nonzero parameter uses playanygame4 and always enables the full campaign/mission layout. A zero parameter counts campaigns: at most two uses newcampaign4x and simplified mode; more uses newcampaign4 and full mode. Interpret the actual parameter, rather than deriving it from a button's display label.

Any-mission mode moves Campaign and CampaignKnob to y 308 with height 48 and gives Missions height 62 where those controls exist. The function initializes rendering with cursor hidden. It then finds Side0 and Side1 GAF sequences and repeatedly zeroes frame-zero hotspot offsets for each sequence-count iteration. It always requests frame zero in those loops, rather than visiting each frame. The mutations affect shared GAF frames.

ApplyCampaignSideSelection (`0x477360`) assigns opposing side values to player slots zero and one, choosing Arm/Side0 for side value zero and Core/Side1 otherwise. This differs from OpenSingleMenu's local-indexed pair. ApplyDifficultyButtons (`0x477410`) sets the Difficulty stage and Easy/Medium/Hard status for difficulty zero/one/two and marks changed; OpenNewGameMenu contains the same logic inline. The Difficulty lookup is dereferenced without a null guard.

Full or any-mission layouts activate Campaign/CampaignKnob. The Campaign list callback is FillMissionList only in any-mission mode; otherwise it is cleared. The apparent null test still dereferences the pointer in the else branch. Campaign names are regenerated for the local player's side and attached as borrowed list text. Any-mission mode additionally activates Missions/MissionsKnob, loads the selected campaign and builds its mission list. Initial focus is Missions for any-mission mode, Difficulty for simplified zero-mode, or Campaign otherwise. Keyboard input and redraw are enabled and cursor mode returns to `0x13`.

FillCampaignList (`0x477940`) frees the prior global campaign-name buffer, plays smlbutton, rebuilds it for the numeric side passed through its pointer-shaped parameter, configures Campaign, synchronizes associated controls and marks changed. FillMissionList (`0x4779e0`) similarly frees old mission text, loads the campaign identified by the selected packed string, obtains a new mission list from Mission and configures/synchronizes Missions. The GUI does not own those buffers. Rebuilding either list invalidates previous string pointers and assumes a valid selection even when discovery produced zero results.

## Starting a campaign or selected mission

HandleNewGameClick (`0x477ab0`) uses cleanup index `-1` to free and null both global name buffers. Start or list activation enters the launch path when the active list matches the mode: Missions in any-mission mode, Campaign otherwise. It plays bigButton, requires the campaign disc, registers archives and initializes mission-status tracking. Full layout loads the selected campaign name; simplified layout loads literal Arm Campaign or Core Campaign according to the local side. Mission index defaults to zero, or the selected Missions row in any-mission mode.

Successful SelectMission sets cursor mode `0x14`, assigns the first two player-info color bytes zero and one, saves settings, and requests substate sixteen for any-mission mode or fifteen otherwise. Failure clears activation and stays. The actual map is loaded later through briefing/start and the wider game loader. Disc failure shows a message and clears activation without performing selection.

PrevMenu requests three. Difficulty cycles only valid values zero to one to two to zero, then clears activation; unexpected values stay unchanged. Arm/Side0 and Core/Side1 update gadget status, side indicator color, global side and the local/opposing player sides, then rebuild appropriate lists. The Core branch skips campaign rebuilding in simplified mode; the Arm branch rebuilds even there and explicitly sets Campaign activity from the simplified flag. Those branches are not fully symmetrical. Other actions clear activation to prevent generic automatic closing.

## Briefing text, narration and ownership

StartBriefingNarration (`0x476c70`) requests Mission name slot three with delay `0x3c`; StartGlamourSound (`0x476ca0`) uses slot eight. Both skip wider frontendState six and test only a nonnull returned pointer. [Sound streaming](sound-sound.md) determines the delay's execution and playback lifetime.

InitBriefingText (`0x476d80`) obtains the mission's borrowed briefing string. When present, it sets SOLARSYSTEM's font tab if found, requires TextRegion, selects the side-specific tab/font, sets the pagination-reset flag, allocates a wrapped copy and replaces it with AllocColorMarkupText's result. It draws the first help page, then requests narration outside wider state six. It does not first free an older global wrapped buffer, so repeated initialization needs the surrounding cleanup contract.

AllocColorMarkupText (`0x476cd0`) takes ownership of its input allocation, allocates strlen plus fifty bytes, zeroes the output and copies until NUL or `0xff`. Each ampersand toggles a color-span state and records its following code. At newline inside a color span it changes the preceding output byte to ampersand and emits CRLF plus a new ampersand/code prefix. This intends to close/reopen markup around a wrapped line, but the local remembered code is declared inside the character loop and is not initialized on every iteration in the C++ body. Its capacity estimate is a fixed fifty extra bytes regardless of how many expansions occur. It frees the input before returning; callers must retain only the replacement pointer.

SkipToPageOffset (`0x476e90`) returns the original pointer for page zero, otherwise scans newline count to page times lines-per-page, stopping at NUL or `0xff`, and returns null if no boundary is found. DrawHelpPage has equivalent inline searches. Negative page/line values and multiplication overflow are not normalized.

## Page rendering and blinking color spans

DrawHelpPage (`0x476ef0`) does nothing without wrapped text. On reset it captures the current gadget count and sets page index to minus one; later calls restore that base count before appending page labels, effectively reusing the dynamic record area. It clears blink words, binds the TextRegion blink gadget, selects its side-specific font, and computes lines per page as region height divided by font height plus two. It increments the page, wraps to zero if the calculated start is absent, and looks ahead to decide MOREBAR text: empty for a single page, MORE... when another page exists, BACK TO START at a later final page.

Each line appends an empty TextRegion label with attributes `0x411`, the selected font tab and the side's base color. Positions start five pixels inside the region and advance by font height plus two. Characters copy until newline, NUL or `0xff`. Ampersand spans choose blink colors using R, Y and G codes mapped to offsets three, two and one in a four-color side table; other codes use offset three. The colored span is copied to a 127-byte temporary and registered with AddBlinkWord using period `0x5e`, duration 1.0 and phase 0.25. X is measured from already copied line text so the blinking overlay aligns with the ordinary label.

There is no general bounds check on copied line length, dynamic record count or side color-table index. Span scanning stops at ampersand or 127 bytes, not necessarily NUL/newline. The outer page loop always attempts linesPerPage rows and advances lineStart after hitting NUL, so a short final page can read beyond the text terminator. A zero lines-per-page value prevents meaningful pagination even when the font divisor itself is valid. [Blink storage](gui-file-list.md) has its own allocated capacity, fifteen slots for mission briefing, rather than automatically growing for all marked spans.

## Briefing screen setup and asset mapping

OpenMissionBriefing (`0x478e80`) loads MSNBRIEF.GUI with `0x80`, installs the click handler/data, loads a background named mbrief plus the configured side resource suffix, and rebuilds the GUI palette remap from game palette storage. It assigns Start/PrevMenu as accept/cancel names, clears attribute `0x10` from MOREBAR and TextRegion, disables SOLARSYSTEM and sets SHUTUP stage one. The twenty-byte background-name buffer has no suffix-length guard.

Initial displayed wind is sampled inclusively between mission minimum and maximum and its countdown is `0..63`. Invalid reversed/extreme ranges are not checked. If the mission planet string is Lunar and global side is nonzero, the function appends 2 directly to that mission-owned string. Resource lookup is exact and case-sensitive, falling back to the first row for an unknown planet:

| Planet string | Screen GAF | Panorama sequence | Rotation sequence |
| --- | --- | --- | --- |
| Green planet | Greenbrief | GreenPan | GreenRotate |
| Archipelago | Archibrief | ArchiPan | ArchiRotate |
| Wet Desert | WDesertbrief | WDesPan | WDesertRotate |
| Desert | Desertbrief | DDesPan | DDesRotate |
| Lava | Lavabrief | LavaPan | LavaRotate |
| Red Planet | Marsbrief | MarsPan | MarsRotate |
| Lunar | Lunarbrief | LunarPan | LunarRotate |
| Metal | Metalbrief | MetalPan | MetalRotate |
| Lunar2 | Lunar2brief | Lunar2Pan | Lunar2Rotate |
| Ice | Icebrief | IcePan | IceRotate |
| Lush | Lushbrief | LushPan | LushRotate |
| Slate | Slatebrief | SlatePan | SlateRotate |
| Water World | Waterbrief | WaterPan | WaterRotate |
| Acid | Acidbrief | AcidPan | AcidRotate |
| Crystal | Crystalbrief | CrystPan | CrystalRotate |

After successful screen-GAF load, found PANORAMA and PLANET type-six gadgets receive their found sequences and UpdateSolarSystem/UpdatePlanet callbacks. Rotation initializes the global sequence cursor. Missing optional sequence/gadget pairs simply remain unbound; required title/text fields elsewhere are less defensive. It disables SOLARSYSTEM again, allocates fifteen blink slots, initializes text/narration, enables keyboard input, renders with `0xc0` and chooses cursor mode `0x13`. Artwork frame storage and surfaces remain owned by the GUI/GAF/cache systems.

## Panorama and planet animation

UpdateSolarSystem (`0x478790`) changes the displayed wind by rand modulo five minus two when a per-call countdown expires, clamps it to mission limits, and reloads a countdown `0..62`. This is briefing decoration, not the live simulation wind callback system. It fills SOLARSYSTEM, draws translated Wind Speed and Gravity strings, with gravity scaled by `1/112`, and draws blink words.

With panorama artwork it sums all frame widths, increments a signed-short global scroll offset when GetTicks exceeds its deadline, wraps at total width, and schedules the next increment two game ticks later. It also derives a frame field from ticks divided by three modulo frame count. Zero frame count divides by zero. It fills the panorama area and draws frame-count plus one tiles, using modulo so the last repeats frame zero. This is an extra repeated tile, not an out-of-range GAF request despite a source comment describing a frame past the count. It zeroes each shared frame's hotspot offsets before drawing, then overlays a side-selected Panmask frame at origin.

UpdatePlanet (`0x478b40`) uses a separate GetTickCount deadline of 25 milliseconds. It resets SHUTUP's stage when the stream has ended. With artwork it advances the sequence when GetTicks differs from g_briefingPlanetLastTick, copies its index to the gadget, centers the frame within the control and draws after clearing the region. This body never updates g_briefingPlanetLastTick, so that comparison is not a complete last-tick latch by itself. Missing artwork marks the GUI changed instead.

Both callbacks calculate their fill color through the unusual expression indexing the gadget's colours pointer by the Gui address plus `0x8b2`. The local declaration makes this look pointer-shaped rather than the usual Gui palette-index lookup. The writeup preserves that actual expression as an unresolved representation/unchecked-address issue rather than asserting a clean palette-table contract. Missing masks, frames, dimensions and shared mutable GAF offsets can also invalidate normal-path assumptions.

## Briefing activation and cleanup

HandleMissionBriefingClick (`0x478cb0`) frees blink storage and the wrapped text, then nulls the latter on cleanup index `-1`. Start plays BigButton and requires the campaign disc. Success registers archives, selects cursor mode `0x14`, stops narration, blanks and requests substate two; failure shows a disc prompt and clears selection. SHUTUP plays Options, stops streaming when its current stage is zero or restarts narration at delay `0x3c` when nonzero outside state six, then clears selection and plays SmallButton. PrevMenu stops narration, plays Previous, blanks and requests substate three. TextRegion/MOREBAR advances the page with More sound when text exists; these and unknown controls clear activation so the briefing remains open.

## Setup-slot queries

The final seven entries operate on the separately allocated `0x18`-byte skirmish setup records, not live Unit or Player objects. All trust numSkirmishPlayers and the options pointer:

| Entry | Exact selection/count rule |
| --- | --- |
| CountComputerSlots, `0x4794d0` | Count type equal to two. |
| CountHumanSlots, `0x479500` | Count type equal to one. |
| FindOpenSlot, `0x479530` | First type/id equal to zero, else `-1`. |
| AreAllSlotsEmpty, `0x479560` | One if all type/id fields are zero, including zero configured slots. |
| IsColorTaken, `0x479590` | One if an active/nonzero-type record uses the color and is not the skipped index. |
| FindFreeColor, `0x4795e0` | First color `0..9` absent from every configured record, including inactive records; else `-1`. |
| CountPlayersInAllyGroup, `0x479620` | Count matching allyGroup with nonzero type/active field. |

The difference between IsColorTaken and FindFreeColor is intentional in the description: the latter does not filter empty slots. These helpers do not alter slots, issue alliance packets or validate a match's start conditions.

## Coverage and limitations

All 34 owned bodies and their inline counterparts have been traced. Names, markup, fonts, fixed buffers and counts determine which documented paths can occur. Original-code limits are recorded rather than repaired in this documentation-only work. No build, game execution or dependency installation was performed, and no source listings are embedded here.
