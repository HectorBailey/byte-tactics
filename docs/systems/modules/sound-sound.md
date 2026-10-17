# Game sound routing, positional audibility and unit speech

[Systems manual](../index.md) | [Roadmap](../../doc-writeup-roadmap.md)

## 1. Scope, state and backend boundaries

The sound/sound group owns 31 functions in [sound.cpp](../../../src/sound/sound.cpp), `0x47ed40` to before `0x480190`. Twenty-eight implement game-level audio/speech policy. Three adjacent vector/squad helpers share the address group and are accounted for separately below. The message metadata lives in [unit_messages.cpp](../../../src/units/unit_messages.cpp); the source also contains several partial local views of shared engine objects.

This group connects named effects, visibility/camera state, unit requests, speech text and optional network sounds to a playback backend. It does not decode PCM itself. [Sample services](sound-samples.md) own DirectSound voices/streams/mixer state, [Windows fallback](sound-play-sound.md) owns PlaySoundA wrappers, [CD services](sound-cd-audio.md) own physical-disc music, and [sound types](sound-sound-types.md) load definitions/choice arrays.

Globals choose NoDirectSound, UseWindowsSound and a temporary looping request. Game state supplies the Sound object, name/handle arrays, sound categories, effects-volume setting, sound enable flags, chat/text thresholds, current local player, map information and camera bounds. Those globals are not per-unit audio devices or saved playback cursors.

## 2. Initialization and teardown

InitSound (`0x47ed40`) reads NoDirectSound and UseWindowsSound preferences. Nonzero preferences set their globals; the function does not explicitly clear previously set globals when a preference is zero. Selecting Windows playback tests IsWindowsSoundAvailable, which currently always returns one, and disables the DirectSound path. If DirectSound remains enabled, it requests 11025 Hz, 16-bit stereo through the game window. Failure with HasNoDriver true disables DirectSound; other failure calls FatalError.

It then calls OpenCdAudio regardless of the sample-backend selection, without treating that return as a fatal condition, and allocates SpeechQueue with parameters 30 and 150. The constructor (`0x47f960`) clears count and lastFrame, stores interval=30 and field_a5=150. The latter field is not used as an expiry limit by the queue methods in this group. The constructor does not clear every entry in the backing array.

SetNoDirectSound (`0x47efc0`) sets its flag. SetUseWindowsSound (`0x47efd0`) sets both fallback and NoDirectSound flags. Neither method migrates already loaded sample representations or closes a backend. Changing backend interpretation while live handles remain loaded is not a safe conversion operation established by these setters.

ResetSpeech (`0x47ee30`) frees/removes all queued text, resets lastFrame and clears all 24 message next-allowed times. ShutdownSound (`0x47eee0`) clears/frees the queue and nulls its global, restores mixer volumes, releases/deletes Sound and nulls the game's pointer. Its mixer-restore call occurs before the later null test on Sound, so a null sound object is not safely tolerated throughout shutdown. Named effect/category ownership is freed separately by FreeSounds during higher game teardown.

## 3. Loading, freeing and backend representation

LoadSoundFile (`0x47efe0`) returns null when DirectSound is disabled and Windows fallback is not selected. Otherwise it builds sounds/name.WAV. Windows mode returns the complete HAPI-loaded image; DirectSound mode calls Sound::LoadSample to create a sample set. That method's definition is void in samples.cpp while this file declares a pointer-returning method. This recovered return-declaration mismatch is retained; it is not an explicit type-consistent allocation API across the files.

FreeSoundSet (`0x47f060`) frees the raw allocation directly in Windows mode or releases its four-buffer DirectSound set otherwise. The current backend flag determines interpretation at free time, making consistent lifecycle ordering essential. The [effect-registry loader](../../../src/game/data_files.cpp), `0x429470`, owns named-table insertion and duplicate lookup; [sound type loading](sound-sound-types.md) invokes it for allsound.TDF records.

StreamSoundDelayed (`0x47f090`) only forwards to Sound::StreamSampleDelayed when NoDirectSound is clear. It has no Windows streaming fallback. Its delegated implementation uses one global pending name/volume and polled timers, with the limits explained in the sample reference.

## 4. Named and indexed effect playback

The local FindSound helper scans soundCount logical-name records case-insensitively and returns 0xffff when no nonempty name matches. PlaySoundByName (`0x47f1a0`) passes that index to PlaySoundByIndex. PlayLoopingSoundByName (`0x47f210`) temporarily sets the global looping flag, makes the same call and clears it; it does not preserve a nested prior flag value.

