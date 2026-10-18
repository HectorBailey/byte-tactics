# Dialog layers, editing, message layouts and palette tools

Status: finished. This reference accounts for all 49 entries in `gui/dialogs`, `0x4aa8f0..0x4acd00`. The main implementation is [dialogs.cpp](../../../src/gui/dialogs.cpp); [the color-cell outline companion](../../../src/gui/dialogs_4ac970.cpp) owns one additional body. Shared state is described by [Gui](../../../src/gui/gui.h), [Gadget](../../../src/gui/gadget.h) and the [generic GUI runtime reference](gui-gui.md).

## Responsibilities and ownership

This group connects parsed GUI records to the live layer stack, provides small named dialog constructors, edits text-input buffers, wraps and truncates strings, and implements palette-editor primitives. It does not decide multiplayer setup, mission outcomes, save contents or battle orders. Those owners install application callbacks and interpret selected gadgets. The input producers are [keyboard](gui-keyboard.md) and [mouse](gui-mouse.md); layout decoding and common GAF assets belong to [GUI files](gui-gui-file.md).

The locally declared layer contains a stack pointer, gadget-array pointer, click handler, callback data, rendering/input fields, and an embedded array of 200 packed `0x15b`-byte records. Record zero holds screen metadata and the count, so count and physical array capacity must be considered separately. The loader allocates `0x10f57` bytes, exactly a `0x3f`-byte prefix plus 200 records. It does not grow that allocation as records are appended. Gadget indices are used directly throughout this module, usually without checking lookup failure or the current count.

Text pointers, cursor GAF sources and background surface assignments are borrowed here. Allocated layer storage and rendered resources are released by the generic GUI close/render-cleanup paths. WordWrapText returns a separately allocated buffer that the caller must free. These GUI records are presentation state; this module has no battle-save record or network packet format.

## Loading, stacking and appending screens

LoadGuiLayer (`0x4aa8f0`) builds the requested path from the context's GUI path and supplied name, strips the directory from a second name buffer for the screen identity, and changes the resource extension to GUI. It probes length through HAPI, so archive/loose-file precedence follows [HPI](util-hpi.md), rather than a separate dialog search policy. Fixed 256-byte path buffers and concatenation have no complete length validation.

The flags that change construction are:

| Flag | Behavior in this loader |
| --- | --- |
| `0x200` | Append parsed controls to the current layer instead of allocating and pushing another layer. Requires an existing layer. |
| `0x400` | Skip immediate RenderLayer initialization; construction, name assignment and focus setup still occur. |
| `0x800` | Fade the current layer's area, or the default surface if no layer exists, by `-0x18` before loading; retain this bit in layer flags. |
| `0x80` | Retain this layer flag only for a newly pushed layer. |
| Other bits | Forwarded to RenderLayer when immediate rendering occurs; their rendering effects belong to that implementation. |

For a new screen, it allocates and zeroes the entire layer plus record storage, then ReadGuiFile fills the embedded table. For append mode, the temporary screen header is read into the slot after the existing final gadget. If an existing control named exactly PANEL is found, the loader deactivates it, adds flag `0x20` and centers the imported controls within its rectangle. Otherwise it translates imported coordinates by the imported screen's own x/y. It adds the imported count to the existing count and copies the imported controls down over their temporary header. This uses overlapping memcpy, with no capacity or overlap validation.

It clears the periodic callback, background surface and text handler. A new layer links to the old top and becomes the context's top. The resulting top is marked for redraw and its keyboardInput field becomes zero. The screen name is copied into a sixteen-byte field without a forced trailing zero; focus and hot activation are reset. Immediate rendering hides and restores the software cursor around RenderLayer with initialization bit one.

