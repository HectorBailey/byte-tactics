# Smacker movie playback, output surfaces and input handling

[Systems manual](../index.md) | [Roadmap](../../doc-writeup-roadmap.md)

## 1. Scope and module boundary

The graphics/movies group owns 12 entries in [movies.cpp](../../../src/graphics/movies.cpp). Its address intervals are `0x47bdf0` to before `0x47c4d0`, then `0x47c6c0` to before `0x47c790`. The intervening WriteSmackStats method at `0x47c530` belongs to debug/memory_stats despite residing in this file. Two build-site getters at the end belong to graphics/movies by the inventory's address grouping even though they perform no movie work.

The game delegates compressed movie decoding, frame pacing and movie sound to imported smackw32.dll entry points. Their locally recovered names include SmackOpen, Close, DoFrame, NextFrame, Wait, Goto, ToBuffer, ToBufferRect, SoundOnOff and SoundUseDirectSound. This module describes the game's calls and state around that DLL. It does not contain the codec's bitstream decoder, sound mixer or complete timing implementation, so those internal algorithms cannot be reconstructed from this source alone.

## 2. Caller lifecycle and asset selection

PlayMovie (`0x426780`) in [frontend.cpp](../../../src/frontend/frontend.cpp) is the surrounding frontend adapter. It stops current sounds and constructs a CD-file path under Data using the requested movie name and zrb extension. It checks HAPI_FileLengthByName and skips playback when the file is absent/zero-length. That missing-file path differs from the fatal failure when a nonempty path reaches SmackOpen but cannot be opened by the decoder.

For an available movie it installs the game's offscreen Surface, fills black, presents it and disables cursor overlay. It allocates a MoviePlayer, runs Play, explicitly closes and deletes the player, and repeats by constructing a new player while introMovieLoop remains set. This is caller-controlled looping, not an automatic seek-to-first-frame in PlayFrame. After the loop it clears the game's player pointer, drains queued keys and clears/presents black again using the offscreen Surface. This adapter does not locally reenable cursor overlay; later frontend state handles its own presentation setup.

Frontend state code supplies opening/campaign movie names and decides whether introduction playback is enabled or skipped. The movie module does not interpret mission victory, unit construction or shipyard behavior. A movie can depict such events, but its frames do not execute their simulation systems.

## 3. Player state and constructor

MoviePlayer stores the decoder pointer, last full-frame marker, stopped flag, window handle, 256 PALETTEENTRY values, a pixel-format result, an output-ownership flag, embedded DirectDraw pointers, a wrapper pointer selecting those or borrowed game pointers, and a surface descriptor. The decoder view exposes width/height/frame count, changed-palette flag, RGB bytes, palette type, current frame number and last updated rectangle.

The constructor (`0x47bdf0`) clears stopped and frame. If its first option is nonzero it calls SmackSoundEnable with the second option converted to a window handle. It gives Smacker the game's DirectSound pointer, opens the supplied path with flags 0xfe100 and extra argument -1, and calls FatalError if opening fails. It enables movie sound exactly when that DirectSound pointer is nonnull. The remaining constructor options are not consumed by the body; their names/large values at the caller do not establish implemented buffering policies here.

Output selection uses two conditions:

| Display state | Selected output and ownership |
| --- | --- |
| Fullscreen flag set and display DirectDraw object present | Borrow display's DirectDraw pointer block; hasSurfaces=0 |
| Fullscreen flag clear and display DirectDraw object absent | Zero embedded pointer block, create private output with SetupDirectDraw; hasSurfaces=1 |
| Fullscreen flag and object presence disagree | Neither branch establishes wrapper/ownership in this constructor |

The last condition is an unhandled inconsistent-state case, not a fallback branch. A failed private setup is fatal. After output selection, the constructor clears the primary with a black color-fill Blt, using WAIT|COLORFILL. It assumes a valid primary pointer and does not test that Blt result. It does not clear all palette-entry flags or call ReadSystemPalette on the borrowed-output branch.

## 4. Private DirectDraw output setup

SetupDirectDraw (`0x47bf70`) computes the window client origin in screen coordinates but does not subsequently use that local point. It creates DirectDraw through the shared thunk and requests cooperative-level value 8. Contrary to an overview comment about exclusive fullscreen, this body does not set a display mode or create a flipping/back-buffer chain. It constructs a descriptor with size 0x6c, flags 1 and primary-surface capability 0x200, then asks for a primary surface.

