# CD audio devices, track policy, categories and fades

[Systems manual](../index.md) | [Roadmap](../../doc-writeup-roadmap.md)

## 1. Scope and state

The sound/cd_audio group contains 34 functions in [cd_audio.cpp](../../../src/sound/cd_audio.cpp), from `0x4cd9b0` to before `0x4cee50`. It uses Windows MCI cdaudio commands for physical-disc playback and shares the [Sound object](../../../src/sound/sound.h) with [DirectSound samples and mixer services](sound-samples.md). It does not decode CD tracks into the game's sample buffers.

State includes open, logical track count, first-data-track offset, current/locked track, playback order, selected category, a 100-byte category table, playState, enabled flag, disc serial, fade step and a disc-change callback. Global state supplies the shared player pointer, fade/next-track timer handles, fade volume, remembered category tracks and an external CD Player window. PlayState values are zero for stopped, one for active policy and two for paused. They are assigned by game requests and can differ from the device's actual reported mode after a failed MCI command.

GetDiscSerial (`0x4cd9c0`), GetPlayState (`0x4ce020`), GetTrackCount (`0x4ce450`), GetLockedTrack (`0x4ce5a0`), GetTrackCategory (`0x4ce680`) and GetCurrentTrack (`0x4ce7f0`) simply return stored fields. IsCdPlaying (`0x4ce800`) instead asks MCI for its current mode and returns one only for a successful reply equal to playing. That distinction matters to UI/status consumers.

## 2. Opening the device and querying a disc

OpenCdAudio (`0x4ce260`) returns one immediately if already open. Otherwise it installs g_cdPlayer, clears the remembered external window, initializes 100 category bytes to (index%4)+1, resets track/lock/serial/callback/order state and tries to open cdaudio. A failed open enumerates external windows and retries once. Enumeration alone does not close a competing player. Failure of the second open returns zero.

After opening, it stops playback, clears play/fade state, removes both global timer handles and resets them to -1. It requests millisecond time format, queries the disc, installs HandleCdMessage through SetMediaNotifyCallback and marks open/enabled. The final setup again sets lockedTrack=1, currentTrack=0, playbackOrder=1, category=0 and callback=null, even if QueryDisc briefly selected track one.

The millisecond-format failure path closes the device only if open is already nonzero, but this initializer has kept open zero until the later success point. Thus an MCI device successfully opened earlier in the function need not be explicitly closed on that failure. There is no all-or-nothing device-acquisition wrapper around every command.

QueryDisc (`0x4cda00`) clears currentTrack/playState, finds the first CD-ROM drive letter and obtains its volume serial through [hpi.cpp](../../../src/util/hpi.cpp). No discovered drive leaves the previous serial unchanged in this method. That drive search is separate from MCI's default cdaudio selection, so multiple-drive configurations are not explicitly correlated by a selected drive identifier here.

It asks MCI for number of tracks, switches to TMSF format, and asks whether physical track one is audio. An audio reply sets dataTrack=0. Failure or any other type sets dataTrack=1 and subtracts one from trackCount, clamped at zero. A positive logical track count selects currentTrack=1. A failed number-of-tracks query returns zero after the initial current/play reset without explicitly clearing an older trackCount/dataTrack. No category-table-capacity clamp is applied to the reported count.

SetCdCallback (`0x4cd9d0`) stores the callback, calls QueryDisc and invokes the callback when nonnull, then returns one regardless of query success. It is not a periodic registration that independently watches hardware; later Windows device messages drive notifications.

## 3. Logical tracks and direct playback requests

PlayCdTrack (`0x4ceb60`) returns one without action when cdEnabled is zero. Otherwise it assigns playState=1. Index zero delegates to PlayNextTrack and returns one. For a nonzero index it asks whether MCI is already playing and avoids a new command when the stored currentTrack matches. Otherwise it stores the logical index, adds dataTrack to produce a physical index, temporarily reapplies cdVolume and switches to TMSF format.

It constructs a play command beginning at that physical index. It appends an exclusive-style to boundary of physicalIndex+1 only when physicalIndex is less than the stored logical trackCount, then adds notify for the main display window. The second method argument, despite callers passing a run length, is unused by the body. Consecutive-category grouping therefore does not itself make this method play the requested multi-track range.

The boundary comparison mixes the adjusted physical index with logical count. With a data first track, this can omit a stopping boundary earlier than a clean physical-count calculation would. No explicit lower/upper index validation occurs before command creation. A failed TMSF-format change returns zero with the already updated logical fields. After issuing play, it requests millisecond format again regardless of play result and returns whether the play command succeeded. The restoration result is ignored.

