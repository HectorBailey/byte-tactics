# Windows waveform playback fallback

[Systems manual](../index.md) | [Roadmap](../../doc-writeup-roadmap.md)

The sound/play_sound group consists of six functions in [play_sound.cpp](../../../src/sound/play_sound.cpp), between `0x49f610` and `0x49f8c0`. It is the PlaySoundA-based alternative selected by the higher sound system's UseWindowsSound preference. It does not allocate DirectSound channels, mix concurrent game effects, compute positional attenuation or decode game-specific sample containers.

## 1. Global state and availability

Two globals carry its local lifetime: g_loopingWav is a borrowed pointer to a remembered in-memory waveform, and g_diskWav owns the most recently loaded disk waveform. Neither is a queue or an array of channels. Their relationship is independent: loading a disk sound does not clear the remembered loop, and remembering a loop does not free the disk buffer.

IsWindowsSoundAvailable (`0x49f610`) always returns one. It does not enumerate devices, open a waveform driver or try a sound. Consequently successful initialization through that test is not evidence of an actual playback device. The higher InitSound routine in [sound.cpp](../../../src/sound/sound.cpp) selects this fallback and disables its DirectSound path; individual PlaySoundA calls can still fail.

## 2. Borrowed memory playback

PlayWavMemory (`0x49f680`) calls PlaySoundA with a supplied waveform-image pointer, null module handle and SND_ASYNC|SND_MEMORY. The call starts asynchronously and returns without retaining a separate owned allocation. The caller must keep the waveform bytes valid while Windows is using them. There is no local validation of a RIFF header, buffer length or pointer, no volume argument and no position argument.

The function is defined void. Higher sound code has an integer-returning declaration of this entry and can return its call result. That declaration mismatch does not establish a reliable success value at the wrapper interface, even though PlaySoundA itself has a result. Documentation must distinguish the explicit BOOL return of PlayWavFromDisk from this void wrapper.

PlayLoopingWavMemory (`0x49f6a0`) first stores the borrowed pointer in g_loopingWav, then calls PlaySoundA with flags 0x0d: asynchronous, memory image and loop. It does not copy the image or remember a separate playback handle. The remembered pointer is installed even if the underlying API call fails. Its own return type is also void despite integer-shaped declarations in higher sound code.

These operations use the waveform API's playback mechanism rather than the DirectSound backend's 32-entry active table or four-voice sample sets. Calling the fallback does not grant the caller an independently managed voice. The higher backend selection therefore changes concurrency and spatial behavior as well as which API receives the bytes.

## 3. Resuming the remembered waveform

ResumeLoopingWav (`0x49f620`) does nothing for a null remembered pointer. Otherwise it calls PlaySoundA with flags 0x15: asynchronous, memory image and no-stop. This resume call does not include the loop bit. Its name alone is not evidence of restarting an indefinitely looping voice through the same flags as PlayLoopingWavMemory.

The higher PlayNextSpeech service calls this function while the Windows fallback is selected. The no-stop flag lets the API refuse to interrupt an already playing sound. No return is checked, no duration is measured and no cursor is saved; the operation is a repeated playback request for the remembered image, not resumption at a previous sample offset. Short effects and resumed background imagery therefore depend on the external API's current playback state.

## 4. Loading a waveform by path

PlayWavFromDisk (`0x49f6c0`) loads the entire requested file through HAPI_LoadFile. Load failure returns zero and preserves both the previous owned disk pointer and the remembered loop. A successful load calls PlaySoundA asynchronously from the newly loaded image, frees any prior g_diskWav allocation, assigns the new allocation to g_diskWav and returns the API's BOOL result.

The replacement allocation is retained even if playback fails. There is no explicit stop before freeing the previous allocation; the routine relies on the playback API's replacement behavior rather than tracking completion of the old image. It has no load-size cap or internal WAV parsing. The HAPI layer determines path/archive access, and the waveform API determines whether the returned bytes are playable.

Only one disk allocation is retained by this group. Previously loaded generic sound-table images have a separate owner and are not freed by replacing g_diskWav. Conversely, this function does not register its loaded image in the game's named-sound table.

## 5. Stopping and freeing

StopWindowsSound (`0x49f640`) first issues PlaySoundA with null sound/module and flag 0x2000. It ignores the result, then frees a nonnull g_diskWav and clears that pointer. It always clears g_loopingWav but does not free that borrowed image. Clearing the loop prevents later ResumeLoopingWav calls from requesting it again.

Higher StopAllSounds invokes this path when UseWindowsSound is set. StopWindowsSound is locally repeatable with already-null globals, but it does not reset the higher named-sound registry or speech queue. Freeing a named sound still belongs to that registry's backend-aware cleanup. Callers must avoid freeing borrowed waveform data while asynchronous playback can still reference it.

## 6. Interactions and completion

The six bodies and all higher sound-module call sites were traced. They contain no per-unit ownership, fog test, replication packet, save format, construction percentage or weapon-target decision. Higher PlaySoundAt falls back to indexed nonpositional playback when this backend is selected, so DirectSound visibility/attenuation decisions cannot be inferred to occur inside these wrappers.

Pointer lifetime, ignored API failures, mismatched return declarations and differing loop/resume flags are part of the retained source contract. Actual Windows playback behavior was not exercised. This reference was written from existing source without builds, installations or copied source listings.
