# GUI contexts, layers, gadget state, rendering and input

Status: finished. All 130 owned entries have been traced across the main implementation and both companion files. This reference covers context/state services, every gadget family, layer rendering, focus, text layout and input dispatch.

## Scope and shared representation

The `gui/gui` group occupies `0x49f8c0..0x4aa8f0`. Implementation is [gui.cpp](../../../src/gui/gui.cpp), with [SetGadgetText](../../../src/gui/gui_4a0880.cpp) and [DrawSlider](../../../src/gui/gui_4a3ef0.cpp) kept separately. Shared declarations are [Gui](../../../src/gui/gui.h), [Layer](../../../src/gui/layer.h) and [Gadget](../../../src/gui/gadget.h). Runtime GUI loading is distinct from [GUI-file record parsing](gui-gui-file.md).

Gui holds current font/GAF and three glyph-GAF slots, current glyph language, top layer, cursor animation/snapshots, focus/hot/hover indices, keyboard/editing state, palette/remap, three resource paths, changed state and fallback background. A Layer is a 64-byte stack node with previous-layer pointer, gadget array, click/close callback and its data, flags, dirty state, keyboard settings/callbacks and selected gadget. Gadgets are packed `0x15b`-byte records, with record zero containing layout-level assets/count and type-specific unions in later records.

The topmost layer, focused gadget, hovered gadget and currently activated gadget are separate state. A callback's application-specific owner data is not interchangeable with its gadget array. Generic GUI code borrows the global context for font/text operations, so passing an unrelated Gui pointer does not make every helper independent of that global.

## Context selection and resource paths

SetCurrentGuiContext (`0x4aa850`) assigns the global context and initializes fields in the supplied object: clears its layer, resource path strings, common GAF, fallback, cached background name, drag/hover state and font-slot pointers, sets pathsReady/inputEnabled/dirty, records current time and resets animation timer. It does not release previously live layers/assets before overwriting them. This is initialization with a global-context assignment, not a harmless switch preserving arbitrary existing state.

GetGuiContext (`0x4aa8d0`) returns that pointer as an integer. SetDefaultFont (`0x4aa8e0`) stores the supplied font at the object's first integer field without loading font bytes. SetFontsPath, SetGuiPath and SetAnimsPath (`0x49fb50`, `0x49fba0`, `0x49fbf0`) copy at most 256 bytes and append a backslash. They do not force termination before concatenation or check room for the added separator. Callers normally supply short relative directories during [application setup](game-game-state.md).

## Dirty flags, keyboard gates and modal pumping

EnableKeyCommands/DisableKeyCommands (`0x49fa50`, `0x49fa70`) set/clear Gui's `+0xa2` field. MarkChanged/ClearChanged (`0x49fa90`, `0x49fab0`) set/clear `+0xcca`. MarkLayerChanged/ClearLayerChanged (`0x49fad0`, `0x49faf0`) change the top layer's dirty field; only the setter checks whether a layer exists. These flags are not synonyms and do not immediately draw a frame.

SetKeyboardInput/GetKeyboardInput (`0x49fb10`, `0x49fb30`) change/read the top layer's separate keyboard-input value. With no layer, the setter does nothing and getter returns zero. This is distinct from the Gui-wide key-command flag.

RunWhileScreenNamed (`0x49f9c0`) remains in a modal loop while a top layer exists and its record-zero name matches the supplied name case-insensitively within sixteen bytes. Each pass removes/dispatches at most one pending Windows message, updates the menu, blits layers, shows the software cursor and flips. There is no sleep, ordinary simulation-step call or explicit WM_QUIT termination test in this body. Modal callbacks and layer changes determine its exit; the loop is not the application's general message pump.

## Name lookup and required-layout failures

Most entry searches inspect records one through the count stored in record zero and compare the first sixteen name bytes case sensitively. HasGadgetNamed (`0x49f8c0`) returns a boolean. FindGadgetIndex (`0x49fdf0`) returns an index or -1 but ignores its nominal type argument. FindGadgetIndexBySubstring (`0x49fe60`) uses strstr instead. FindGadgetOrNull (`0x49ff10`) returns a pointer or null.

The five checked variants (`0x49ff90`, `0x4a0010`, `0x4a0180`, `0x4a0200`, `0x4a0280`) perform the same exact-name search, call FatalError with Error in GUI layout on failure and return null if that call returns. Their suffix letters do not imply different gadget-type validation.