Default accept and cancel names are inferred only when their metadata strings are empty: the first type-one name with a case-insensitive OK or NEXT prefix becomes accept, and a PREV or Cancel prefix becomes cancel. An explicit focusName is searched with a sixteen-character case-sensitive comparison and can leave current at `-1`; otherwise current starts at zero and SelectAdjacentGadget chooses the next candidate. A screen consisting of exactly one type-three control receives special font/color selection, text-edit cursor initialization and a keyboard-queue clear.

Failure handling is unsafe outside the expected valid-asset path. A zero/missing GUI length leaves the local layer pointer uninitialized before later writes. ReadGuiFile failure frees layer storage but execution still uses that pointer; in append mode this can free the already live layer. A RenderLayer result other than one frees layer storage without an explicit stack rollback in this body. Consequently a zero return is not evidence that the old stack is intact. These are observed source paths, not failures exercised in a running game.

## Stack queries, composition and dynamic controls

HasLayer (`0x4ab040`) tests only whether the top pointer exists. IsScreenNamed (`0x4ab060`) compares the first record's actual name field, ignoring case for sixteen characters; there is no additional two-character filename prefix despite an older nearby source comment. IsMessageBoxScreen (`0x4abd20`) instead uses an exact, case-sensitive comparison with MSGBOX.GUI. IsPointInEntryRect (`0x4aafe0`) constructs an inclusive rectangle from top-level x/y/width/height and tests the retained GUI mouse point, returning zero if no layer exists.

BlitLayers (`0x4ab0b0`) recurses through next first, drawing older layers before newer ones. Without a clip rectangle it draws only nodes whose redraw equals one. With a clip rectangle it draws a node when redraw equals one or its inclusive bounds overlap that rectangle. The rectangle is a selection test here, not an additional clipped-blit parameter: the selected layer's surface is drawn at its full x/y through DrawSurface. Each drawn node clears redraw. A null root returns zero; an existing root returns one even if no node needed copying. BlitMenuLayers (`0x4ab170`) forwards the context's top.

ClearSelectedGadget (`0x4ab0a0`) sets hotGadgetIndex to `-1`. This matters to UpdateMenu's automatic close decision after a click callback. SetBackgroundSurface (`0x4ab290`) stores the supplied value in the current layer when present and always returns one, including no-layer calls. SetDescListCleanupFlag (`0x4ab190`) writes context offset `0xcc6`, shared with the field named pathsReady in the common header. Its name does not imply that it frees a description list; generic key/button handling also tests that value.

Dynamic additions have different contracts:

| Entry | Copied or initialized data | Limits and ownership |
| --- | --- | --- |
| AddTextGadget, `0x4ab1b0` | Appends type five; initializes position, height 15, color 15, attributes and active state; width `-1` means screen width minus x minus five. Copies up to 127 text bytes and terminates byte 127. | No count/capacity check. Name uses unbounded strcpy into a sixteen-byte field. Other untouched fields retain their storage contents. |
| AddButtonGadget, `0x4ab2b0` | Copies `0x13e` bytes and forces type one. | Rejects count exactly 200, not greater counts. Does not initialize the rest of the record. |
| AddHotspotGadget, `0x4ab310` | Copies `0xcc` bytes and forces type six, then clears callback/image/frame/index fields. | Same equality-only limit; supplied image/callback state is deliberately discarded. |
| AddBarGadget, `0x4ab3a0` | Copies `0xd6` bytes and forces type thirteen. | Same equality-only limit and partial-record copy. |

All additions increment the signed count before addressing the destination. The equality check does not make 200 usable controls safe in the loader's 200-record allocation, which also contains record zero. No helper reallocates storage or owns external pointers in copied records. A known long name such as Player%dController exceeds the sixteen-byte name field; field order determines what that write overwrites.

## Cursor choice and mouse-event consumption

