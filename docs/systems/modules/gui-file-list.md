# Directory lists, file requester and blinking text slots

## Scope and representations

The `gui/file_list` group owns fourteen entries in `0x4aefa0..0x4b0090`, all in [file_list.cpp](../../../src/gui/file_list.cpp). Five support list sorting, directory scanning and FILEREQ.GUI; nine manage timed blinking words. The shared address group does not make blinking text part of filesystem enumeration.

File lists are consecutive NUL-terminated strings, optionally paired with another such list and an integer sort-key array. [SkipTextLines](util-registry.md) advances across NUL or newline separators by count. It has no overall buffer-length bound, so callers must supply valid counts and terminated storage. HPI supplies the combined loose/archive discovery view described in [file services](util-hpi.md).

## Sorting names, parallel strings and numeric keys

SortFileList (`0x4aefa0`) allocates two 96,000-byte output buffers and two 12,000-byte pointer arrays, even if the optional second list is null. It builds pointers for count entries and remembers the last first-list item beginning with a backslash. That index divides the two sorting regions. It does not first partition arbitrary mixed entries into directories/files; it assumes the source order already has the intended grouping.

The first region through that last backslash item is bubble-sorted by ascending integer key when keys exist, otherwise case-insensitive ascending text. The second region is sorted by descending integer key when keys exist, otherwise case-insensitive ascending text. With no backslash item, the first region is absent and the second covers the entire list. Equal comparisons retain their prior order.

Every swap also swaps parallel second-list pointers and keys when provided. It copies the reordered strings into temporary buffers, copies those bytes back to the caller lists and frees all temporary allocations. Keys remain reordered in place. Integer subtraction comparisons can overflow; there are no caller-capacity checks before filling the fixed temporary buffers or indexing beyond 3000 pointer slots.

The sort writes bytes for the supplied count of strings, not a separately allocated persistent list object. Ownership of caller storage does not transfer. File times are only one possible numeric key source; the routine itself does not inspect dates or directories beyond the leading-backslash split marker.

## Scanning a directory view

ScanDirectory (`0x4af320`) allocates 12,000 bytes tagged FILETIMES, then optionally enumerates directory names when mode equals one. That first search uses `*.` and accepts names not beginning with a dot only when the entire attribute value equals `0x10`. Accepted directory strings receive a leading backslash, and the optional size list receives `<DIR>` plus NUL.

It always performs a second search with the supplied path/mask. This accepts non-dot names whose entire attribute value is not `0x10`, copies their names, optionally strips extensions when flag equals one, records modification times and optionally records a decimal HAPI_FileLengthByName result. The length query uses the leaf name, relying on the process directory rather than prepending the search mask's directory.

The exact-attribute test is significant: archive directories are returned by HPI with `0x11`, so this routine does not classify them as ordinary `0x10` directories. They can enter the second pass's file branch. A physical directory with additional attribute bits also need not satisfy the first test. This is not a bitwise directory predicate.

When what is zero, no sorting is done. Value two sorts names alphabetically. Other nonzero values sort by the time array. Those calls pass no parallel size list, so optional size strings are not reordered along with names by this internal sort. Directory items also increase the combined count without filling time entries, while file times begin at the start of the time allocation. Numeric sorting of mixed directory/file output can therefore use uninitialized/misaligned keys.

After sorting if requested, it frees times, writes an extra NUL at the end of the populated name region and returns the combined item count. It has no output-capacity arguments, file-count limit, callback cancellation or robust error object. HPI enumeration failure is simply an absent pass. Dot-prefixed names are filtered regardless of their filesystem hidden attribute.

## File requester creation and refresh

OpenFileRequester (`0x4afa30`) loads FILEREQ.GUI and returns null if that fails. Otherwise it allocates `0x24c` requester state, stores the GUI pointer, copies the supplied selection/path into its cwd field and strips the filename there. An empty result becomes NO PATH. It installs FileRequesterHandler on the newly loaded layer, not on the original GUI object, and initializes the requester callback null.

It resolves NAME, MASK, PATH, TITL and SLID gadgets, resets selected entry coordinates/title placement fields, copies the title and allocates FILE NAMES storage of 96,000 bytes plus FILE SIZES storage of 60,000 bytes, initially filled with `0xff`. It retains the mask pointer and the caller's mutable selection buffer. It strips the path from that caller buffer and copies its remaining filename into NAME, so merely opening the requester already mutates the caller's original path string.

It saves the current drive/directory, then RefreshFileList (`0x4af5b0`) reads the actual current drive/directory, scans the MASK gadget text with directories included and no internal sort, sorts names and sizes together alphabetically, binds SWIN and SIZE listboxes, resets the slider knob and sets PATH text to the current directory.

The copied input directory is not first installed through ChangeDirectory before this refresh; the actual process directory overwrites cwd. Thus an initial full path is not sufficient evidence that the dialog begins in that path's directory. Drive/directory wrappers' discarded errors and string constraints are described in [HPI](util-hpi.md).

The list buffers are retained in requester state. The teardown callback below frees only that state explicitly; it does not free names/sizes there. Generic list configuration stores their pointers as list text, while generic GUI teardown has separate type-specific resource fields. This module does not establish a complete explicit release of both allocations, so it should not be documented as safely owning/freeing them automatically.