SetGadgetName (`0x49f930`) renames a found entry with a seventeen-byte bounded Windows copy when a layer exists. GetGadgetName (`0x49fed0`) copies sixteen bytes and forces destination byte sixteen NUL, requiring at least seventeen writable bytes. IsCurrentGadgetNamed (`0x49fd60`) checks layer/hot-index presence then uses full strcmp. IsGadgetNamed (`0x4a0300`) rejects index -1 but otherwise uses the sixteen-byte comparison. These slightly different equality rules can matter for malformed/nonterminated record names.

Missing names are not universally benign. SetQuickKeyByName (`0x4a1530`) uses the returned index without checking -1 and writes its quickKey byte, so a layout missing that named control can address before the array. Callers must honor each helper's own contract rather than assuming every ByName function is checked.

## Focus, activation and associated controls

ClearFocus (`0x49fc40`) assigns -1; IsFocused/HasFocus (`0x49fcf0`, `0x49fd10`) test equality/nonnegative-sentinel state. TrySetFocus (`0x49fc50`) first releases existing focus if it is a type-3 text input. It refuses a different requested focus only when another non-text focus remains. On accepted text-input focus it sets the text cursor length to strlen. It does not validate arbitrary indices or automatically check gadget active/disabled state.

SetGadgetActive (`0x4a03f0`) stores the active byte and marks Gui changed. For sliders, it propagates active state to type-1 controls with the same association byte. Disabling a listbox finds the associated slider and disables it recursively. The unsuccessful search leaves index zero, while its later test checks against -1, so absence can disable record zero. If disabling the layer's current gadget it selects an adjacent one.

GetGadgetActiveByName (`0x4a04f0`) returns active or char -1 for absence; SetGadgetActiveByName (`0x4a0570`) does nothing when layer/name is missing. Association helpers FindAssocListBox/Slider/TextInput (`0x4a1990`, `0x4a19f0`, `0x4a1a50`) return the first matching type/team index or zero. The team byte here groups controls, not game-player ownership.

ClearPeerStatus (`0x4a0340`) clears nonzero status on other buttons sharing a nonzero association and redraws those buttons, marking the layer dirty. Status and stageIndex are different fields. GetGadgetStatus (`0x4a0f30`) reads the short status. GetButtonStage/ByName (`0x4a0ff0`, `0x4a0f60`) require button type and return stageIndex or -1. SetButtonStage (`0x4a1030`) validates button type; its ByName counterpart (`0x4a1080`) writes a found record without that type check. Stage setters do not themselves mark changed or clamp to the number of stages.

SetGadgetStatusByName (`0x4a1110`) marks changed and clears peers only for a nonzero new status. SetGadgetStatus (`0x4a11c0`) always clears peers and does not itself mark changed. Consequently callers cannot interchange them solely on the assumption that one is an indexed wrapper.

## Disabled-state families

SetGrayedOut (`0x4a1200`) assigns the button-style low bit and marks changed. SetGrayedOutByName (`0x4a1250`) changes that bit without marking changed. SetGadgetGrayedOut (`0x4a12e0`) instead dispatches by type: slider lock field plus associated arrow-button bits, button low bit, listbox attribute `0x100`, label/link low bit at `+0x148`, or type-12 frame flag. The two name wrappers at `0x4a1450` and `0x4a14c0` call that type-aware setter.

These operations do not universally clear active, focus or selection, and not all request redraw. Bitfield assignments retain low bits, while the listbox branch uses ordinary nonzero truth. A dimmed appearance, a locked slider and an inactive gadget are therefore distinct state even when consumers present them similarly.

## Text storage, translations and quick keys

SetGadgetTextByName (`0x4a07d0`) copies up to 128 bytes, marks changed and assigns a quick key. The companion SetGadgetText (`0x4a0880`) uses unbounded strcpy by index and updates the text cursor when that entry is focused, without a changed flag. GetGadgetText (`0x4a0d00`) returns borrowed text for button, input or label types and optionally copies it unbounded into caller storage. A missing layer is fatal; missing names or other types yield null.

SetControlTextByName (`0x4a0e00`) searches the previous layer but has two hazards: it obtains that previous pointer's entries before checking null, and its type-3 branch writes into the current layer at the previous layer's found index. Button/label branches write the previous layer. It is not a uniformly safe parent-layer text setter.

