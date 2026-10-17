# Message HUD ring, chat formatting, filtering and unit jumps

Status: finished. `network/net_chat` contains six entries in `0x463ca0..0x464290`: five in [net_chat.cpp](../../../src/network/net_chat.cpp) and [CycleMessageUnits](../../../src/network/net_chat_464000.cpp). This group combines local gameplay notifications with formatted chat display. Network recipient selection and packet parsing are delegated to net_game, while external chat reporting is delegated to the reporting path.

## Record layout and retained indices

The game owns thirty fixed 72-byte records. Each has 64 text bytes, a tick timestamp, a sixteen-bit unit ID, an eight-bit sender-player slot and flags. The local field called unit_46 in DrawMessages is the same byte used as player by AddMessage, not the unit ID. The game also retains read/write indices, display/filter settings, scroll duration and font. [Packet support](network-packets.md) resets indices to zero but does not clear all record bytes.

There is no allocation per message and no save/network serialization of this HUD ring here. Unit IDs are numeric lookups, not reference-counted Unit pointers or generation-tagged handles. Reuse of an ID can consequently change which live object a retained message jump resolves to. Sender slots and player logos use the retained byte directly.

## Appending notifications

AddMessage (`0x463ca0`) ignores empty strings and returns when textLines is zero. It tests whether `(write + 1) modulo textLines` equals read and, if so, advances read with a fixed wrap at thirty. It then writes into the current physical ring record and advances write with that same fixed thirty wrap. The configurable textLines value therefore participates in the collision test while actual storage indices still cycle through thirty; it is not a separate resized ring or a universally consistent capacity bound. Negative settings are not guarded.

Text copy is limited to 64 bytes and explicitly terminates byte 63, so at most 63 display bytes are retained. Time is the current simulation gameTick. The low four flags bits are replaced from the supplied key, while the old high four bits are preserved. Unit ID and player byte are assigned from narrowed parameters. Old jump/highlight flags can therefore survive when a physical record is reused; new insertion does not reset all navigation state.

The MessageArrived sound plays unless the supplied player byte is ten, represented in this declaration by character newline. This comparison is about the numeric sender sentinel, not whether the text contains a newline. Notifications from a concrete player can sound; sentinel/system/local-echo messages can suppress it. If the top main GUI screen is TIMEOUT.GUI, AddMessage marks both context and layer changed so the waiting screen can redraw. Other screens do not receive that explicit redraw marking here.

## Sending a formatted chat line

SendChatMessage (`0x463e50`) formats a 200-byte local string from the sender's full name, optional arrow plus recipient label and the supplied text. The display form is an angle-bracketed sender/optional destination followed by text. It uses unbounded sprintf, so long names, recipient labels or message content can overflow the local buffer before the later HUD truncation applies.

It calls SendChatPacket with the formatted string, optionally ReportGameChat for mission type three, then adds a local HUD copy with the supplied category key, unit ID zero and player sentinel ten. The to parameter changes the label in this function; no destination ID is passed to SendChatPacket by this wrapper. It is therefore insufficient evidence of private routing by itself. [Frontend/game chat routing](../../../src/network/net_game.cpp) interprets current game state and recipient policy separately.

The initial send and external reporting results are ignored through the local void declarations. A visible local echo does not prove either network delivery or external reporting success. The category is narrowed by the actual AddMessage definition even where the caller uses an integer-shaped declaration.

## Expiration and camera navigation

ExpireOldestMessage (`0x463ef0`) checks only the read record when the ring is nonempty. It advances read and returns one when record time plus `(textScroll + 1) * 30` is strictly less than current gameTick. Exact equality keeps it. Otherwise it returns zero. It expires at most one record per call, leaves bytes unchanged and uses simulation time rather than wall-clock time. Negative settings and tick arithmetic follow original integer conversions; no separate wrap-safe deadline policy exists.

ScrollToNextMessageUnit (`0x463f60`) walks from read to write, with physical wrap thirty. It selects the first record with nonzero unit ID and no visited bit `0x10`, looks up that unit in the global array and requires live flag `0x10000000`. It then sets visited/highlight bits `0x30`, centers the camera on the unit's whole X/Z coordinates with argument one and returns one. Invalid/dead candidates are skipped, not removed or marked visited. The lookup has no unit-array bound check, ownership test or LOS test in this method.

CycleMessageUnits (`0x464000`) clears highlight bit `0x20` in all thirty physical records and tries the scan. If none is found it clears visited bit `0x10` in all thirty and scans again, wrapping navigation over available messages. It does not alter read/write indices, category nibble or messages themselves. Preserved high bits on record insertion can postpone a newly overwritten message's first scan until this reset cycle.

## Selecting and filtering visible lines

DrawMessages (`0x464060`) returns when max_lines, the same field named textLines by AddMessage, is zero. Starting at write, it walks backward at most max_lines minus one records or until read. Thus ordinary positive settings select up to one fewer candidate than the configured value. Filtering occurs afterward; hidden candidates are not replaced with additional older visible records.

It selects the Comix font, captures line height and draws forward from the chosen index to write starting y 52. The filter switch is:

| Mode | Visibility test on low category nibble |
| --- | --- |
| 1 | For category other than two, set show zero. For category two, **no assignment to show** occurs, so the subsequent test reads uninitialized/previous stack state. This mode has no dependable complete visibility rule in the source. |
| 2 | Show every category except eight. |
| 3, screenChat zero | Show categories one, four and eight; hide all others. |
| 3, screenChat nonzero | Show every category. |
| Other values | Hide everything. |

A shown line uses GUI palette color ten when highlighted, otherwise fifteen, with key color `0xfe`. Sender byte ten suppresses the logo. Other sender bytes index Player directly; the icon rectangle begins at x 138, is about 0.8 font heights square, and text moves right by roughly 1.5 icon widths. There is no independent sender-index range check or active-player test before logo drawing. DrawTextClipped receives the record address, whose first bytes are the terminated text, at the selected x/y. Y advances only for shown records, by captured font height. The routine leaves shared font/text colors changed and does not draw an enclosing panel or independently clip a maximum screen height.

## Coverage and interaction boundaries

All six bodies, packed record aliases and physical versus configurable ring arithmetic have been traced. Message production categories and recipient logic are defined by their callers; victory, damage, timeout and chat code can all feed this shared HUD without making it a chat-only queue. Actual output depends on settings, fonts and palette assets. No game/network execution, build or dependency installation was performed, and no source listings are copied into this writeup.