On successful primary creation, GetBlitMode selects a pixel-format result. A zero result makes it read the system palette, mark its entries, create an eight-bit palette and attach it when creation succeeds. The window is resized to the movie width/height with the no-move positioning flag. The body does not derive this window size from the game's current render viewport.

If the decoder reports a changed palette, setup opens a Smacker buffer with fixed dimensions 640x480, supplies its RGB bytes/palette type and asks SmackColorRemap to use fields of that returned buffer. The buffer pointer is local and no corresponding buffer-close call appears here. The allocation and returned fields are used without a null guard. The codec's internal ownership behavior is outside this source; this routine does not show an explicit release of that buffer.

DirectDraw creation failure and cooperative-level failure return zero; the latter first releases the created DirectDraw object. Other failures are less comprehensive: primary-surface creation can fail and the function still reach return one. Palette creation/attachment results likewise do not all determine the returned status. The constructor can therefore proceed to its unconditional primary Blt after a nominally successful but incomplete setup.

## 5. Pixel formats and palette state

GetBlitMode (`0x47c150`) ignores its pointer-shaped parameter and queries wrapper->primary through method slot 21. It returns zero for an eight-bit pixel format. Four recognized RGB mask layouts return these flags:

| Red / green / blue masks | Returned mode |
| --- | --- |
| 0xf800 / 0x07e0 / 0x001f | 0xc0000000 |
| 0x7c00 / 0x03e0 / 0x001f | 0x80000000 |
| 0xf800 / 0x07c0 / 0x003f | 0xa0000000 |
| 0xfc00 / 0x03e0 / 0x001f | 0xe0000000 |

Other masks display an unsupported-pixel-format message and also return zero. Zero therefore conflates a supported indexed format and the warned unsupported case. The four mask tests follow the eight-bit check; the function does not independently require a 16-bit count before each mask match. These result values are stored during private setup, but PlayFrame does not directly branch on paletteResult or explicitly combine it into the final decoder call.

ReadSystemPalette (`0x47c230`) reads all 256 system colors through a window DC, releases that DC, clears flags on entries 0..9 and 246..255 and marks 10..245 PC_NOCOLLAPSE. Its integer argument is unused. The equivalent sequence is also inlined in SetupDirectDraw's zero-result path.

UpdatePalette (`0x47c2a0`) copies all 256 RGB triples from the decoder into the player entries, preserving their existing flags, then calls wrapper->palette->SetEntries. PlayFrame repeats that copy when newPalette is nonzero. Neither path tests for a null palette or propagates SetEntries failure. In particular, the private nonzero pixel-format path need not create wrapper->palette, so these routines do not establish a generally safe changed-palette path for every accepted format. Borrowed display palette and player-entry flags depend on earlier initialization.

## 6. Decode target and frame presentation

PlayFrame (`0x47c3a0`) returns immediately when the supplied window does not have focus or when stopped is already set. Otherwise it updates a changed palette, installs g_game's offscreen pointer through SetOffscreenSurface, calls SmackToBuffer, decodes the current frame with SmackDoFrame and calls FlipScreen.

The offscreen pointer is allocated by AllocSurface in [game_load.cpp](../../../src/game/game_load.cpp) and [game_state.cpp](../../../src/game/game_state.cpp). Accordingly its field at offset 8 is pitch and its field at offset 12 is the pixel pointer, even though this file's partial view calls them width and height. The actual seven stack values passed to SmackToBuffer are the decoder, zero, (480-movieHeight)>>1, offscreen field 8, movieHeight, offscreen field 12 and zero. Reading the misleading local field names as a second width/height pair or the final zero as proof of a null output buffer would misdescribe the call. The pitch and pixel-pointer values are supplied through the ordinary Surface prefix.

The fixed 480 in the vertical offset centers relative to that historical height, not the current screen-height getter. There is no horizontal centering offset, arbitrary-resolution scaling, local movie-size clipping or explicit rejection of a movie taller than 480. A negative vertical offset can therefore be passed for oversized input. Asset dimensions and backing buffer suitability are caller/decoder prerequisites.