AssignQuickKey (`0x4a05e0`) handles buttons/labels, respecting button attribute `0x10000`, excluding staged buttons and labels without link text. It clears the applicable key and chooses the first nonspace text character unused case-insensitively among existing button/label keys. It does not require an alphabetic candidate or reassign all peers to find an optimal set. Signed-byte tolower inputs and fixed string bounds follow the historical implementation.

TranslateGadgetItems (`0x4a08f0`) translates stages consecutive NUL-terminated strings into a 128-byte temporary, then copies all 128 bytes back. It does not bound the combined translations or initialize unused temporary bytes. SetTranslatedText (`0x4a09c0`) translates first, then handles label, input and button differently: labels use a bounded copy and optional quick key, inputs use unbounded copy/update cursor and optionally replace status, buttons use a bounded copy/quick key and translate staged strings after replacing pipe delimiters with NUL.

Its delimiter scan advances after each replacement, splitting pipe-separated stages into consecutive strings. Subsequent stage translation assumes the expected number of terminated strings. Missing/oversized translations can therefore affect memory rather than being safely truncated by every path. SetTranslatedTextByName (`0x4a0bf0`) delegates only when layer/name exist. SetGadgetColorByName (`0x4a0c70`) stores a found color and marks changed.

UpdateHelpText (`0x4a0090`) retains the hovered index, obtains its help-key storage or an empty default, translates it and copies into HELPTEXT if present, marking changed. No target text-buffer capacity is passed. The [file reader](gui-gui-file.md) separately discards loaded common help text before translation; runtime help handling does not undo that earlier behavior.

## Geometry and font/background helpers

GetGadgetRectByIndex/GetGadgetRect (`0x4a15c0`, `0x4a1630`) use origin zero for type zero, otherwise the gadget's local X/Y, with inclusive right/bottom equal to origin plus dimension minus one. GetGadgetScreenRect (`0x4a1680`) starts from the record's X/Y and adds panel origin for nonzero type. GetGadgetRelativeMouse (`0x49fd20`) copies the GUI's last mouse event (`Gui::point`) and subtracts the gadget's origin from its X and Y, giving the mouse position relative to the gadget.

IsPointInRect (`0x4a1920`) is inclusive. Helpers `0x4a1950`/`0x4a1970` test whether a value is above top or below bottom. RedrawGadgetRect (`0x4a1ab0`) grays and darkens a local gadget rectangle; its name does not mean a full type-specific redraw.

DrawGadgetSelectionBox (`0x4a16f0`) first clears colors on all text inputs and marks changed. A selected input receives color 30; label/listbox types return without an exterior border. Other types receive six expanding lit rectangles with progressively reduced levels. Its final parameter is not used as a general border-size setting.

SelectFontForEntry (`0x4a1810`) finds the font-number-th type-7 record and installs its loaded font, otherwise installs the current global default. FindBackgroundCell (`0x4a18c0`) scans type-8 records by texture number but returns zero even on a match. In this source it never supplies a real background cell. Consumers' nonzero-cell branches therefore remain unreachable through this implementation; the mere presence of a filename/texture record does not activate them.

## Listbox rendering traced so far

DrawListBox (`0x4a1b40`) marks the layer dirty and obtains a background from the layer surface or Gui fallback. It copies that background into the gadget rectangle when present, or requests the listbox skin when absent and layer flag `0x80` is clear. It then computes line spacing from current glyph-font height or bitmap font height, overridden by the listbox's configured row spacing.

Text mode requires attribute `0x10`, a text pointer and nonzero row count. It starts at firstRow in the NUL/newline list, selects a type-7 font, chooses left/right/center alignment through attributes 1/4/2 and uses wrapped drawing for unusually tall rows. Missing alignment flags can leave position/width locals uninitialized. Row filebuf flags or an ampersand-G text prefix mark gray rows; selected rows are faded unless attribute `0x100` disables selection. The code's two focused/unfocused selection fade branches are identical.

Image modes use attributes `0x20` or `0x80`: one borrows pointers to records containing frames, the other treats rows as contiguous GafFrame objects. They temporarily clip to the list rectangle, scale cells into rows, fade disabled items or cross out marked items and highlight the selected item, then restore clipping. The loops assume usable row/frame/flag arrays; the selected-frame branch can dereference a missing cell. Pixel drawing does not itself change firstRow or the selected index.

