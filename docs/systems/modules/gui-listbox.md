# Listbox skins and bevelled drawing primitives

## Scope and ownership

The `gui/listbox` group owns seven entries in `0x4b0090..0x4b0610`, all in [listbox.cpp](../../../src/gui/listbox.cpp). They draw bevelled boxes and a listbox/background skin. List-item storage, selected rows, scrolling and keyboard/mouse interaction are implemented in the generic GUI module, not these drawing functions.

The routines borrow a destination surface, gadget geometry, palette indices and GAF frames. They allocate/free no retained resources and implement no packet or save record. [Drawing](graphics-draw.md), [surfaces](graphics-surface.md) and [GAF](graphics-gaf.md) define their pixel/clipping dependencies.

## Border and fill contracts

DrawBevelBorder (`0x4b0090`) draws two top/left edges in its third argument's color and two bottom/right edges in its fourth. It uses inclusive rectangle endpoints and one-pixel inset/displacement combinations to form a two-pixel bevel. Its fifth argument is unused. DrawBevelBorderDarkFirst (`0x4b0160`) draws the same geometry with third/fourth argument roles reversed: third is bottom/right, fourth top/left.

FillBevelBox (`0x4b04b0`) fills the supplied rectangle, then calls the first border helper using the supplied light/dark colors. FillBevelBoxDarkFirst (`0x4b04e0`) fills and forwards the same positional color arguments to the dark-first helper, so the helper's interpretation reverses the edge colors. The wrapper's local parameter names do not override that callee contract.

DrawRaisedBox (`0x4b0510`) fills and draws a single-pixel top/left edge in the third argument's color and bottom/right in the fourth. DrawSunkenBox (`0x4b0590`) fills with the same final argument but reverses those edge colors. They do not change a gadget's selected/pressed stage, depth-test against scene models or shade colors automatically. The caller supplies actual palette indices and decides when raised/sunken appearance is appropriate.

None of these helpers validates rectangle ordering or minimum dimensions. Small/inverted rectangles can cause border lines to overlap or extend beyond the nominal interior. Destination clipping remains the underlying primitive's responsibility; the rectangle is not itself installed as a new clip region by these wrappers.

## Listbox frame selection

DrawListboxFrame (`0x4b0230`) obtains the gadget rectangle from the current entry table. If its explicit bitmap/sequence argument is null, it requires a non-null object GAF and a Listbox entry in it. Missing either returns without drawing. Successful default lookup expands the rectangle by three pixels on every side. An explicitly supplied sequence does not receive that expansion.

The destination surface comes from the entry table's asset slot at offset `0xbc`. The routine then asks FindBackgroundCell for the gadget's background. With a background cell, it tiles across the destination surface's full dimensions in the cell's X/Y steps and draws a dark-first bevel around the selected rectangle. It does not restrict the tiling loop to the gadget rectangle itself; any current surface clipping controls which pixels are actually affected. However, the current FindBackgroundCell body in [generic GUI](gui-gui.md) returns zero even for a matching texture record. This nonzero-cell branch is therefore not reached through that implementation.

When no background cell exists, the selected GAF sequence determines the skin path. A multi-frame sequence follows the tiled grid below; a sequence with count no greater than one draws frame zero at destination origin zero/zero, not necessarily at the gadget's rectangle origin. A zero-frame sequence can return a null frame through GetGafFrame, whose draw behavior belongs to the GAF primitive.

There is a final fill-plus-bevel branch for a null bitmap after background selection, but ordinary entry control flow already returned or resolved a non-null bitmap before reaching it. It is not an available automatic fill fallback when Listbox artwork is absent. The source overview's generic fallback description is broader than the reachable path.

## Multi-frame tiled skin

Any sequence count greater than one takes the grid path, even though the pattern can request indices through eight. Frame zero supplies tile width/height. The rows select frame offsets zero, three and six for top, middle and bottom; the columns add zero, one or two for left, middle and right. Interior tiles repeat as needed, while final tiles are moved back to pin their right/bottom edges to the requested extent.

For gadget index zero, tiling starts at destination zero/zero. Other indices start at the rectangle's left/top. Width and height use inclusive rectangle extents. At edges smaller than a tile, repositioning can produce negative offsets relative to that origin; no special small-widget skin is selected.

The code assumes frame zero exists and has positive dimensions. A zero width/height can prevent loop progress. A sequence with two through eight frames can still receive requests for missing grid cells; the GAF lookup checks frame index, but this routine does not replace missing cells with a fallback. It also dereferences frame zero before checking its return. A count-greater-than-one test therefore does not validate a complete nine-frame skin.

The skin frames are borrowed and not copied into a new widget asset. Changes to the underlying GAF lifetime, current palette or destination clipping affect subsequent drawing. This helper does not choose which list row is highlighted or draw the row strings itself.

## Completion and interaction boundary

All seven entries and their color/geometry conventions are covered. Their output explains listbox borders and repeated skins in menus, file browsers and battle dialogs. The data displayed inside those frames and the behavior of clicking/scrolling it belong to GUI/file-list consumers. No widget interaction, rendering execution or screenshot was used for this source-based writeup.