[Surface presentation](graphics-surface.md) determines whether FlipScreen copies the override image into a DirectDraw primary or into the cached GDI bitmap and then to the window. The movie player does not itself copy every decoded pixel with a custom renderer. Cursor overlay was disabled by its frontend caller; the shared presentation machinery still owns that overlay policy.

After presentation, SmackToBufferRect is queried. Only a reported update exactly covering the complete movie rectangle at origin 0,0 causes frame to become decoder frameNum+1. That marker records the last complete update suitable for later repaint seeking; it is not incremented after every partial frame. If frameNum equals frames-1, stopped becomes one and the routine returns without NextFrame. Otherwise it advances the decoder with SmackNextFrame. The final frame is presented before stopping. Malformed zero frame counts or failed decoder operations are not locally recovered.

## 7. Clearing and repainting

ClearScreen (`0x47c2f0`) performs the same black primary-surface WAIT|COLORFILL Blt as the constructor, without testing its result or changing stopped/frame state.

OnPaint (`0x47c330`) uses BeginPaint, black PatBlt over movie width/height and EndPaint. If a nonzero full-frame marker has been recorded, it disables decoder sound, seeks to that frame marker and enables sound again. It does not directly call DoFrame or FlipScreen during this handler; subsequent playback services handle further decoding/presentation. It forces sound on after the seek rather than restoring a remembered prior sound-enable value. A zero marker produces only the black repaint. The handler does not rebuild lost DirectDraw surfaces itself.

## 8. Playback message loop, stopping and focus

Play (`0x47c6c0`) loops until stopped. With a queued Windows message it removes one message, handles two explicit stop conditions, then normally translates/dispatches it. With no message it asks SmackWait whether the next frame is due and calls PlayFrame only when that function returns zero.

The explicit stop conditions are:

- Message 0x104 with wParam 0x73, the system-key F4 case: set stopped, clear introMovieLoop, post quit and return immediately without dispatching that message.
- Message 0x102, character input: set stopped and clear introMovieLoop, then still translate/dispatch that message before the loop condition ends playback.

These are not mouse-move/button codes despite a misleading description above the body. Ordinary mouse input does not directly stop the movie through these tests. A key-down can later generate a character message through TranslateMessage, but the loop does not treat every raw key-down as an immediate stop. There is no separate WM_QUIT branch that exits this loop merely because such a message was retrieved.

Loss of focus causes PlayFrame to return while leaving stopped unchanged. The outer loop continues processing messages and consulting SmackWait; there is no Sleep or blocking wait in the empty-queue branch. Frame timing remains delegated to the DLL and focus gating does not establish a separate paused-movie state or local sound-pause operation. Natural end-of-file stops the player without clearing introMovieLoop, allowing the frontend wrapper to construct another player for a requested loop.

## 9. Closing and ownership limits

Close (`0x47bf20`) calls SmackClose and, only when hasSurfaces is true, releases a present palette, primary and DirectDraw object. Borrowed game objects remain owned by the display module. No back-buffer/clipper release appears in this private close sequence, but private setup does not create those objects. Fields are not nulled and the method is not an idempotent repeated-close interface. The frontend calls it once and then deletes the player.

Fatal constructor paths call the display FatalError routine, which exits the process rather than returning a recoverable MoviePlayer failure. Normal completion/input cancellation reaches Close through the frontend wrapper. This group does not persist movie position, transmit frame state to multiplayer peers, save palette ownership or resume a partially decoded movie after a saved-game load.

## 10. Build-site getters and coverage

GetBuildSiteMetal (`0x47c770`) returns g_lastPlaceMetalSum. GetBuildSiteHeight (`0x47c780`) returns g_lastPlaceHeight. They perform no recomputation, validation, movie update or terrain query; construction/placement code produces those shared values. Their presence here is an address-group boundary detail rather than an interaction between movie playback and ship construction.

All 12 owned entries were inspected, together with frontend allocation/loop/close, offscreen allocation and shared presentation. The adjacent statistics writer was also read but is accounted for by its diagnostic module, preventing double-counting. Decoder internals and actual driver/asset results remain outside static evidence. No builds, installations, game execution or source listings were used in this documentation.