GetSliderRects (`0x4a23b0`) constructs the track and knob rectangles, treating attribute one as horizontal and using knobPos/knobSize plus small frame offsets. The companion DrawSlider (`0x4a3ef0`) derives knob size/range from an associated list: text rows use visible-line ratio with a minimum knob size ten; pointer-image rows use total cell heights; contiguous-frame rows use row spacing/count with divisor checks. The pointer-image branch divides by total height without a zero guard, and text calculations also assume a positive row count. It finally delegates actual slider painting.

## Slider drawing, associated values and callbacks

DrawGafBar (`0x4a2480`) draws a left cap, repeated middle frames and right cap from a three-frame sequence, vertically centering the first cap within its gadget. It tests the first frame before drawing but then dereferences its width anyway. Middle/end frames and positive repeat dimensions are assumed. The repeated region is not scaled to arbitrary artwork.

DrawSliderBar (`0x4a2580`) selects the gadget font, obtains track/knob rectangles and draws primitive raised/sunken boxes when no slider GAF exists. With artwork, shape comes from width versus height: vertical sliders use a repeated track plus a multi-piece knob, horizontal sliders use repeated track caps plus a knob frame. sliderStyle supplies the frame-index base. Several missing frames are dereferenced after optional draw checks; zero-sized repeat frames can prevent progress.

Attribute four adds a value label to the right. Explicit gadget text overrides numeric output; otherwise a nonzero max scales knobPos by max divided by width minus knobSize, attribute eight displays knobPos plus one, and the remaining path displays knobPos. The numeric-scale denominator uses width even for vertical presentation. A sixteen-byte local label buffer receives unbounded explicit text. Attribute `0x10` or sliderLocked grays/darkens the track rectangle.

FUN_004a3eb0 clears knobPos, callback and callback user data. DecrementKnobPos/IncrementKnobPos (`0x4a96d0`, `0x4a9780`) change the knob by one, clamp to zero through range minus one, redraw/synchronize only on change, but invoke a present callback even if clamping prevented movement. A nonpositive range eventually produces zero through the clamp order rather than establishing a useful scroll range.

HandleSliderInput (`0x4a4170`) rejects disabled attribute `0x10` and sliderLocked. It translates the mouse to panel-local coordinates, captures either button's press on the track and starts dragging only if the press is inside the knob. Focused dragging applies the current pointer delta to its saved knob position; a focused non-drag interaction steps toward the pointer one unit per handler invocation. Releasing both mouse buttons clears focus/drag state. A changed position clamps, marks the layer dirty, redraws, synchronizes peers and invokes the slider callback with its configured user value. Another active drag prevents a new slider capture.

SyncAssocGadgets (`0x4a2be0`) walks equal-association peers. A listbox copies selected/firstRow to other lists, can copy selected text into an associated input when attribute eight is set, and maps scroll to an associated knob. A slider maps knob position to associated list firstRow and redraws it. These calculations involve row spacing, counts, maximum first row and range minus one with incomplete zero-divisor guards. They are proportional scroll relationships, not resource transfer or gameplay percentages.

## Configuring and navigating list data

ResetListBox (`0x4a30c0`) resets firstRow/selected, installs its font, rounds gadget height down to a line-height-plus-two multiple and stores current ticks in its repeat timer. ConfigureListBoxByName (`0x4a32a0`) binds a borrowed string list/count, enables text attribute `0x10`, raises row spacing to at least font height plus one, optionally stores a flag pointer/value in the language/filebuf field and sets attribute `0x800`. It resets selection/scroll and calculates maxFirstRow by subtracting row spans from available height backwards.

It then enables/disables an associated slider through the global GUI context and draws it if scrolling is needed. Missing gadget names are fatal. A failed association search returns zero but is checked against -1, so it can treat record zero as an associated control. This function does not copy or own the supplied text simply because it binds it. Later type-specific teardown ownership depends on the actual storage field/type.

SetGadgetRows (`0x4a35a0`) binds contiguous frame records with attribute `0x80`; SetGadgetItems (`0x4a36a0`) binds pointers to records containing frames with attribute `0x20`. Both set count/firstRow/selection and calculate how many trailing rows fit using configured row spacing or frame heights. They do not allocate item storage or enforce that count fits its short field. Empty counts can form pointers before the supplied arrays even where later loops do not dereference them.

