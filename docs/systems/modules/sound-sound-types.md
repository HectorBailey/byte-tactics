# Sound definitions, speech choices and category ownership

[Systems manual](../index.md) | [Roadmap](../../doc-writeup-roadmap.md)

The sound/sound_types group contains six functions in [sound_types.cpp](../../../src/sound/sound_types.cpp), `0x42f450` to before `0x42f910`. It loads the named-effect catalog and unit speech-choice definitions, manages their allocations and frees them. It does not implement playback, select a weapon target or decide whether a construction wireframe is visible.

## 1. Two separate definition files

LoadAllSound (`0x42f7e0`) resets the game's named sound count, builds gamedata/allsound.TDF and iterates its top-level records when loading succeeds. Each record's name, copied into a 32-byte local, is the logical effect name. A present sound field, read into a 256-byte buffer, supplies the file/sample name passed with that logical name to LoadSoundByName. Missing sound fields are skipped. The TDF file is unloaded after enumeration.

It then calls LoadSoundCategories regardless of whether allsound.TDF loaded. Thus effect-file failure does not skip the separate category-file attempt. This function does not first release an existing effect catalog before resetting its count; it expects an appropriate initialization/teardown order rather than repeated reload onto live allocations.

LoadSoundCategories (`0x42f580`) resets category pointer/count and builds gamedata/sound.TDF. Each top-level record becomes a zero-initialized 0x160-byte category: a name in its initial 0x40-byte region followed by 24 three-word choice slots. It gets the category count directly from the TDF record count, allocates the combined array, zeroes it and enumerates records. Allocation is not null-checked before memset. A missing/unloadable file leaves an empty category table; a failed record selection leaves its allocated category record zeroed.

The loader fills slots one through 23 beginning at byte 0x4c. Slot zero, beginning at 0x40, remains unused/zero in the normal load. A slot consists of count plus two pointers: one array of waveform names and a parallel array of caption text. The arrays store 64-byte fixed records, not pointers to separately allocated individual strings. The category record name is copied with a 0x3f-byte limit into its prezeroed name area.

## 2. Message kinds and data-table aliases

The 23 loaded keys come from the existing [unit message table](../../../src/units/unit_messages.cpp): select, underattack, activate, deactivate, ok, arrived, cant, unitcomplete, build, repair, working, load, unload, cloak, uncloak, capture, count5, count4, count3, count2, count1, count0 and canceldestruct. Each has the same numeric kind used by speech playback. There is no separate ship-only category format; a unit definition selects a category, whose records can provide hull-specific speech/audio assets.

The symbol g_speechCategories at 0x5086fc is a view beginning at entry one's sound-key pointer inside g_unitMessages. Advancing that view by its 24-byte stride reaches later message keys. The g_speechTypes view used by playback begins at another offset in the same underlying message data. These are not independent tables that can be freely reordered. The message table supplies priorities/delays/default texts; sound.TDF supplies per-category waveform/caption alternatives.

## 3. Reading one choice and its caption

ReadSoundEntry (`0x42f450`) receives the selected TDF record, a key and a slot represented by three integers. It reads the key's waveform value into 64 bytes. A missing value returns zero without adding an entry. A present value constructs key+text, reads its optional caption into another 64-byte local and uses an empty string when that caption is missing.

It reallocates both parallel arrays to (oldCount+1)*64 bytes, stores the waveform and caption at their matching oldCount offsets, increments count and returns one. A found-but-empty waveform field is governed by the TDF getter's result; this routine does not separately reject the empty string. Missing caption does not discard the audio choice. Reallocation results are immediately assigned/used without allocation-failure recovery, and the three-word slot stores pointers through integer-shaped fields.

For each message key, LoadSoundCategories tries the unsuffixed name first, then key1, key2 and increasing suffixes until the first missing numbered key. A missing unsuffixed key does not prevent key1 from loading. A gap at key2 prevents discovering key3 and later choices. Order is the optional base choice followed by consecutive numbered choices. Every caption key is formed from the exact choice key, for example selecttext, select1text and select2text.

The numbered-key local is 32 bytes and generated with sprintf; there is no general unlimited key-length or suffix-count guard. The current built-in key names are the ordinary bounded context. The routine does not prefetch every speech waveform into DirectSound while loading these strings; later speech selection opens the selected sound by name.

## 4. Named effects and registry dependency

LoadSoundByName (`0x429470`) belongs to game/data_files in [data_files.cpp](../../../src/game/data_files.cpp), but is the effect-loader dependency. With a logical name it searches the existing name table case-insensitively over 32 bytes; with a null logical name it searches the filename table instead. An existing match returns its index without reloading.

A new effect is rejected when soundCount+1 is at least 256, returning zero. This permits at most 255 inserted entries through this test, and zero is also a valid first index, so the full-table result is not an unambiguous absent-sound sentinel. For an accepted entry it loads through LoadSoundFile, copies filename/logical name into 32-byte records and increments the count even when the loaded handle is null. Full-length strncpy input need not be NUL-terminated. Backend selection determines whether the returned handle is a raw WAV allocation or a DirectSound sample set.

Freeing must use the same backend interpretation as loading. These dependencies are documented to establish this module's ownership contract, without promoting the broader data_files module from its partial status.

## 5. Binding a unit category and consuming choices

[Unit-type loading](units-unit-types.md) is separate from this definition loader. The concrete binding in [unit_types.cpp](../../../src/units/unit_types.cpp) reads soundcategory, searches category names for a symbolic value or uses its numeric conversion path, and stores a short index on UnitDef. Speech later reads that index and message kind to select a slot. This module does not clamp every later UnitDef/category index or decide audibility.

The sound/sound module uses slot count to choose one random alternative, uses the first string array for its waveform name and the second for fallback text, and applies global message priorities/cooldowns and unit-chat settings. Merely having a loaded sound category does not make every order play a voice; explicit speech requests and policy checks are required. [Unit definitions](units-unit-types.md) and [unit script/order references](units-unit-script-calls.md) explain producers of game actions.

## 6. Cleanup and lifecycle

FreeSoundCategories (`0x42f740`) visits each allocated category and all 24 slots. A positive slot count causes both arrays to be freed. It then frees the category array when categoryCount was positive and clears count/pointer unconditionally. A malformed slot with a nonnull pointer but zero count is not independently freed. A nonnull zero-category allocation likewise does not take the positive-count free branch. Normal initialization supplies consistent counts and pointers.

FreeSounds (`0x42f8c0`) frees categories first, then calls the backend-aware FreeSoundSet for every named effect up to soundCount and resets that count. It does not erase every stale name/file/pointer array cell or recreate the speech queue. [game_state.cpp](../../../src/game/game_state.cpp) calls LoadAllSound during game setup and FreeSounds during teardown; the Sound object and active requests must be kept consistent with that order.

FUN_0042f900 (`0x42f900`) is an empty one-argument function. Its presence does not add a second resource manager or hidden category update.

## 7. Coverage and limits

All six owned functions were inspected, including complete numbered-choice parsing, shared-table aliasing, allocation/free conditions and effect-registry/unit-definition consumers. TDF syntax and archive resolution belong to their own modules. This group has no sound-category save record, per-unit playback cursor or network replication packet. Definitions are reconstructed from assets rather than serialized here.

Documentation uses existing source only. No build, dependency installation, game execution or source listings were used, and no missing asset values are invented as universal unit behavior.
