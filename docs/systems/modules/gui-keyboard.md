# Keyboard queue, virtual-key translation and input snapshots

## Scope and queue storage

The `gui/keyboard` group owns fourteen entries in `0x4c1a40..0x4c2380`, all in [keyboard.cpp](../../../src/gui/keyboard.cpp). It owns the internal integer key ring, virtual-key mapping, asynchronous held-key queries, initialization of the shared blit event, two Windows cursor-position wrappers and accessors for the retained mouse snapshot. Software cursor drawing and mouse-event queueing belong to the next module.

Queue state is embedded in the display context: capacity at `+0xf2`, thirty integer slots at `+0xf6`, producer head at `+0x16e` and consumer tail at `+0x172`. ClearKeyQueue (`0x4c1a40`) sets both indices zero without clearing slot bytes. InitKeyQueue (`0x4c1a60`) caps a requested capacity above thirty to thirty and resets indices. It does not clamp zero or negative requests to a valid minimum.

PushKeyCode (`0x4c1b20`) computes the next producer slot modulo capacity. If that would equal tail, it drops the new code silently. Otherwise it stores at the old head, advances and wraps at capacity. One slot distinguishes full from empty, so capacity thirty holds at most 29 pending codes. Capacity one can hold none; capacity zero divides by zero; negative capacities violate the indexing assumptions. [Display initialization](graphics-display.md) normally requests thirty.

PopKey (`0x4c1ab0`) returns zero when head equals tail, otherwise returns the oldest code, advances tail and wraps. PeekKey (`0x4c1b00`) returns the oldest code without advancing, or zero if empty. An explicitly queued zero is consequently indistinguishable from the empty sentinel to these consumers. No overflow notification, event count, allocation or queue lock is provided by this module.

DestroyPreCleanup (`0x4c1aa0`) is empty. The fixed embedded queue has no separately allocated buffer for that hook to free.

## Windows-message boundary

The [display window procedure](../../../src/graphics/display.cpp) sends WM_KEYDOWN to HandleVirtualKey with flag zero, WM_CHAR directly to PushKeyCode, and WM_SYSKEYDOWN to HandleVirtualKey with flag one before default processing. Thus ordinary text characters arrive through a separate route from control/function navigation keys. The queue does not tag a code with whether it originated as a text character or a translated virtual key.

HandleVirtualKey (`0x4c1d50`) first samples Control held state. It always handles the following special keys, regardless of its flag or Control, then returns:

| Windows key | Internal code without Control | With Control |
| --- | --- | --- |
| Up, Down, Left, Right | `0xf5`, `0xf7`, `0xf4`, `0xf6` | Same |
| Insert, Delete | `0xee`, `0xef` | Same |
| Home, End | `0xf0`, `0xf1` | Same |
| Page Up, Page Down | `0xf2`, `0xf3` | Same |
| Pause | `0xf8` | Same |
| F1 through F4 | `0xe2..0xe5` | `0xce..0xd1` |
| F5 through F8 | `0xe6..0xe9` | `0xd2..0xd5` |
| F9 through F12 | `0xea..0xed` | `0xd6..0xd9` |

After these cases, flag zero with no Control returns without queueing anything. Otherwise digits map to their character codes without Control or character code plus `0x94` with Control. A-to-Z virtual keys map to lowercase character codes without Control or uppercase code plus `0x69` with Control.

The OEM-key ranges map through fixed punctuation tables: `0xba..0xc0` becomes semicolon, equals, comma, minus, period, slash and backtick; `0xdb..0xde` becomes left bracket, backslash, right bracket and apostrophe. Other virtual keys fall through without output. These tables are not a complete keyboard-layout/Unicode conversion. Ordinary shifted text is handled by WM_CHAR, not by adding a Shift table in this function.

There is no repeat-event suppression or key-up queue in this body. Windows message delivery can produce repeated key-down events, and held modifiers are sampled when translating, not carried as a separate immutable field in every key code. The constant-key cases use an inlined producer with the same ring-full rules as PushKeyCode.

## Held-key queries and practical battle mappings

IsKeyDown (`0x4c1b80`) recognizes only eight internal identifiers:

| Internal ID | Windows asynchronous query |
| --- | --- |
| `0xfb` | Alt |
| `0xf9` | Shift |
| `0xfa` | Control |
| `0xf5`, `0xf7`, `0xf4`, `0xf6` | Up, Down, Left, Right |
| `0x20` | Space |

It masks GetAsyncKeyState with `0xfffe`, excluding the low transition indicator, and reports whether the retained short is nonzero. Other internal IDs return zero rather than being interpreted as arbitrary Windows virtual keys. It does not consume a queued event or latch a modifier for later frames.

Combined with [battle key dispatch](ingame-keys.md), this establishes actual paths such as Control-F5 through Control-F8 saving four camera bookmarks and F5 through F8 restoring them, Control-F9 reaching the global screenshot event, Control-F10 reaching movie capture, and Pause reaching the pause control packet. Additional application/permission gates still apply. Shift is the modifier forwarded to selection operations, while Alt reverses the number-key squad/build-page role according to the configured visual option.

These mappings describe the traced Windows/internal path, not every possible remapping supported by generic GUI quick keys. GUI text entry can consume queued input before battle dispatch, as illustrated by the [application frame](ingame-screenshots.md).

## Blit event initialization

InitBlitLock (`0x4c22a0`) clears the shared held tag. If the global event already exists it resets it and returns. Otherwise it creates an unnamed auto-reset Windows event, initially nonsignaled. Failure of CreateEvent is not checked here. Reinitializing while another participant owns the lock would erase ownership state, so normal environment setup must establish an idle context.

This event supports the [surface/display](graphics-surface.md) and [mouse cursor](../../../src/gui/mouse.cpp) lock protocol using MAIN/MOUS owner tags. InitBlitLock itself does not acquire, release or destroy the event and is unrelated to key-ring synchronization. The lock's tag-based nesting rules are defined by those consumers.

## Cursor wrappers and retained mouse snapshot

SetCursorOverlayEnabled (`0x4c22d0`) directly writes the supplied integer to display `+0x1d2`, the visible/overlay field used by software cursor code. It does not normalize the integer, balance hide/show counts or immediately redraw/restore the cursor. The separate mouse routines decide how that field gates drawing.

SetCursorPosition (`0x4c22f0`) calls Windows SetCursorPos and ignores its result. GetCursorPosition (`0x4c2310`) calls GetCursorPos and writes the resulting screen X/Y through both caller pointers. It does not validate null pointers, convert screen to client/map coordinates or check whether GetCursorPos succeeded before reading its local POINT.

GetCurrentMouseEvent (`0x4c2340`) copies 24 bytes from display `+0x196` into caller storage. SetCurrentMouseEvent (`0x4c2360`) copies 24 bytes back. The [MouseEvent declaration](../../../src/gui/mouse_event.h) identifies X, Y, keyFlags, scaledTick, message and double-click count. These are snapshot copies, not queue pop/push operations, and neither accessor locks around the copy.

The event can be updated by software cursor code and message handling. A copied record does not itself select a unit or imply that its message was removed from the mouse ring. [Frame arbitration](ingame-screenshots.md) deliberately compares snapshots around GUI processing before choosing a game-facing event.

## Completion and ownership limits

All fourteen entries are covered, including cursor/event helpers outside the keyboard queue. No persistent allocation, save schema or network packet is owned here. Key-ring contents and retained mouse records are local input state; later handlers can turn them into orders or control packets under their own gates. No keyboard event, cursor movement or Windows setting was changed while preparing this source-based reference.