PlaySoundByIndex (`0x47f0c0`) treats index 0xffff as no sound. Other indices are used to access the sound handle without a general index-range check. In Windows mode it immediately chooses ordinary or looping memory playback. That branch precedes the DirectSound effects-volume/soundFlags gates and inherits the fallback's void-versus-integer return-declaration mismatch.

The DirectSound branch requires nonzero fxVolume, at least one of soundFlags' low three bits, and NoDirectSound clear. An optional replication request broadcasts a type-0x13 packet before local playback. A looping request delegates to PlayLooping; an ordinary request to PlaySampleSet, both with volume -585 and no position. fxVolume acts as a gate here; this routine does not proportionally scale that fixed argument by its value. The wider mixer/options system supplies other volume behavior.

PlaySoundFile (`0x47f290`) accepts a path directly. Windows mode calls PlayWavFromDisk without applying the DirectSound gates. Otherwise it requires a nonnull/nonempty name, nonzero effects volume, an enabled low sound flag and DirectSound available, then plays the file at -0x249, also -585. Missing/unplayable assets and saturated channels inherit backend failure behavior. This wrapper is not a guarantee that a named effect can always be heard.

StopAllSounds (`0x47f750`) stops the 32 recorded DirectSound voices when enabled and/or invokes StopWindowsSound for the fallback. It does not clear the speech queue, stop the separate DirectSound stream or stop CD music in this body. The name must not be read as a complete teardown of every audio mechanism.

## 5. Positional audibility and fog knowledge

PlaySoundAt (`0x47f300`) immediately delegates to nonpositional PlaySoundByIndex in Windows mode. In the DirectSound path it rejects absent index, muted effects, disabled low sound flags or NoDirectSound before further work. A requested replication packet is then sent before testing local map validity or visibility.

The supplied position has 16.16 world coordinates with signed high-word integer X/Y/Z views. A GetMapCell query using fixed X/Z divided by 2^20 rejects positions outside the terrain cell map. For audibility information it uses 32-pixel map coordinates X>>5 and (Z-(Y>>1))>>5, incorporating the display projection's half-height correction.

If mapFlags bit 1 is set, the current player's explored byte at that cell must be nonzero. Otherwise the shared visibility mask must contain that player's bit. Bounds comparisons against unsigned explored dimensions reject negative coordinates as well as too-large ones. The first policy can permit sounds from explored ground without current live sight; the other requires the live information bit. A model's rendered visibility and an audio request's visibility test are separate decisions.

There is no unit-type naval exception in this function. A ship's gun/explosion sound reaches the same map-information gate when requested positionally; whether a unit can actually fire at a particular contact remains [weapon logic](weapons-weapons.md). Sound does not authorize targeting.

## 6. Camera-relative 3D and ordinary near/far volume

For an audible position with 3D enabled, PlaySoundAt supplies a vector relative to the camera center: X is world X minus scrollX and half the view width in pixels; its Y is zero; its Z is camera-center Y plus half world height minus world Z. It sets minimum distance from the average viewport tile dimension times 16 and maximum from the sum of map tile dimensions times 16, then plays at -585. The backend may fall back to ordinary playback if its buffer does not expose a 3D interface.

Without 3D, it compares integer X/Z with the scroll rectangle, whose size is viewWidthTiles*16 by viewHeightTiles*16. Inside or on the boundaries it uses -585; outside it uses -1585. This is a two-level camera-region attenuation, not a continuous distance curve. The non-3D test uses Z directly, whereas the fog cell and 3D vector include height corrections.

PlaySoundAtByName (`0x47f610`) resolves a logical name through FindSound and forwards the position/replication arguments. It does not add another sight policy or distinguish an underwater listener. The asset/backend supplies the actual audible waveform.

## 7. Replicated sound packet discrepancy

Both game send paths use an 18-byte type-0x13 record: type byte, flag byte, 32-bit index and twelve remaining bytes. The positional sender initializes those last bytes from XYZ. The nonpositional sender's local record leaves those three words uninitialized before broadcasting the full structure. Both senders set flag=1.