TruncateListItems (`0x4a31c0`) obtains an allocation size from a header 68 bytes before its text pointer, allocates SCROLLITEMS storage, copies/truncates each line through a 100-byte temporary and copies the result back. It does not free that temporary allocation in the traced body, bound total output, or guarantee termination of a full-width input copy. Its memory-header assumption means it is not safe for every arbitrary C string allocation.

SetListBoxScrollByName (`0x4a2e40`) assigns the requested selected index without checking it against row count, computes visible line count and brings an out-of-window selection into view, capped at maxFirstRow. It proportionally updates an associated knob and marks changed. The proportional division by maxFirstRow is not generally guarded, and absent association can use record zero.

ListBoxSelectUp/Down (`0x4a9830`, `0x4a99c0`) move a selection by one and adjust firstRow when crossing its visible window. They restore the prior selected index for a text row beginning `&G`, a gray/disabled marker, but do not necessarily undo a scroll already performed. They redraw/synchronize peers, or delegate an already out-of-window selection to SetListBoxScrollByName. The line-height calculations assume the current font and valid row metadata.

HandleListBoxInput (`0x4a3780`) returns early for an already hot gadget or zero rows. It computes panel-relative hit geometry and row span, handles either-button focus/click-mode capture, and accepts a double-click within the list. Attribute `0x200` causes the double-click path to reject `&G` rows; attribute `0x40` can also request immediate activation on a focused in-list event.

Focused text selection maps Y to firstRow plus row index, clamps it and synchronizes selected indices on same-association listboxes. Image selection walks row heights or fixed spacing. Its nontext branch condition is written as flags AND `0x20` OR `0x80`, which is always nonzero, so this fallback is entered even when neither intended image mode is set. Invalid row arrays then are not protected by an effective mode gate.

A changed selection redraws and invokes its list callback. Dragging above/below the rectangle autoscrolls on a current-tick-plus-two deadline, maintains a boundary selection, rejects gray text rows and synchronizes peers. Releasing mouse buttons clears focus, but current hot/activation state and callback return can still determine whether the generic update closes the layer. No local copy of the list data is created.

## Progress bars, hotspots, line and frame gadgets

ScheduleGadgetEndTick (`0x4a4620`) stores current ticks plus interval. TickProgressBar (`0x4a4890`) runs only while active and value below max; after a strict deadline it adds the integer-truncated float step, caps/disables only when value exceeds max and schedules from current time. It redraws even before the next deadline. Exactly reaching max leaves active set but stops entering the update branch. A fractional step below one can produce no progress indefinitely.

DrawProgressBarGadget (`0x4a4660`) locks its selected/fallback surface, draws bevel/interior/fill from value divided by total and optionally centers numeric text. It does not check total zero, clamp ratio or require a successful lock result before drawing. ResetProgressBar (`0x4a4930`) clears total, max, interval and only a short at the next-deadline offset; it does not fully reset value, step, active and every timing byte. These progress bars differ from the six loading-stage bars built by match-loading code.

DrawHotspot (`0x4a4980`) chooses an indexed GAF frame, explicit frame or palette fill. Uncompressed frames are mapped to a destination quad; compressed GAF frames use their ordinary draw path/hotspot. HandleHotspotInput (`0x4a4b50`) invokes a configured custom callback on every call before checking its clickable flag, then captures either button and reports activation on release inside the rectangle. Its point test does not perform the same panel-origin subtraction as every other gadget family.

DrawLineGadget (`0x4a4c90`) draws only when its supplied render flags include bit one, with attributes one/two/four selecting horizontal, vertical or diagonal. DrawGadgetGlyph (`0x4a5e50`) marks dirty, draws a borrowed frame at gadget position plus hotspot with optional lighting and applies a fade for its frame flag. Neither owns the glyph allocation.

## Text metrics, clipping, wrapping and label mutation

GetCharGlyph (`0x4a5010`) indexes the active glyph sequence without a separate null-language guard. GetTextPixelWidth (`0x4a5030`) returns zero for null text, otherwise uses bitmap-font metrics or sums available glyph widths. GetFontLineHeight (`0x4a50b0`) uses bitmap height or glyph I height plus two; missing I is not guarded before dereference.