InitCursorFrames (`0x4ab4e0`) assigns one static frame to shared/default/hover slots, displays it, clears animation bit zero and mouseKeyFlags, and clears both alternate GAF-source pointers. SetCursorAnimation (`0x4ab400`) installs the default source, initializes its sequence at index zero, enables animation and displays the first frame. SetCursorHover (`0x4ab440`) chooses hover or default source/frame. A source already equal to the current source is retained without reinitialization; a static frame already equal to the displayed sprite also returns early. Thus that early static-frame return does not independently normalize cursorSrc or animation state. ApplySharedCursorFrame (`0x4ab4c0`) displays the shared frame and clears animation bit zero; despite a nearby historical comment, its body does not wait on a handle.

UpdateCursorAndMouse (`0x4ab5d0`) advances an active GAF reference by animTimer and updates the sprite only when the sequence index changes. It peeks one queued mouse event. With a layer, an event is consumed if its point lies inside the layer's screen rectangle or its keyFlags are zero. Consumption copies the whole 24-byte event to retained GUI point/message fields and separately sets mouseKeyFlags. A queued event outside the rectangle with nonzero keyFlags stays queued, and no-layer calls do not consume queued events. Only the empty-queue path refreshes the retained point snapshot through GetCurrentMouseEvent; that copy does not separately rewrite mouseKeyFlags. This is a queue admission rule, not global clipping of the operating-system pointer.

IsMouseButtonMessage (`0x4ab510`) recognizes down or double-click messages; IsDoubleClickMessage (`0x4ab570`) recognizes double-click only. Both choose the left branch if mask bit one is set, otherwise the right branch if bit two is set. Passing both bits therefore tests left only. HasMouseKeyFlags (`0x4ab5b0`) is an any-bit mask test. SetClickMode (`0x4ab690`) stores both the context mode and the current layer's unaligned mode field; it assumes a layer exists. GetClickMode (`0x4ab6b0`) returns the context copy.

## Text-edit buffers and key handling

CommitTextEdit (`0x4ab6c0`) sets the cursor to strlen when clear is zero and the existing string length is at most maxLength. Otherwise it empties the buffer and sets cursor zero. It immediately draws the input. It does not truncate an overlong value, allocate a buffer or validate the supplied index.

HandleTextEditKey (`0x4ab720`) edits the selected record's inline text, with a cursor stored in the shared Gui rather than separately per control. When layer keyboardInput is zero, it ignores the incoming key and pulls from PopKey. Once processing starts, it drains subsequent queued keys regardless of that initial mode. It returns the last processed code; Escape exits the loop early. Any nonzero initial code marks the control changed and triggers redraw, even if the code is unsupported or cannot alter text.

| Code | Edit operation |
| --- | --- |
| `0xf0`, `0xf1` | Home and End. |
| `0xf4`, `0xf6` | Move left/right within the current string. |
| `8` | Backspace before the cursor, moving later bytes left. |
| `0xef` | Delete at the cursor when it precedes the terminator. |
| `0xbf`, `0xee` | Clipboard replacement using Windows clipboard format one, CF_TEXT. |
| `0x1b` | Stop processing and return Escape without restoring old text. |
| Printable codes `0x20..0x7f` | Insert if character, length and pixel-width tests pass. |

Normal insertion rejects only length equal to status, rather than length greater than or equal. Attribute bit two restricts characters to alphanumeric, underscore, space and apostrophe. Existing text width plus the proposed one-character width must fit width minus four. Insertion shifts bytes starting at status minus one down toward the cursor and stores the new byte. This relies on valid status, cursor and existing termination; it is not an independently bounded string implementation.

Clipboard handling opens the clipboard with the display window, obtains CF_TEXT global memory, clears 128 inline text bytes, and copies the smaller of GlobalSize and status minus one. It removes trailing characters until the measured width fits the full gadget width, then unlocks and closes the clipboard. Paste replaces the entire input rather than inserting at the cursor, does not apply the alphanumeric attribute filter, and does not update cursor afterward. GlobalLock failure, negative/small status, oversize status and inconsistent termination are not fully checked. In particular, status minus one can become a negative copy length. The cursor may remain beyond the newly pasted string until another operation changes it. These limitations explain why callers and GUI data must provide valid short input fields.

