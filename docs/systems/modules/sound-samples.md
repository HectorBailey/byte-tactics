# DirectSound samples, channels, streaming and mixer state

[Systems manual](../index.md) | [Roadmap](../../doc-writeup-roadmap.md)

## 1. Owned entries and shared Sound object

The sound/samples group has 51 entries from `0x4cee50` to before `0x4d0ab0`, all in [samples.cpp](../../../src/sound/samples.cpp). This includes sample/stream/mixer services, five empty/zero stubs and four adjacent compression-initialization helpers. Excluding the latter because they are not audio would leave the inventory incomplete.

[sound.h](../../../src/sound/sound.h) describes the 0x294-byte Sound object shared by the sample and CD-audio modules. It contains DirectSound/primary pointers, 32 active-buffer pointers and associated age/loop flags, eight temporary sample-set pointers, one stream buffer/file, stream half/EOF state, PCM format fields, mixer settings and CD state. g_game's sound pointer and g_cdPlayer refer to this shared audio object in the ordinary consumers. Allocation/deletion and game-level speech/name policy are handled above this module.

Two different ownership tables matter. A sample set is a separately allocated four-pointer array owning a base DirectSound buffer and up to three duplicates. The 32-entry active table borrows buffer pointers from such sets and records playing voices. The eight-set table owns temporary sets made for one-shot memory/file requests. It is not the complete persistent game sound registry.

## 2. Constructor and DirectSound initialization

Sound's constructor (`0x4cee50`) clears open/use3D/noDriver, installs minimum distance 1 and maximum distance 1e20, clears DirectSound/primary and eight temporary sets, captures mixer settings, sets maxBuffers=8, count=0 and sequence=1, clears all 32 active records, clears the stream pointer, reads auxiliary volume into cdVolume and sets streamTimer=-1. It does not zero every CD/file/format field in the whole object. CD initialization establishes its own state.

InitDirectSound (`0x4cef90`) creates DirectSound when absent, requests cooperative level 2 on the supplied window and creates a primary buffer with flag 1. It constructs PCM WAVEFORMATEX from the requested sample rate, bits and channels: block alignment is bits/8*channels and byte rate is rate*alignment. The stored cbSize is 0x12. It asks the primary buffer to accept that format, then records the three format settings and returns one.

If DirectSound already exists, creation is skipped and the existing primary is reformatted. Any failed creation/cooperative/format operation returns zero after ReleaseDirectSound. HRESULT 0x88780078 additionally sets noDriver; other failures do not set that flag. HasNoDriver (`0x4cff20`) returns the stored flag and does not retry device discovery. Higher initialization distinguishes missing-driver disablement from other fatal failures. This routine does not validate arbitrary rates, channel counts or bit depths before constructing the format.

## 3. Creating and releasing sample sets

CreateSampleFromMemory (`0x4cf230`) and CreateSampleFromFile (`0x4cf370`) create a secondary buffer of the requested byte length with flags 0x92 and PCM format derived from their arguments. They lock the buffer, fill the returned contiguous region and unlock it. They then allocate a 16-byte four-pointer set, zero it and install the created buffer at index zero. The set allocation is used without a null check.

The memory form copies the returned lock length from the supplied raw sample pointer, with no independent source-length validation. The file form reads that length from the file's current position and rejects a reported short read after unlocking. Around CreateSoundBuffer only, the file form temporarily changes floating-point control-mask bits 0x18 and restores their previous values. It does not leave floating-point exceptions masked throughout all later audio operations.

Creation, lock or unlock failure releases a created buffer and returns null. These are raw PCM constructors, not full WAV-image decoders: WAV headers are handled by OpenSample first. No secondary wraparound lock region is requested. The file remains the caller's property at this level.

ReleaseSampleSet (`0x4cf4d0`) accepts null as a no-op. For each of four present buffers it stops and releases the object, finds the first matching active-table record, clears that record/flag and decrements count. It then frees the pointer array. It does not retain a reference for any external holder of that array, remove all possible duplicate active records or null the caller's variable. Correct set ownership and active-table consistency remain prerequisites.

## 4. Voice selection, duplication and looping

PlaySampleSet (`0x4cf570`) selects one of the set's four buffers. If the global looping request is active, it first rejects the request when any recorded active buffer has loop flag exactly one. Thus this implementation permits at most one recorded looping voice globally through this path, not one loop per unit or sound set.

Before testing whether the supplied set is null, it evicts while count is at least maxBuffers. It then scans four set entries. A present buffer whose GetStatus returns status zero is reusable immediately. Otherwise it records the buffer with the greatest play-cursor byte position and remembers the last empty nonzero slot. If all existing voices are playing, an empty slot gets a DuplicateSoundBuffer of set[0]; with no such slot, the greatest-position voice is selected and restarted. This is a four-voice overlap limit for one set, separate from the global active-count policy. Greatest cursor is only the implemented heuristic, not a normalized fraction of a sample's duration.