DrawTextClipped (`0x4a50e0`) has different contracts by font path. Bitmap-font fallback calls DrawString with width -1, ignoring this wrapper's supplied width/style. Glyph mode skips control bytes, advances for spaces without drawing them, skips missing glyphs and stops before a glyph wider than the remaining budget. Style zero draws ordinary frames, nonzero draws lit frames. It does not render partial glyphs as its own width-clipping mechanism.

DrawWrappedText (`0x4a51d0`) repeatedly inserts temporary NULs into mutable input text to measure and draw segments. Space and carriage return delimit candidate breaks; LF is not an equivalent explicit delimiter here. It restores altered bytes after each draw, advances Y by line height plus two and stops when remaining vertical space is exhausted. It returns final Y rather than number of characters consumed. An oversized first unbreakable word can force the previous-break fallback at the current start, causing empty/truncated segments and skipped characters rather than a clean safe word-split algorithm. Read-only text storage violates its mutation contract.

LayoutLabelText (`0x4a53c0`) installs a font and mutates gadget X/width/height according to right/center/left alignment and measured text. Centering uses twice truncated half-width. DrawLabel (`0x4a56b0`) temporarily selects glyph slot one, centers an X=-1 label within its panel, optionally fills its background, draws a shadow/text with alignment/wrapping choices and optionally underlines its exact matching hotkey. Disabled labels gray/fade their rectangle. It then selects glyph slot zero, not the arbitrary language pointer present on entry.

SetCurrentFont (`0x4a5d30`) directly selects a glyph slot without an index check. TruncateGadgetText (`0x4a5d50`) destructively removes final characters until the borrowed text fits width minus six, choosing the alternate glyph slot for labels/marked buttons. A gadget width below six can cause an endless loop once the text becomes empty: zero width still fails a negative available-width comparison, and the empty branch continues without further mutation.

DrawTextInput (`0x4a4d70`) installs the entry font, fills or restores the background/skin, draws its text and, when focused, temporarily terminates at Gui's cursor index to measure the caret's X. It restores the byte and draws a vertical caret using font height. A stale/out-of-range cursor index is not checked before that temporary write. It does not implement text editing itself.

## Button artwork and input state machine

FindButtonGaf (`0x4a7f70`) first tries the screen GAF by gadget name, then common GAF by name, CHECKBOX, stagebuttn with a capped stage number, or BUTTONS0. Fallback art has all frame hotspots cleared, and groups of four frames are compared to requested geometry using absolute width/height difference. The nearest sampled group selects frameBase and replaces gadget width/height from that frame. A one-stage fallback becomes two stages plus attribute `0x4000`. Missing frames and more than byte-sized frame bases are not comprehensively checked.

DrawButton (`0x4a5f40`) truncates its text before drawing and chooses GAF frames from status, stageIndex, frameBase, disabled state and attributes. Disabled modes can use the final frame or draw a normal frame then gray/fade it. Without artwork it uses different bevel/color combinations for disabled, pressed and ordinary states. Staged text is selected from consecutive NUL strings, with left/right/center/bottom-centered variants and quick-key underlining/highlight.

The bottom-centered attribute-`0x20` quick-key path writes NUL at the found character in the original selected text and does not restore it. Rendering can therefore permanently truncate that button string. Other paths use temporary copies, themselves relying on a 128-byte bound. The routine resets language to glyph slot zero afterwards. ClearAllStatus/ClearGroupStatus (`0x4a69d0`, `0x4a6a40`) clear nonzero button status globally or by association, redraw each changed button and mark changed.

HandleButtonInput (`0x4a6ae0`) rejects grayed buttons, translates mouse coordinates, captures either-button focus and remembers prior status. Its ordered attribute branches implement distinct behaviors:

- `0x10` selects/radios on the held-button path; the literal release test differs from the ordinary release-activation branch.
- `0x40` shows a pressed state while held and toggles the remembered value on a valid release; leaving the rectangle restores/drops its visual state.
- Eight toggles a zero/one status on release inside.
- `0x100` cycles status through GAF frame count on a left-button event.
- The remaining path shows momentary status, cycles stageIndex on valid release for ordinary buttons and drives associated slider arrows for masks `0x1800`.

Arrow/repeat handling uses a shared fifteen-tick countdown and one-update-per-tick gate under `0x2000`, then steps an associated slider, redraws, synchronizes and calls its callback. The association miss again returns zero but is tested against -1, permitting record-zero access. Keyboard quick keys are accepted with no focused text input unless Alt is held, require pathsReady one and a matching nonzero event, mutate toggle/radio state as applicable, clear peers, consume the key and report activation.