## Named dialog constructors and callbacks

Each constructor returns one when LoadGuiLayer succeeds and zero otherwise. These are asynchronous layer constructors, not blocking calls returning the user's answer. They copy caller strings directly into inline fields with no length checks, usually trust that every required named gadget exists, and leave application interpretation to the layer/update owner.

| Constructor | Layout and setup |
| --- | --- |
| OpenConfirmDialog, `0x4abb20` | CONFIRM.GUI, flags zero; fills type-five TITL and sets its x to `-1`. Does not install a new handler here. |
| OpenYesNoDialog, `0x4abbd0` | YESORNO.GUI, fade flag `0x800`; fills type-one CHC1/CHC2 and type-five TITL. Parameter order supplies chc2 before chc1. Installs YesNoDialogHandler. |
| OpenNotExistDialog, `0x4ac0a0` | NOTEXIST.GUI, flags zero; fills type-five NAME, x `-1`, installs NotExistDialogHandler. |
| OpenChoice3Dialog, `0x4ac170` | CHOICE3.GUI, flags zero; invokes RenderLayer initialization again, then fills three type-one choices and type-five title, title x `-1`. Installs Choice3DialogHandler. |
| OpenInputDialog, `0x4ac340` | INPUT.GUI, flags zero; fills type-three INPT, title and two type-one choices; title x `-1`. Its final parameter is unused. Installing text does not itself call CommitTextEdit afterward. |

YesNoDialogHandler (`0x4abba0`), MessageBoxHandler (`0x4abd00`), NotExistDialogHandler (`0x4ac080`) and InputDialogHandler (`0x4ac300`) only call name-test helpers and discard their results. Choice3DialogHandler (`0x4ac130`) only looks up CHC1/CHC2/CHC3 and discards the indices. They do not assign a returned answer, send a packet or clear hotGadgetIndex. Generic UpdateMenu consequently applies its usual automatic closing after activation unless another owner changes that state. Consumers needing a meaningful choice install their own callbacks or inspect the shared selection at the appropriate point.

## Message-box layout and immediate update

OpenMessageBox (`0x4abd90`) loads MSGBOX.GUI with fade, translates text into a 256-byte local buffer using unbounded strcpy, and allocates a wrapped copy. It invokes RenderLayer cleanup with flag two before adding new text controls. It copies at most 254 bytes of wrapped text into another 256-byte local buffer without explicitly terminating that copy, then uses strtok with newline separators. Empty lines are skipped by strtok; carriage returns from CRLF wrapping remain in each token.

For each token it appends a TEXT label, starting y at 20 and advancing by current font height plus five. The first new index is captured before additions. The later width loop measures from that captured index, so the actual body measures the appended controls, despite a nearby source comment claiming it starts beyond them. The parameter named autoHeight actually chooses width calculation: nonzero takes maximum line pixel width plus twenty; zero uses wrapWidth. Height is always line count times 25 plus entries[1].height plus forty. It centers the overall rectangle on the current screen, gives all type-five entries the screen width and attribute two, and optionally repositions the OK control near the lower right.

The parameter named centre controls OK availability: nonzero finds OK, places it and assigns both accept/cancel metadata names to OK; zero deactivates OK. It then enables keyboard input, initializes rendering, installs MessageBoxHandler, marks changed and immediately calls UpdateMenu once before freeing the wrapped allocation. This immediate pass can process input/callback closing before OpenMessageBox returns. Translation expansion, token-copy termination, record capacity, fixed line-height versus font-height mismatch and negative/offscreen geometry are not validated here.

## Wrapping and truncation algorithms

WordWrapText (`0x4ac4c0`) optionally selects a gadget font and measures the width of the letter d. It estimates allocation as source length plus three times the estimated number of lines plus two, where estimated characters per line are width divided by that glyph width. Zero glyph width or width smaller than one glyph can therefore cause division by zero. The estimate is not a checked maximum for all proportional-font inputs.