SelectTrack (`0x4ce8c0`) returns zero if no tracks. An index above trackCount is reduced modulo trackCount, which can produce zero; zero/negative inputs are not normalized to one. In active playState it calls PlayCdTrack(index,1) and returns currentTrack; otherwise it merely stores/returns the index. SetLockedTrack (`0x4ce580`) accepts any value at most trackCount, including zero/negative values. SetPlaybackOrder (`0x4ce7a0`) stores any integer and returns one. These setters do not enforce a complete valid-domain policy.

## 4. Playback-order state machine

PlayNextTrack (`0x4cdb40`) returns immediately for zero trackCount, and does not advance paused state two. Category four explicitly stops, resets currentTrack/play/fade state, removes timers and returns. Categories two/three bypass ordinary order selection and use category matching. Other categories apply playbackOrder:

| Order | Implemented behavior |
| ---: | --- |
| 0 | If previously stopped, return. Otherwise clear playState, query actual mode and, when still playing, stop/reset/cancel timers and return |
| 1 | Keep an already-playing device; otherwise increment stored currentTrack, call PlayCdTrack and then wrap stored currentTrack to one if it exceeded count |
| 2 | Keep an already-playing device; otherwise choose rand()%trackCount+1 and request it |
| 3 | Request lockedTrack when not playing or stored currentTrack differs; change lockedTrack zero to one first |
| 4 | Use category-matching selection |
| Other | Skip selection and reach the common completion tail |

Sequential order increments a nonnegative currentTrack before requesting playback and performs its over-count wrap afterward. An over-count command can therefore be attempted before the stored value is repaired. The passed remaining-track count has no effect in PlayCdTrack. A first request starting from currentTrack zero selects one.

The common tail temporarily restores cdVolume and assigns playState=1, even when a preceding selection request failed or an unsupported order did not start anything. Its category-search exhaustion path first stops/resets/cancels timers and then falls through to that tail, restoring playState to one. Stored active policy is consequently not reliable proof of device playback.

## 5. Category table and bounded category search

SetCategoryOfTrack (`0x4ce7c0`) and GetCategoryOfTrack (`0x4ce7e0`) directly write/read arr_214[index], without bounds checking. CopyTrackTypeTable (`0x4ce3e0`) copies trackCount bytes into the table beginning at element one. The backing array has 100 bytes, and neither operation validates that the supplied index/count fits. Category values are arbitrary bytes at this level.

The category-search branch keeps an already-playing current track when its category matches. Otherwise it chooses r=rand()&15 and scans forward from currentTrack, wrapping at trackCount. Matching tracks decrement r until selection; r zero and one both select the first encountered match because decrement precedes the nonpositive test. The scan budget is (initialR+1)*trackCount. On selecting a matching track it also counts adjacent matching categories until the end of the disc and passes that run length to PlayCdTrack, whose second argument is unused.

If no selection occurs within the budget it stops the device and clears timers before reaching the common active-state tail described above. This is a bounded repeated scan, not a precomputed uniformly weighted playlist of category members.

SetTrackCategory (`0x4ce690`) ignores an unchanged category. Otherwise it remembers currentTrack in g_cdCategorySavedTrack[old] when old is nonnegative, without an upper bound, and stores the new category. This module writes those remembered values but does not read them to resume a category at its old track. Numeric categories two/three or playbackOrder four trigger transition behavior; other requests can change the stored category without immediately starting a fade or new track. Category four is interpreted as stop by PlayNextTrack, rather than a normal playable category even though default table bytes include four.

## 6. Fade transitions and delayed advancement

When the transition condition applies, SetTrackCategory snapshots cdVolume into global fade volume. Transitioning from category four removes existing timers, requests the target auxiliary volume and advances immediately. If a fade timer already exists, it cancels that and the next-track timer, then advances immediately. Otherwise it sets step=cdVolume/-18 and installs a repeating two-tick OnCdFadeTimer.

OnCdFadeTimer (`0x4ce5e0`) adds step to global fade volume. Positive remaining volume is written temporarily through SetAuxVolume. At zero or below it removes/clears its timer, clamps fade volume to zero, sets step=0 and writes silence. For category zero it then schedules OnNextTrackTimer after 0x78 ticks; other categories advance immediately. OnNextTrackTimer (`0x4ce5b0`) removes its own stored handle, clears that handle to -1 and calls PlayNextTrack.

At the normal [30-tick clock](graphics-display.md), two ticks are about 67 ms and 120 ticks about four seconds. Those are requested cooperative intervals, not guaranteed wall-clock callback deadlines. Poll delays and the timer implementation's discarded overshoot affect actual duration. The integer fade step can truncate to zero for a positive cdVolume below 18, preventing a positive fade volume from reaching completion. Timer creation failure is not checked before storing the returned handle.