## Text editing and directional focus

BeginTextEdit (`0x4a7190`) and inlined equivalents set font/colors, focus and current gadget, call CommitTextEdit with the gadget text/status maximum and clear the entire key queue. HandleTextInput (`0x4a7290`) activates on either-button click within panel-relative bounds, then delegates the supplied key to HandleTextEditKey while focused. Return code thirteen clears focus and reports activation; code 27 clears focus, empties text and also reports activation. Other results mark the layer dirty. Actual edit-buffer/key semantics are owned by [dialog support source](../../../src/gui/dialogs.cpp).

FindNextTextInput (`0x4a7560`) searches forward then wraps for type three without checking active/disabled state. SelectGadgetByName/Index (`0x4a76b0`, `0x4a7830`) clear focus, update layer current and initialize editing when the chosen record is an input. Indexed selection assumes a valid index. MoveEntryToFront (`0x4a7ee0`) moves a non-minus-one record to position one by copying whole records and shifting predecessors; it does not repair every external saved index.

SelectAdjacentGadget (`0x4a7960`) uses X/Y ordering metrics with factor 5000 and a wrap span of 25,000,000, grouping nearby columns within ten coordinate units. It excludes inactive, attribute-`0x400`, grayed/locked controls and vertical sliders from its supported type set. Only fifty integers of its 200-integer temporary are initialized, and its indexing/count loops have no comprehensive bounds checks. The chosen input is initialized twice in identical consecutive branches. This is a legacy spatial focus heuristic, not a general layout graph.

## RenderLayer initialization and resource binding

LoadScreenGaf (`0x4a75d0`) forms an animation-directory/name GAF path and stores its loaded block in record zero when length/load succeed. GetAnimsPath (`0x4a81b0`) copies that configured prefix. AddGadgetEntry (`0x4a8150`) increments the count, clears the next record, assigns type and active one without allocating more array capacity; the layer loader must have reserved room.

RenderLayer (`0x4a81e0`) returns zero without a top layer. Initialization flag one can center record zero using `0x100` or center within the battle region to the right of X 128 using `0x1000`. Existing X=-1/-2 have the same special placement semantics. Offscreen right/bottom extents trigger recentering. Initialization drains keys, optionally clears button quick keys and rejects layouts larger than the physical screen.

It then binds resources/type setup across the dynamically growing gadget count:

| Type | Initialization behavior |
| --- | --- |
| Any resourceFlags bit 0 | Try a per-gadget name suffixed `_gadget.GAF`, retaining archive and matching named entry |
| 0/11 | Load panel-named GAF, resolve panel artwork, then common artwork/BackTile fallback; clear fallback frame hotspots |
| 4 | Resolve SLIDERS artwork, choose vertical/horizontal style, append two arrow-button records, shrink/move the track and set knob/range geometry |
| 3 | Resolve TEXTINPUT art, cap status at 127 when at least 128 and clear initial editable text |
| 2 | Resolve LISTBOX art and equalize row spacing among same-association lists |
| 12 | Resolve named frame zero from panel/common GAF and clear lighting |
| 1 | Resolve named/fallback button art, adjust geometry/stages, split/translate staged strings and replace attributes with low alignment one plus retained `0x4000` |
| 7 | Load configured font-directory filename with `.FNT` appended into its file buffer |
| 8 | Load its filename under the configured GUI path into its file buffer |
| 13 | Schedule progress deadline |
| 5 | Disable unlinked labels via attribute `0x10` or assign quick key; clear lighting |

The slider fallback creates arrow buttons with masks `0x3400`/`0x2c00`, shared association and borrowed artwork, so they are later included by the growing-count loop. Repeated initialization over live entries can append again and overwrite resource pointers; this is not an idempotent refresh call. GAF names, frame counts, array capacity and many allocations are trusted.

Initialization allocates the GUI SURFACE at panel dimensions and captures the underlying screen. Unless flag `0x20` suppresses it, it allocates SAVE UNDER and copies that content for later restoration. These surfaces and per-screen/per-gadget allocations are owned by the layer lifecycle, while common GAFs remain owned by the Gui/application.

## Render flags, cleanup and close order