Status-query failure returns zero. The play-cursor query used for choosing a busy voice is not checked. A malformed set with no usable base or a table inconsistent with the active count is not normalized into a safe empty request.

After selection, the routine queries IDirectSound3DBuffer. If available, it disables 3D mode when use3D is false or no position was supplied. Otherwise it sets XYZ from the supplied integer vector converted to floats, sets stored minimum/maximum distances and enables normal 3D mode. It releases that queried interface. Failure to obtain the 3D interface does not itself reject ordinary playback, and individual 3D setter results are ignored.

The buffer is rewound, given the requested DirectSound volume and started with the current loop flag. Failure of any of those three checked operations returns zero. On success it enters the first empty active record, assigns incremented sequence as age, records its loop flag and increments count. If the active table is physically full, the loop simply finds no slot but the function still returns one after starting playback. It does not first remove a previous active record for a reused buffer, so duplicated tracking is possible when callers have not reaped/maintained state as expected.

PlayLooping (`0x4cf540`) sets the global looping flag for the duration of a PlaySampleSet call and then clears it. It returns void and does not preserve an outer nested flag value. The flag is global, without a synchronization guard; this is not a thread-local independent loop request.

## 5. Active-channel eviction, reaping and limits

StopOldestBuffer (`0x4cf180`) chooses an active buffer whose loop flag's low bit is clear, then takes the smallest recorded sequence among eligible buffers. It stops that voice, clears its pointer and decrements count without releasing its owning sample-set object. It does not reset every stale flag/priority field. Despite the name, the policy is oldest recorded nonlooping sequence, not measured sample time or unit importance.

There is no no-candidate guard. With no eligible nonlooping record, its selected index can be 32 and the following dereference is outside the active table. PlaySampleSet's capacity loop can reach that condition when the requested limit is invalid or only protected loops remain.

SetMaxBuffers (`0x4cf210`) stores the supplied value without clamping to 1..32 or immediately reducing live voices. GetMaxBuffers (`0x4cf220`) returns it. A nonpositive value can make the capacity loop demand eviction even with no active voice; a value above 32 can allow playback beyond the record table. The default eight is part of the practical ordinary-call context, not an enforced bound on all direct callers.

ReapFinishedBuffers (`0x4cf0b0`) first checks the base buffer of each temporary set and releases that whole set when status is zero or querying fails. It then clears active-table entries with stopped/failed status and decrements count. Finally, if a stream exists, it services UpdateStream. It does not inspect every duplicate in a temporary set before releasing the set, though the ordinary one-shot path initially plays its base.

StopAllBuffers (`0x4cf150`) stops/clears all 32 recorded active buffers and decrements count for each, without releasing their owners. It does not stop a separate streaming buffer, cancel a delayed stream or free named samples. Releasing ownership is a distinct operation.

## 6. Temporary memory/file playback

PlayMemorySample (`0x4cf800`) and PlayFileSample (`0x4cf8a0`) reap first, find the first empty one of eight temporary-set slots, create a set and play it. Creation failure returns zero. Play failure releases/clears the new set and returns zero; success returns one. If all eight slots remain occupied they return zero without allocation. File playback consumes data from the supplied file cursor and does not close that file itself.

The eight-slot limit controls temporary allocations, while maxBuffers controls the attempted active voice count. Persistent sound-table sets use the same playback method but are not allocated into this temporary-set table. A sound becoming inaudible due to higher game policy is not decided in these raw constructors or channel-management routines.

## 7. Stream initialization and file ownership

StartStream (`0x4cf940`) cancels a previous delayed timer, stops/releases an existing stream and closes its file, then constructs a looping secondary buffer with flags 0x82. Its total byte count is channels*rate*(bits/8)*2, and streamSize is half of that. For valid PCM this is a two-second ring split into one-second halves, not a fixed number of frames or bytes independent of format.

It stores bit depth, starts streamPos at zero, sets EOF marker streamOffset=-1 and adopts the caller's file pointer. It fills the first half, sets volume and starts the hardware buffer looping. Initial fill advances streamPos to the other half; later polling fills that half according to cursor position. The stream is a ring of raw PCM, not a decoder that seeks the file back to the start for endless music.

CreateSoundBuffer failure clears stream and returns before adopting/closing the new file. OpenSample's mode-two caller nevertheless returns immediately, so this failure can leave that file unclosed. Volume/play failure tears down the adopted stream and file. FillStreamHalf can itself tear down on a lock failure, but StartStream proceeds to call stream->SetVolume without checking whether the fill cleared stream, another retained partial-failure weakness. The void interface does not return a reliable creation result.