It copies one character at a time until NUL or `0xff`. When the next source character is space, newline or hyphen, it measures the accumulated line. At or above the requested width it backs up to a previous space or hyphen, removes that break character, inserts CRLF and resumes after it. Existing newline advances the measurement start. This is delimiter-triggered wrapping: it does not split every overlong word at an arbitrary fitting character. Backtracking has no source-start guard, so a too-wide first word reaching a delimiter can read backward outside the source. The caller owns the returned pool allocation; selecting an alternate font changes shared font state without restoration in this helper.

TruncateTextWithEllipsis (`0x4ac610`) measures using default or selected font. Width strictly below limit returns unchanged. Otherwise it reserves the measured width of three dots when requested, removes bytes from the end until the remaining text plus marker fits or becomes empty, and optionally overwrites the end with three dots. A marker wider than the limit is still appended after the empty-string stop. Exact-width text enters this path and may gain dots. The function needs writable spare capacity for dots and NUL, does not allocate, and truncates bytes rather than Unicode characters.

## Palette remapping and editor primitives

BuildColorRemapTable (`0x4ac710`) maps all 256 source colors to the closest of 256 destination colors. Distance is the sum of absolute differences of red, green and blue, not Euclidean or perceptually weighted distance. Strictly smaller distance replaces the winner, so ties retain the first destination index. Flags/alpha are ignored. RemapPaletteToClosestIndices (`0x4ac7d0`) first copies the supplied 1024-byte palette into Gui's palette area and maps each copied color back to the closest supplied source color, storing 256 indices in the context's remap table. CopyPaletteEntries (`0x4ac8a0`) copies that retained 1024-byte area to the caller.

DrawColorGrid (`0x4ac8c0`) finds the type-six COLS control and fills a 16 by 16 array of eight-pixel squares with indices row times sixteen plus column. Coordinates add both screen and gadget offsets and draw into the screen surface. DrawColorCellOutline (`0x4ac970`, companion) outlines one such eight-pixel square with a caller color; it does not validate the requested cell. GetColorCellAt (`0x4acbe0`) performs the inverse coordinate calculation, clamps row and column independently to `0..15`, and returns row times sixteen plus column. It returns a nearest edge cell for outside points rather than an invalid index.

ShowPaletteEntryRgb (`0x4aca20`) writes an indexed four-byte palette entry's RGB bytes into the knobPos fields of RED, GREN and BLUE type-four controls, then calls RenderLayer with flag four. ApplySlidersToPaletteEntry (`0x4acc70`) uses the low value byte at offset `0x140` from three stored slider pointers, writes the chosen four-byte palette entry and calls SetPaletteColors for that one index. Its local fourth flags byte is uninitialized before the four-byte assignment; indices and pointers are unchecked.

LerpPaletteRange (`0x4acae0`) orders its two endpoints, reads their packed three-byte RGB values and linearly interpolates entries from low inclusive to high exclusive using integer division. Equal endpoints execute no interpolation iterations, so they do not divide by zero in the loop. The high endpoint remains as copied. This helper treats input as packed RGB triples, unlike the four-byte palette-entry routines above. More seriously, its local buffer is 768 bytes but both inbound and outbound memcpy lengths are 1024 bytes. Both copies exceed the local array regardless of endpoint choice. No bounds, palette upload or remap rebuild follows automatically.

WriteTabs (`0x4accd0`) writes depth tab bytes one at a time through HAPI_WriteFile, ignores individual write results, and does nothing for zero or negative depth. It is shared export support adjacent to the GUI-record writer, not a palette persistence format.

## Coverage boundaries

All 48 main-file entries and the outline companion have been read, including failure branches and state setters. Actual GUI dimensions, fonts, GAF sequences, names, translations and application callbacks determine presentation. Source tracing establishes these contracts and unchecked paths without claiming an executed UI result. No build, dependency installation, game execution or source listing was included in this documentation work.