Flags four, one or `0x40` enter drawing. Full initialization or `0x40` restores the layer background or draws its skin unless `0x80` suppresses that background. Active gadgets dispatch by type to button/list/input/slider/label/hotspot/frame/progress/line drawing; most types redraw on one/`0x40`, while buttons also honor mask `0x48`. Initialization resets list scroll/geometry, slider callback/position, hotspot custom image/callback fields and progress deadlines before their first draw.

Selection highlighting can use the current gadget and named default action. After the loop, that second highlight tests entries[i].active where i is already beyond the processed gadget range; no range correction precedes it. The code therefore assumes readable extra storage even for a display-state test.

Cleanup flag two restores/frees SAVE UNDER, frees/clears GUI SURFACE, frees per-gadget archives with resource flag one, record-zero screen archive and type-7/8 file buffers. It does not generically free every borrowed list text/items/record pointer. Most asset fields are not cleared after free. Cleanup must not be repeated as though it were idempotent resource release.

CloseTopScreen (`0x4a9660`) clears hover/hot/focus, invokes the layer handler first, then hides the cursor, calls RenderLayer cleanup, shows the cursor, pops/frees the node, marks the underlying layer dirty and redraws it when the closed flags include `0x800`. The handler sees hot index -1, explaining application-specific close cleanup. The callback must respect that ordering rather than independently freeing the same layer and letting this body continue on it.

## Generic keyboard commands and per-frame UpdateMenu

HandleGuiCommand (`0x4a9b90`) uses layer current and processes Tab (Shift reverses), Escape's named default, Enter's named default or current control, Space activation, and arrow navigation/list selection/slider steps. Enter stays available to a focused text input; left/right stay available to text editing. It filters inactive/grayed controls where specified, can advance staged buttons and sets hotGadgetIndex for accepted actions. A consumed command pops the queue only when layer keyboardInput is zero because other mode already popped it.

Up/down list callbacks run after their navigation helper even when that helper could not move. Horizontal sliders use left/right increments; other arrow directions choose adjacent controls. The function assumes a valid layer current before taking its gadget pointer, so a sentinel current is not a safe arbitrary direct-call input.

UpdateMenu (`0x4a9fd0`) returns zero without a layer. It records elapsed timer ticks, updates cursor/mouse state, then either peeks a key or pops one depending on keyboardInput. Peek mode suppresses local use of codes `0xe2..0xeb` without removing them, allowing later application/battle processing. Pop mode plus Gui dirty applies HandleGuiCommand; unconsumed input shifts the layer's fourteen-character key history, stores uppercase lastKey, calls an optional text handler and clears hot state.

It handles callback/layer disappearance after that stage, redraws changed state with layer flags plus `0x40`, sets cursor-hover from panel bounds and scans active gadgets. It chooses type-specific input handlers, updates button/frame lighting decay and progress animation, and stops scanning once an activation is selected. Text inputs receive zero key when Alt is held so quick-key handling can take precedence. Labels can redirect activation to the gadget named by their link, validating active/locked state or selecting/initializing a linked input.

Changed hover updates HELPTEXT. An optional layer cb1c runs every pass after gadget scanning. On activation, UpdateMenu stores the selected index, selects that gadget and invokes the click handler. If hotGadgetIndex remains nonnegative afterwards, it automatically closes the top screen; a handler keeps the dialog open by clearing that selection. This is why application handlers that merely return after OK/CANCEL can still dismiss a screen, while callbacks explicitly clearing the current gadget remain open.

The generic frame has no universal rollback after a callback changes layers or data. Callbacks and custom hotspot/text handlers can mutate shared state during iteration, so the explicit rechecks and retained pointers define the actual safe boundaries. It does not simulate a battle tick, issue a unit command without application callbacks or own a network schema.

## Coverage and unresolved runtime inputs

All owned entry families, both companion bodies and the complete main source have now been traced. Layout-specific callbacks, text-edit buffer machinery and layer construction belong to adjacent dialog support; the schema/defaults are covered by [GUI files](gui-gui-file.md), skins by [listbox drawing](gui-listbox.md), and input production by [keyboard](gui-keyboard.md)/[mouse](gui-mouse.md).

Actual dimensions, artwork, translations, file lengths and active callback state determine which edge paths occur. No GUI interaction, file export, build or game execution was performed. Ownership and local source links have been audited.