StopStream (`0x4cfb40`) cancels a present stream timer, stops/releases a present stream and closes its file. IsStreamActive (`0x4cfba0`) is true when either stream exists or streamTimer is not -1. Thus a merely scheduled delayed start counts as active before a hardware stream is created. File closure is conditional on a stream existing; a stray file pointer alone is not swept.

## 8. Half-buffer filling and end of input

FillStreamHalf (`0x4cfca0`) locks streamSize bytes at streamPos. Lock failure cancels the timer, releases the stream and closes the adopted file. A successful lock supplies one region; the routine does not request a second wraparound region because it operates on aligned halves.

While streamOffset is negative, it computes file length minus current file position, reads the smaller of that remainder and streamSize, and pads a partial final half with silence. It does not validate the read call's returned count against the requested length. A short underlying read can therefore leave unexpected bytes despite the intended padding policy. Once fewer than a full half were requested, it records the EOF endpoint as bytesReadRequested+streamPos. Later fills with a nonnegative EOF marker write only silence.

Eight-bit silence is byte 0x80; 16-bit silence is zero. Other bit depths receive no corresponding fill operation. FillSilence (`0x4cfe40`) exposes the same two supported patterns for an arbitrary destination/size and otherwise returns without writing. After a successful fill, Unlock is called with the intended half length, its result is ignored, and streamPos toggles between zero and streamSize.

UpdateStream (`0x4cfbc0`) reads play/write cursors but does not check the HRESULT and uses the play cursor only. When the cursor has entered the half opposite the pending fill offset, it fills that pending half. Once EOF is recorded, its tests stop/release/close on the relevant later half-boundary crossing. It does not stop at the precise partial-byte EOF position, so the padded tail can remain audible as silence until that boundary. Missing enough polls to span ring revolutions is not compensated with a complete catch-up/refill algorithm.

Streaming consumes the remaining file from its current cursor. For a WAV mode-two request, OpenSample finds the data chunk but StartStream is not passed that chunk's byte length. Its later file-length calculation can therefore include trailing chunks beyond the declared data payload. The reader's file cursor and total file length, rather than a separately enforced WAV data bound, govern EOF here.

## 9. Delayed streaming and cooperative timers

StreamSampleDelayed (`0x4d06c0`) copies a filename into one global delayed-name buffer, stores one global volume, installs AddTimer(delay,0,OnStreamTimer), stores its returned handle and returns one. There is no bounded filename copy, AddTimer failure check or cancellation of an earlier delayed timer in this method. Multiple pending requests can overwrite the shared filename/volume while earlier callbacks remain scheduled.

OnStreamTimer (`0x4d0680`) operates through global g_cdPlayer, removes the handle currently stored on that object, sets it to -1 and calls OpenSample in stream mode with the current global name/volume. It does not carry a per-request copied filename in the timer ID. Consequently overlapping requests share both object handle and delayed arguments. [Display timers](graphics-display.md) run callbacks synchronously during polling; they are not background stream threads.

LoadSample (`0x4d0620`), PlaySample (`0x4d0640`) and StreamSample (`0x4d0660`) are thin OpenSample wrappers for modes zero, one and two. Their definitions are void even where higher files declare returned sample pointers/integers. That recovered-declaration mismatch does not create an explicit, type-consistent result contract for direct callers.

## 10. Raw, DIGI and WAV sample dispatch

DetectSampleFormat (`0x4d01b0`) seeks and checks four-byte tags. DIGI at zero plus HSHD at eight plus SDAT at 0x20 selects kind one. RIFF at zero plus WAVE at eight selects kind two. Anything else selects raw kind zero. Reads/seek results and total minimum length are not checked before comparing local tag bytes.

OpenSample (`0x4d02a0`) opens the path through HAPI and returns zero if that fails. It then selects these interpretations:

| Kind | PCM data origin and format |
| --- | --- |
| Unrecognized/raw | Whole file, 11025 Hz, eight-bit mono |
| DIGI/HSHD/SDAT | Data starts at 0x28, rate read at 0x16; 11000 is changed to 11025; eight-bit mono |
| RIFF/WAVE | Locate fmt chunk of at least 16 bytes, read rate/channels/bits, then locate positive-length data chunk |

Mode zero creates/returns a persistent set; mode one plays through a temporary set; both close the file before returning. Mode two passes the file to StartStream and immediately returns one, regardless of whether hardware setup succeeded. An unsupported mode falls through to normal file closure and a zero result. For WAV, the format tag and every consistency relation between sample format, block alignment and data size are not validated; the new DirectSound buffer is still constructed as PCM from the selected fields.

