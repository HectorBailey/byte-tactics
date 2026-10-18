# Registry storage, rectangle tests and environment utilities

[Systems manual](../index.md) | [Roadmap](../../doc-writeup-roadmap.md)

The util/registry group contains 16 functions in [registry.cpp](../../../src/util/registry.cpp), from `0x4b6750` to before `0x4b6b80`. The name covers registry wrappers plus adjacent geometry, byte, user-name, text, thread, sleep and display-callback utilities. All are included here rather than treating a registry-only writeup as full module coverage.

## 1. Storage root and common access routine

AccessRegistryValue (`0x4b6880`) builds the path under HKEY_CURRENT_USER, Software, Cavedog Entertainment, then the caller's subkey. It uses RegCreateKeyExA at all three levels for both reading and writing, requesting KEY_READ on reads and KEY_WRITE on writes. A read is therefore implemented through create/open calls rather than a strictly open-existing-only path. A caller subkey such as Total Annihilation or Total Annihilation/Skirmish selects the game's settings namespace; this module does not hardcode every individual setting.

It tracks three handles initialized to null and closes each nonnull handle in reverse order on every exit. Failure to create/open any level returns zero after cleanup. Reads call RegQueryValueExA with the caller's data pointer and in/out byte-count pointer. They do not request/validate the value's registry type. Both ERROR_SUCCESS and ERROR_MORE_DATA are accepted as result one. All other read errors return zero.

Accepting ERROR_MORE_DATA means one is not proof that a complete value fitted in the supplied buffer. The size can describe a required larger allocation while the caller's bytes do not contain a complete result. No automatic resize, retry, string termination or type conversion is provided here. Callers interpreting one as a complete DWORD/string need to account for that historical behavior.

Writes call RegSetValueExA with the supplied type and dereferenced byte count. Success returns one; any write error returns zero. There is no transaction across multiple game options, migration/version field, flush call or per-setting default in this common routine. The current user account owns the namespace, not the machine-wide HKLM path used by the separate [DirectX check](graphics-display.md).

## 2. Read and write adapters

| Entry | Common-access arguments and result |
| --- | --- |
| ReadRegistryValue, `0x4b6860` | Caller subkey/name/buffer/in-out size; read operation |
| ReadRegistryData, `0x4b69b0` | Equivalent read through generic pointer-shaped arguments |
| ReadRegistryDword, `0x4b69d0` | Local size four; read into supplied pointer; returns common result |
| WriteRegistryBinary, `0x4b6a00` | Type 3, caller buffer and fourth argument as byte length; void |
| WriteRegistryString, `0x4b6a20` | Type 1, strlen(value)+1 including NUL; void |
| WriteRegistryDword, `0x4b6a50` | Type 4, four bytes from the supplied integer value; void |

The binary writer's fourth parameter is provisionally named unused but is actually consumed as the write size. The three void writers discard AccessRegistryValue's result, so a higher caller cannot obtain reliable success through their declared interface. ReadRegistryDword does not require REG_DWORD, verify a returned size of four or propagate that local size to its caller. A smaller stored value can leave bytes of the destination unchanged; a larger value can enter the accepted-more-data case.

[settings.cpp](../../../src/game/settings.cpp) supplies concrete option keys, defaults, limits and game-field assignments. [settings_campaign.cpp](../../../src/game/settings_campaign.cpp) uses generic data reads for campaign-related text. This utility layer does not determine whether gamma, audio, nickname or campaign progress is valid game data. It only transfers bytes according to these contracts.

## 3. Inclusive rectangle predicates

RectInsideRect (`0x4b6750`) requires each horizontal endpoint of A to lie between B's left/right and each vertical endpoint between B's top/bottom, inclusive. It returns one only if all eight endpoint inequalities pass. It does not check that A's own left<=right or top<=bottom, so an inverted A can still pass when all its endpoints lie inside a normal B.

RectsOverlap (`0x4b67d0`) rejects A left beyond B right, A right before B left, A top beyond B bottom, then accepts when A bottom is at least B top. Touching edges/corners count as overlap. It assumes the expected rectangle ordering and does not compute or return an intersection rectangle.

[Font containment](graphics-font.md) uses RectInsideRect to choose complete-string acceptance rather than per-glyph clipping. [Surface/drawing services](graphics-surface.md) use their own clipping mechanics for output; calling a predicate does not modify either rectangle or constrain future writes.

## 4. Byte helpers and current user name

CompareByte1Equal (`0x4b6820`) treats its two integer arguments as addresses, reads byte offset one from each and returns one when equal. It does not compare the complete pointed-to records or test null. SetBytes1To3 (`0x4b6840`) writes its three supplied byte values at offsets one, two and three of a buffer, leaving byte zero untouched. It does not infer a color format or allocate the buffer.

GetWindowsUserName (`0x4b6a80`) asks GetUserNameA to fill a 256-byte local. On API success with a nonempty first byte it strcpy-copies that name to the caller's output and returns one. Failure/empty input returns zero without establishing an output default. No destination-capacity argument is provided, so the caller must supply enough storage. It does not read the game's configured nickname, impersonate another user or persist the resulting name.

## 5. Line skipping has no terminal bound

SkipTextLines (`0x4b6af0`) returns the start of the requested zero-based line by incrementing a byte index and counting both newline and NUL bytes as separators. For n=0 it returns the original pointer. It does not modify the source or treat carriage return as a separate separator.

A NUL does not terminate the search: it increments the line count and advances beyond that byte. Asking for a line beyond an ordinary NUL-terminated string can therefore read beyond its intended allocation. A negative requested line can never be reached by a normally incrementing nonnegative line counter without wrap. This is a pointer-walking helper with caller-supplied bounds assumptions, not a safe split-lines parser with an absent-line result.

## 6. Thread creation, sleep and media callback

StartThread (`0x4b6b20`) passes its entry pointer, stack size and parameter to CRT _beginthread. It returns zero only for the unsigned -1 failure result, otherwise one. It discards the returned thread identifier/handle, supplies no join/cancel interface and does not copy the parameter's pointed-to storage. [game_load.cpp](../../../src/game/game_load.cpp) uses it to launch LoadThreadMain and handles a false launch result separately. This routine does not itself synchronize access to game loading state.

SleepMilliseconds (`0x4b6b50`) directly calls Windows Sleep with the supplied unsigned duration. It implements neither game-tick advancement nor a timer callback. CD Player shutdown uses it for a 500-ms wait; actual thread scheduling remains external to this wrapper.

SetMediaNotifyCallback (`0x4b6b60`) stores its integer-shaped callback value at display offset 0x80. It does not invoke the callback, retain separate owner state or verify g_display. [Display WindowProc](graphics-display.md) invokes that pointer for messages 0x219/0x3b9, and [CD audio](sound-cd-audio.md) installs its handler through this helper. A later store replaces the single prior callback; there is no observer list or automatic lifetime detachment.

## 7. Coverage, side effects and persistence boundary

All 16 address-owned entries were inspected and their geometry, settings, thread and CD/display consumers traced. Registry handles are locally owned and closed, while supplied buffers, strings, rectangles, thread parameters and display context remain borrowed. Return one can mean successful transfer or an accepted incomplete registry read; void write adapters do not expose failures.

This group provides settings storage but no match-save format, unit serialization, network packet, construction rule or render frame lifecycle. No registry values were actually read or written by the documentation task. All explanations come from existing source, without builds, installations, application execution or copied source listings.