The receiver in [net_game_453d40.cpp](../../../src/network/net_game_453d40.cpp), message case 19, treats any nonzero flag as a call to PlaySoundByIndex(index,0). Only flag zero calls PlaySoundAt(index,position,0). Thus positional sends currently select the receiver's nonpositional branch despite containing a position. Remote playback does not apply the sender's positional fog/camera tests through that packet path. The positional sender can also broadcast before later rejecting its own local position, so a local rejection does not imply peers received no request.

Receiver calls suppress rebroadcast by passing zero. These are presentation requests keyed by the shared sound registry, not damage events or a replicated audio sample stream. Their behavior depends on compatible sound indices/assets on peers. This reference records the actual flag mismatch rather than assuming the packet name guarantees positional playback.

## 8. Complete message metadata

The underlying g_unitMessages table has 24 records, including unused zero. g_speechTypes starts at the priority field of record zero and advances by 24 bytes; g_speechCategories starts at the sound-key field of record one with the same stride. These offset views share one data table, not separate independent arrays. The following values drive the concrete speech policy; a blank default caption can fall back to sound.TDF choice text.

| Kind | Sound key | Priority | Repeat delay, seconds | Default caption |
| ---: | --- | ---: | ---: | --- |
| 1 | select | 10 | 0 | None |
| 2 | underattack | 9 | 20 | Under Attack |
| 3 | activate | 4 | 2 | None |
| 4 | deactivate | 4 | 2 | None |
| 5 | ok | 5 | 1 | None |
| 6 | arrived | 3 | 4 | Arrived |
| 7 | cant | 8 | 1 | Cannot Comply |
| 8 | unitcomplete | 3 | 3 | Nanolathe Complete |
| 9 | build | 4 | 2 | None |
| 10 | repair | 3 | 1 | None |
| 11 | working | 2 | 1 | None |
| 12 | load | 7 | 1 | None |
| 13 | unload | 7 | 1 | None |
| 14 | cloak | 7 | 1 | Cloaked |
| 15 | uncloak | 7 | 1 | Visible |
| 16 | capture | 4 | 1 | None |
| 17 | count5 | 10 | 0 | five |
| 18 | count4 | 10 | 0 | four |
| 19 | count3 | 10 | 0 | three |
| 20 | count2 | 10 | 0 | two |
| 21 | count1 | 10 | 0 | one |
| 22 | count0 | 10 | 0 | zero |
| 23 | canceldestruct | 10 | 0 | Self destruct terminated |

The mutable next-allowed time is per kind globally. It is not a separate cooldown per unit, player or selected waveform. The literal caption Nanolathe Complete is a speech message, not the source of construction completion itself.

## 9. Admission of unit speech

QueueUnitSpeech (`0x47f780`) requires the unit's playerIndex to match the current player, flag 0x10000000 set and flag 0x4000 clear. With a null explicit text it selects the kind's default caption, passes the resulting text through Translate and enqueues. QueueUnitSpeechIfVisible (`0x47f7e0`) adds an IsUnitVisible acceptance condition; QueueUnitSpeechIfNotVisible (`0x47f850`) adds its inverse. They preserve the ownership/flag checks inside QueueUnitSpeech. They do not themselves select a sound file.

EnqueueSpeech (`0x47fad0`) rejects a kind before its global minFrame, then rejects any kind already queued, regardless of unit. It permits up to eight live entries in a nine-entry backing array. At count eight it processes the lowest-position entry through PlaySpeech with audio disabled and text enabled, frees/removes that entry, then makes room for the new request.

Insertion sorts by descending metadata priority, preserving earlier entries ahead of equal priority. It records kind, current game tick, unit pointer and a copied priority, and allocates/copies optional text. The recorded enqueue frame and constructor field_a5 do not implement an age-expiry test here. Allocation is used without a null check. Kinds, unit pointers and table indices must already be valid; arbitrary inputs are not checked against all 24 records.

Because duplicate suppression is by kind, a second unit's same-kind request can disappear behind the first unit's pending report. Higher-priority insertion can overtake earlier low-priority messages. Neither behavior represents a unit failing to receive its actual game order.

## 10. Servicing and dropping queue entries

PlayNextSpeech (`0x47f680`) processes at most the front entry. If gameTick has reached lastFrame+interval, it calls PlaySpeech with audio/text enabled and updates lastFrame. Otherwise it still calls PlaySpeech with audio disabled and text enabled. In both cases it frees the entry text, shifts the list down and decrements count. Requests are consumed during the inter-voice interval rather than retained until they can all be spoken. Afterward it requests ResumeLoopingWav whenever Windows mode is selected, even for an empty queue.