## File requester events and process-directory mutation

FileRequesterHandler (`0x4af670`) obtains state from the current layer. A hot index -1 restores saved drive/directory, frees the requester object and returns. Other actions follow gadget names:

| Gadget | Behavior |
| --- | --- |
| LOAD or SWIN | Look up the selected name. Leading backslash means descend into that directory; otherwise form cwd plus backslash plus name in the caller's selection buffer and accept. |
| CANC | Set accepted-result state without constructing a new selection value. |
| PATH | Walk the current path backwards to a separator and change to its parent, preserving a drive root's trailing separator. |
| NAME | Change the hot gadget to LOAD, copy editable NAME text into the caller's selection buffer and accept. |
| ADRV, BDRV, CDRV, DDRV | Change to drive A, B, C or D. |
| VDRV | Change to drive R, despite the gadget's V label. |

Directory descent first copies the selected directory into caller storage and truncates at the first space among its first ten characters. If cwd length is three it skips the selected leading backslash before concatenation, avoiding a duplicate separator at a conventional drive root. These are legacy name/path assumptions, not robust long-name navigation.

On acceptance, a non-null callback is invoked with requester state. This body does not itself close the layer or distinguish a cancel code from a load code in that callback argument. With callback null, an accepted event simply returns without refresh. Otherwise directory/mask/navigation events refresh, mark the GUI changed and clear the selected gadget. Selected-row validity, output-buffer capacities and every gadget pointer are not comprehensively checked.

The requester changes process-wide drive/current directory during navigation. Its close handler attempts restoration; it is not an isolated file-picker context. [Generic GUI close](../../../src/gui/gui.cpp) calls the layer handler with hot index -1 before rendering/freeing the layer. Unexpected callback/lifecycle exits can therefore affect whether directory restoration occurs.

## Blinking-word allocation and slot operations

A BlinkWord is 164 bytes: 128 text bytes, X/Y, colors A/B, float durations A/B, phase, float next-toggle tick and a final value. The dialog owns an array pointer at `+0xa6`, count at `+0xaa` and active integer at `+0xae`. The first word's final value serves as a shared font/gadget selector.

AllocBlinkWords (`0x4afc60`) frees an existing array, allocates count records, clears each first text byte, stores count, enables drawing and sets word zero's selector to -1. It does not clear every record field. Count zero still reaches the word-zero store; negative counts and allocation failure are not safely rejected. A valid positive slot count is required.

FreeBlinkWords (`0x4afcf0`) frees/clears the pointer and disables drawing, leaving count unchanged. SetBlinkGadget (`0x4afd20`) writes word zero's selector without checking the pointer/count. EnableBlinkWords (`0x4afd40`) and DisableBlinkWords (`0x4afd60`) change active only when the array pointer exists.

AddBlinkWord (`0x4afd80`) finds the first slot with an empty first text byte. It returns zero for no array/full array. Otherwise it copies at most 128 text bytes without forcing termination, stores coordinates/colors/durations, sets phase zero and sets the first deadline to current ticks plus tick rate times duration A. It returns one rather than the allocated slot index. Passing empty text can leave the slot looking unused.

ClearBlinkWord (`0x4afe90`) clears the indexed first text byte when index is less than or equal to count. It does not require nonnegative index, index strictly less than count or a valid array, so negative values and index exactly count can write outside storage. ClearBlinkWords (`0x4afec0`) safely does nothing for null, otherwise clears each in-range text first byte without freeing/reinitializing the other fields.

## Blink timing and drawing

DrawBlinkWords (`0x4aff00`) returns when inactive. Otherwise it reads current ticks, optionally selects a font using the first word's selector when not -1, and visits nonempty slots. A strict deadline-less-than-current comparison toggles phase at most once per drawing call. Switching to phase one sets a new deadline from current ticks plus duration B times tick rate; switching back uses duration A.

The deadline is based on the current draw time, not the previous scheduled deadline, so skipped frames do not cause repeated catch-up toggles. Durations are floats with no nonnegative/finite validation. Float tick conversion can also lose precision for sufficiently large times. Enable/disable does not reset deadlines, and there is no separate update function required while hidden.

Phase chooses text color A or B; the current text-key color supplies the other text-color argument. It draws the string at the slot's X/Y on the current GUI surface with width -1. It does not restore the previously selected font/colors after drawing. The font selector is shared through word zero even if that slot's text is empty.

These words are presentation state, not simulation particles, chat recipient flags or mission timers. [Font](graphics-font.md) and [surface text](graphics-surface.md) provide the rendering behavior and require valid terminated text; a full-length unterminated AddBlinkWord input violates that expectation.

## Completion and remaining GUI responsibilities

All fourteen owned entries are covered, including sorting asymmetries, attribute classification, file-requester callback/ownership limits and all blink operations. Generic layer resource cleanup, listbox selection/scrolling, gadget parsing and frontend file-specific interpretation remain separate modules. No directory navigation, file opening through the game, GUI interaction or blink rendering was executed for this documentation.
