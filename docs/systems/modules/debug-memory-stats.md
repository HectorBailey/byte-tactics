# Movie playback statistics and summary display

[Systems manual](../index.md) | [Roadmap](../../doc-writeup-roadmap.md)

## 1. Real scope of the provisional module name

The debug/memory_stats address group has two functions between `0x47c4d0` and `0x47c6c0`: ShowStatsSummary in [memory_stats.cpp](../../../src/debug/memory_stats.cpp) and MoviePlayer::WriteSmackStats in [movies.cpp](../../../src/graphics/movies.cpp). Its current name does not mean it implements a general allocator profiler or game's live memory accounting. The concrete outputs are a Smacker playback-statistics text file and a message box that reads that file.

The group interrupts the two address intervals assigned to [movie playback](graphics-movies.md). Counting only physical files would omit the statistics writer from this group or incorrectly count it twice. Both bodies and their declarations were inspected.

## 2. Collecting decoder statistics

WriteSmackStats (`0x47c530`) requires an existing MoviePlayer with a valid decoder pointer. It allocates a local statistics structure and calls imported SmackSummary before opening the output file. The local layout has 21 unsigned integer fields, 0x54 bytes total; six are not printed. The source annotates the imported decoder's field-layout uncertainty, including fields beyond the range reportedly populated by the DLL. The DLL implementation is not present here, so the game-side reads are documented without claiming every returned field has a verified external meaning.

The method opens stats.txt in the process's current directory using write mode. A successful open truncates any previous contents. It prints one tab-separated label/value line for each listed measurement and closes the file. A failed open silently skips output; it does not display an error, choose another directory or return a success indicator. SmackSummary has already been called even on that failure path.

| Printed measurement | Game-side input or calculation |
| --- | --- |
| Frames Per Sec | 1000*frames/totalTime, integer arithmetic |
| Total Playback Time | 1000*totalTime/totalTime, integer arithmetic |
| Time to Open File | openTime |
| Total Frames Played | frames |
| SkippedFrames | framesSkipped |
| TotalBlitTime | timeBlit |
| TotalReadTime | readTime |
| TotalDecompTime | decompTime |
| TotalReadSpeed, bytes/sec label | readSpeed |
| SlowestFrameTime | slowestFrameTime |
| Slowest2FrameTime | slowest2FrameTime |
| AverageFrameSize | averageFrameSize |
| Highest1SecRate | highest1SecRate |
| HighestMemAmount | highestMemAmount |
| TotalExtraMemory | totalExtraMemory |
| HighestExtraUsed | highestExtraUsed |

The playback-time expression uses the same totalTime as numerator and denominator, so for ordinary nonzero values without multiplication wrap it prints 1000 regardless of movie duration. It is not a correctly converted duration. A zero totalTime reaches division by zero when the file opened. The products are not widened to a 64-bit duration, so overflow can also affect the reported arithmetic. Numeric output uses signed decimal formatting even though the structure fields are unsigned, making high-bit values appear as signed results under the existing interface.

No timestamps, movie name, game state, machine configuration or schema header accompany these lines. Writing another movie's statistics overwrites the same file. The writer does not aggregate sessions, lock the file against other writers or persist an in-memory rolling history.

## 3. Reading and showing a summary

ShowStatsSummary (`0x47c4d0`) zeroes a 1024-byte stack buffer, opens stats.txt in read mode and requests one 1024-byte item using fread. When opening succeeds it displays that buffer in a parentless, topmost message box titled Summary. It does not check fread's item count or report partial/error reads. When opening fails it silently returns without a box.

A file shorter than the buffer ordinarily retains zeroed trailing bytes and therefore supplies a terminator after its read content. A file that fills all 1024 bytes can overwrite every zero; no explicit final NUL is written before MessageBoxA consumes the buffer as a string. Longer files are not paginated or dynamically allocated. This is a fixed-size text reader, not a formatted table viewer.

The function does not fclose the successfully opened file. Repeated calls can retain open stream/file resources. It does not validate that the text came from SmackSummary, parse its measurements or sanitize embedded bytes. The displayed contents are whatever the relative filename resolves to at that moment.

## 4. Reachability and diagnostic interpretation

The [frontend movie adapter](../../../src/frontend/frontend.cpp) constructs, plays, closes and deletes MoviePlayer without calling WriteSmackStats. A repository source search finds the method declaration/definition but no active invocation. ShowStatsSummary likewise has an unused declaration in [gui.cpp](../../../src/gui/gui.cpp), without an established active call. These retained functions therefore do not establish that every movie playback writes stats.txt or that a current GUI button opens the summary.

Neither function controls frame pacing, changes DirectDraw surfaces, alters a ship's statistics or records heap allocations. The memory-related labels are fields returned by the movie decoder's summary API. Their presence is not evidence of a game-wide memory profiler.

## 5. Failure, ownership and completion

The decoder remains owned by MoviePlayer. The writer borrows it and owns/closes its output stream. The reader owns a local text buffer but fails to close its input stream. Both filenames are relative to the current directory; neither routine creates directories or retains a path. Neither participates in game saves, multiplayer packets, unit state or construction rendering.

Both address-owned functions are covered, including cross-file ownership, imported-data limits, arithmetic, stream lifetime, truncation and absent demonstrated call sites. This is static source documentation without decoder execution, builds, installations or copied source listings.