SpeechQueue::PlayNextSpeechEntry (`0x47fca0`) performs the same front-entry timing/audio/text/removal behavior without the global fallback-resume step. At the ordinary 30 game ticks per second, interval 30 is a nominal second between audio-enabled service attempts. It is tied to supplied gameTick and service calls, not to the waveform's actual ending. A slow/fast simulation or a long spoken file can change perceived cadence.

ClearSpeechList (`0x47f990`) and ClearSpeechEntries (`0x47fa30`) are equivalent: free/remove all entries and reset lastFrame. Unlike ResetSpeech they do not reset every kind's minFrame. RemoveSpeechAt (`0x47ffa0`) frees/removes an indexed entry without a local index-range check. RemoveSpeechOfId (`0x480020`) removes all entries for a matching Unit pointer. The global RemoveSpeechOfUnit (`0x47f8c0`) does the same through g_speechQueue. Their shift loops read one entry beyond the active end, within the spare ninth record under the ordinary eight-entry limit.

The queue borrows unit pointers and owns only copied text. [Unit destruction](../../../src/units/units_485010.cpp) invokes RemoveSpeechOfUnit to clear those pending references. Queue storage is not a generic reference-listener system that can safely retain a deleted unit without that cleanup.

## 11. Selecting audio and text for an entry

PlaySpeech (`0x47fd70`) obtains the unit definition's soundcategory as an unsigned short, selects the category's slot for the entry kind and chooses a random choice index by rand()*count/32768 using a widened intermediate. It does this before independently deciding whether to play audio or show text. Category, unit and slot bounds are not locally validated.

Audio requires priority strictly greater than 10-unitChat, positive choice count, the caller's audio-enable argument and soundFlags bit 0x40. Normally the chosen waveform name comes from that slot's first 64-byte array. With the sing command enabled it instead uses sing except on every eighth game-second bucket, which selects honk. It constructs sounds/name.WAV and chooses Windows disk playback or the gated DirectSound file path at -585.

On entering this audio branch it sets the kind's minFrame to gameTick+delay*30. This occurs even if backend playback fails or the subsequent DirectSound effects gate prevents an audible voice. Conversely, a text-only service does not establish that cooldown. The fixed factor 30 is embedded in this speech policy rather than obtained from the display clock.

Text has its own strict priority threshold, 10-unitChatText, and caller enable argument. It first uses the entry's copied explicit/default text, otherwise the matching choice's second 64-byte array. Null/empty text produces no message. When the unit still has flag 0x10000000 it formats unit definition name, colon and caption into a 100-byte local and calls AddMessage with key one, unit ID and newline terminator. Formatting is unbounded relative to that local, so unusually long names/captions can exceed it.

Text can therefore appear without audio, audio can be attempted without a displayed caption, and a chosen sound/caption pair can vary by unit category and random alternative. The unit's construction type, weapon or hull silhouette alone does not determine the utterance. Message producers in orders, damage and scripts decide which kind to request.

## 12. Adjacent unordered containers and squad constructor

Class_004800c0::EraseSwapBack (`0x4800c0`) overwrites a supplied Unit-vector slot with its last element, erases the last element and returns the original iterator. It preserves no element ordering and assumes a nonempty vector plus a valid iterator. The nearby IntDynArray::EraseByValue (`0x480100`) finds the first integer equal to its argument, returns zero if absent, otherwise overwrites it with the last element, removes the last and returns one. Later equal values remain, and ordering can change.

Squad::Squad (`0x480160`) stores owner/index, clears two fields and constructs an empty integer-vector representation. The local squad module uses its corresponding shared-layout constructor inlined when creating player groups; this out-of-line entry belongs to sound's address interval. It is not another speech-channel allocation. [Squad behavior](units-squads.md) remains documented in its own four-entry module.

## 13. Coverage, persistence and failure interpretation

All 31 owned entries, the complete 23-kind metadata, asset registry/choice dependencies, network receiver and unit-death cleanup were inspected. No active waveform cursor, queued speech list or audio device is serialized by this group. Settings persistence and multiplayer sound requests are separate from saving a playing sound.

The reference distinguishes eligibility, queue admission, audio attempt, displayed text and actual backend success. Static evidence cannot establish a particular ship's asset contents or the sound produced by a historical driver. No builds, installations, runtime tests or source listings were used.