The mixer setter suppresses nontemporary volume changes while step is nonzero but accepts temporary fade writes. The separate cdVolume remains the desired restored level. Stopping/resetting step without a corresponding restore can leave the physical auxiliary volume at an intermediate level until a later playback/volume request.

## 7. Pause, stop, enable and time queries

PauseCdAudio (`0x4ce910`) returns one without a command when disabled or playState zero. A nonzero pause argument builds pause cdaudio and sets playState=2 before the command result is known. Resume asks for current track/position and, when that track is below trackCount, appends a from current-position to next-track-position range; otherwise it requests plain play. It adds notify and sets playState=1 before sending. Status queries used to build this command are not checked before using their text replies. Return value reflects only the final command's success.

StopCdAudio (`0x4ced40`) issues stop, resets currentTrack to one when any tracks exist or zero otherwise, clears playState/step and cancels both global timers. These state changes occur even when stop fails; the returned integer reflects command success. EnableCdAudio (`0x4cedc0`) stores its argument and performs the same stop/reset/cancel work for zero. A nonzero enable does not automatically start a track.

IsFirstTrackData (`0x4ce460`) switches to TMSF, asks for type of physical track one and treats query failure or any non-audio reply as data. It does not restore the previous time format or assign the stored dataTrack field. GetTrackLength (`0x4ce530`) asks for a track length and converts its reply with atoi; GetCdPosition (`0x4ce880`) does the same for current position. Both return zero on MCI error. Their numeric meaning depends on the current MCI time format, and atoi is not a colon-separated TMSF parser. Format-changing calls elsewhere therefore affect query interpretation.

## 8. Windows notification routing

The [registry utility](../../../src/util/registry.cpp) SetMediaNotifyCallback stores a pointer in display field 0x80. [WindowProc](graphics-display.md) forwards messages 0x219 and 0x3b9 to that pointer. HandleCdMessage (`0x4ce030`) handles them through g_cdPlayer:

- For 0x219 it stops/resets/cancels timers for every received notification. wParam 0x8000 additionally queries the disc and invokes a nonnull disc callback. Other device-event values still perform the reset but do not take that query/callback branch.
- For 0x3b9, wParam one and playState one, it checks actual MCI mode. If the device is no longer playing, it advances policy through PlayNextTrack. Other notification statuses do not advance through this branch.

This mechanism is synchronous Windows-message dispatch, separate from the display's polled software timers. No multiplayer packet is sent when a local disc ends or a drive event arrives.

## 9. External CD Player window and shutdown

FindCdPlayerWindow (`0x4ce1e0`) is an EnumWindows callback comparing class names with SJE_CdPlayerClass. It records a match and always continues enumeration, so the last match wins. HasCdPlayerWindow (`0x4ce1d0`) only checks the stored handle; it does not revalidate window existence. CloseCdPlayerWindow (`0x4ce190`) sends WM_CLOSE and WM_QUIT synchronously to a stored handle, sleeps 500 ms through the utility helper and clears the handle. There is no process termination or returned confirmation that the external program exited.

CloseCdAudio (`0x4ce410`) stops/closes MCI only when open is nonzero and then clears open, ignoring command failures. It does not cancel the two global CD timers or unregister the display callback in this body. Higher stop/shutdown order must account for callbacks holding global player state. [DirectSound teardown](sound-samples.md) calls this method before deleting the shared object at a higher level.

NopRet4 (`0x4cd9b0`) and NopRet4_B (`0x4cee40`) are one-argument no-op entries. They have no hidden notification or audio behavior.

## 10. Activity-driven music consumer

UpdateCdCategoryByActivity (`0x494e70`) is owned by the dialog module, not this group, but supplies an important category caller in [dialogs_493340.cpp](../../../src/ingame/dialogs_493340.cpp). Under its match/loading gates it samples on a greater-than-30-tick interval into a 30-slot ring. After more than ten stable samples it totals the ring and its latest five samples.

From category zero, either total above 50 or recent above 30 requests category one only when the local player also has more than 30 units. From category one, total below ten and recent zero plus more than 60 stable samples requests zero. A changed requested mode resets the stable counter. It then advances/clears the next ring slot and records its sample time. AddCdActivitySample adds to the current ring bucket. These numeric thresholds establish a hysteresis-like low/high-activity policy; the CD module itself does not inspect weapons or determine whether a ship is in combat.

## 11. Coverage and limits

All 34 address-owned functions were inspected, together with shared mixer/timer, display callback, drive serial and activity consumers. The reference distinguishes logical versus physical tracks, stored policy versus actual device status, and requested fade timing versus polled execution. MCI commands and physical hardware were not run.

The group has no match-save writer, replicated music clock or unit simulation effects. Settings and higher game transitions supply persisted preferences/track assignments separately. Documentation is based on existing source without builds, installations or copied source listings.