The internal FindChunk helper restarts from RIFF header offsets on every call, reads the container length at four plus eight and walks four-byte tag/length pairs from offset 12. It skips exactly the declared payload length and eight-byte headers, without a separate even-byte pad correction. Truncation, arithmetic overflow, negative/corrupt lengths and nonstandard chunk layout are not handled as a robust external-parser contract.

## 11. Exported chunk helpers

FindChunkSize (`0x4d0720`) uses the same chunk walk and returns the matching chunk's length field, leaving the cursor at its payload, or zero on reaching the container limit. Its provisional name must not be read as a separate offset-table format: the loop advances over payload lengths and subsequent headers.

ReadWaveFormat (`0x4d07f0`) independently searches for fmt, requires length at least 16, reads those bytes and returns sample rate from byte four, channels from byte two and bit depth from byte 14. FindDataChunkSize (`0x4d0910`) independently searches for data and returns its length with the cursor at the payload. Both share the unchecked read/seek and no-explicit-padding limits. They do not load a full file or retain a new handle, and ReadWaveFormat does not use the format tag to reject compressed WAV encodings.

## 12. 3D configuration and mixer volumes

Enable3D (`0x4cfe80`), Disable3D (`0x4cfe90`) and Is3DEnabled (`0x4cfea0`) set/read a stored option. Set3DDistances (`0x4cfeb0`) stores its two floats without ordering/range validation. These changes are applied when later playback configures a buffer, not by walking all currently playing voices. Game-level sound code supplies camera-relative positions and visibility gates.

InitMixerVolumes (`0x4cff30`) records the number of wave-output devices, chooses the first auxiliary device reporting CD-audio technology, and captures the first readable wave volume and selected auxiliary volume. QueryWaveVolume (`0x4cfff0`) and QueryAuxVolume (`0x4d0040`) return only the low 16-bit left-channel value, or -1 when unavailable. They do not preserve separate right-channel values.

SetWaveVolume (`0x4d0070`) clamps an integer to 0..65535, writes it to both channels of every enumerated wave device and returns nonzero if any write failed. It does not overwrite the saved startup waveVolume. SetAuxVolume (`0x4d00d0`) also clamps/duplicates channels but returns nonzero on success, the opposite failure polarity. A nontemporary request while step is nonzero returns one without updating hardware or cdVolume. Otherwise nontemporary requests update remembered cdVolume; temporary fade writes leave that target unchanged.

RestoreMixerVolumes (`0x4d0130`) reapplies a saved nonnegative wave value to all wave devices. It restores saved auxiliary volume only when nonnegative and step is zero, updating cdVolume as well. Original stereo imbalance is lost because each captured left value is written to both channels. These operate on system mixer devices, not merely a private per-unit sound-volume field.

## 13. Teardown and stubs

ReleaseDirectSound (`0x4ceee0`) releases/clears all eight temporary sets, removes a stream timer, stops/releases an existing stream and closes its file, closes CD audio, releases primary/DirectSound and nulls those last two pointers. Persistent named sets are expected to have been freed by higher sound cleanup; this method does not enumerate that registry. It does not independently sweep every remaining borrowed active-table pointer. Partial initialization and repeated cleanup therefore depend on established ownership order.

NullStubRet0 (`0x4cfed0`), NullStubRet0_B (`0x4cfef0`) and NullStubRet0_C (`0x4cff10`) return zero. NopRet_B (`0x4cfee0`) and NopRet_C (`0x4cff00`) return without side effects. They do not implement hidden driver probes or asynchronous work.

## 14. Adjacent compression initialization helpers

InitLzssLock (`0x4d09e0`) zeros a shared ticket and either resets an existing event or creates an unnamed auto-reset, initially nonsignaled event. It does not enter a critical section or free an old event. LzssInitTree (`0x4d0a10`) zeros three trailing tree-state fields, records its index in a short and initializes that indexed node's three short links with 0x1000/zero sentinels. The supplied index and global allocation must already be valid.

LzssAllocWindow (`0x4d0a70`) calloc-allocates 0x1011 zeroed bytes and stores the global pointer. LzssAllocTree (`0x4d0a90`) calloc-allocates 0x1001 six-byte records. Both return zero on success and -1 on failure. Neither frees a previously stored pointer before replacing it. These routines prepare shared compression state; their presence in this address group does not mean PCM playback itself is LZSS-decoded. The compression algorithm starts in the next sqsh module and remains its own documentation responsibility.

## 15. Coverage and limits

All 51 owned entries were read and reconciled against the module's address interval, including the chunk helpers, empty stubs and compression tail. The contracts distinguish borrowed active pointers from owning sets, cooperative polling from worker threads, and a successful request from actual resource creation. Audio driver behavior and audible results were not tested.

This group has no gameplay damage, unit-target rule, match save record or audio-replication packet. Higher sound services make those presentation requests and apply sight/speech policy. Documentation is based on existing source without builds, installations, execution or copied source listings.
