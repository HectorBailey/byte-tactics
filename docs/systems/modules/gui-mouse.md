# Software cursor composition, worker lifetime and mouse-event queue

## Scope and retained state

The `gui/mouse` group owns seventeen entries in `0x4c2380..0x4c2ea0`, all in [mouse.cpp](../../../src/gui/mouse.cpp). It maintains software cursor backgrounds, draws cursor frames, optionally runs a redraw worker, controls sprite borrowing and supplies a mouse-event ring. It does not pick units, transform clicks into map orders or decode GUI gadget behavior.

The display context holds queue capacity/pointer/head/tail, a retained 24-byte event, a signed hide counter, a borrowed GAF frame, old cursor top-left coordinates, three owned scratch surfaces, thread-start status, mode, visible/overlay enable and a stop-request field. The [MouseEvent header](../../../src/gui/mouse_event.h) names the record's X/Y, keyFlags, scaledTick, message and double-click count.

Two visibility controls coexist: hide/show nesting count and the separate overlay-enabled field. Mode one selects the worker path and changes which count-based functions operate. A non-null sprite alone does not make every cursor draw call active.

## Initialization and allocation limits

InitMouse (`0x4c2bd0`) stores the caller's queue capacity and allocates capacity times 24 bytes tagged MOUSE EVENTS. It resets indices, clears the sprite and allocates SAVEMOUSE 1, 2 and 3 as surfaces of width 1600, height one. It sets hide count one, overlay enabled zero, mode zero and pending stop zero.

Those three allocations are scratch pixel capacity, not permanently 1600-pixel-wide cursor graphics. Composition later overwrites their width, height and pitch with current sprite dimensions without reallocating. A cursor requiring more than 1600 pixels therefore has no capacity guard here. The ordinary asset format and cursor selection must fit the buffers; changing descriptor dimensions does not enlarge their allocation.

If the start argument equals one, initialization launches MouseThreadProc using stack size `0x8000` and the display pointer. Successful launch sets mode one; failure leaves nonworker mode. Other argument values do not start a worker and set running zero. Allocations, negative/zero capacities and multiplication overflow are not independently validated before later operations. [Display initialization](graphics-display.md) normally requests a twenty-entry mouse ring.

Calling InitMouse over a live initialized context does not first free its previous ring or surfaces. The routine expects lifecycle setup, not arbitrary repeated replacement.

## Nonworker drawing and hide/show nesting

DrawSoftwareCursor (`0x4c2380`) draws the borrowed frame at the retained event X/Y when mode is not one and hide count is no greater than zero. It snapshots the retained event before drawing and does not explicitly test the overlay-enabled field or null sprite in this wrapper. [GAF drawing](graphics-gaf.md) supplies frame/hotspot behavior.

HideSoftwareCursor (`0x4c2470`) has effect only outside mode one. It increments the hide counter and, when the old count was zero, draws the saved background back at its retained top-left. Nested hides above zero do not repeatedly restore the background. Negative counts can prevent the zero-transition restoration, so balanced use matters.

ShowSoftwareCursor (`0x4c2870`) likewise acts only outside mode one. It decrements the count and, when the new value is no greater than zero, reads the Windows cursor position, stores X/Y in the retained event and, if a sprite exists, updates top-left from its hotspot and resizes the saved-background descriptor. It captures background through DrawSurface, then rechecks mode/count and draws the cursor.

An extra show can drive the count negative and perform another capture/draw; the count is not clamped at zero. These functions do not change the counter at all when mode one is selected. This is not a universal pair of boolean show/hide setters. GUI/menu/capture callers rely on their nesting and the separate worker/overlay policy.

GetCursorPos returns screen coordinates here; these bodies do not convert them to client coordinates. Windows mouse messages use client-style event positions in the display procedure, so the retained snapshot can be updated through different coordinate-producing paths depending on mode/window state. This module should not be described as performing a single global screen-to-map conversion.

## Background capture, restore and redraw composition

RestoreCursorBackground (`0x4c23e0`) first requires overlay enabled. It locks the primary surface, blits the saved background to the old top-left and unlocks with the affected rectangle. A lock failure leaves the screen unchanged. The rectangle uses the saved descriptor's current dimensions.

CaptureBackgroundAndDrawCursor (`0x4c24b0`) also requires overlay enabled and a successful primary lock. It obtains the Windows position, updates the retained snapshot, calculates top-left as position minus frame hotspot, resizes the saved descriptor and captures primary pixels into it. It draws the frame at the cursor position and unlocks with the new cursor rectangle. It dereferences the sprite without a separate null check.

RedrawMouseCursor (`0x4c25e0`) handles motion/composition while preserving the underlying screen:

1. Require overlay enabled and lock primary; otherwise return.
2. Read the Windows position, update snapshot X/Y and calculate the new top-left from sprite hotspot.
3. Resize the under/work descriptors to the sprite dimensions and capture pixels beneath the new location into under.
4. Overlay the previously saved old background into under at the old-to-new displacement, correcting any overlap that otherwise still contains the old cursor.
5. Copy under to work, reset work clipping and draw the cursor frame into work at its hotspot.
6. Copy overlapping composed pixels into the old saved region, prepare old/new damage rectangles, restore the old region on primary and place the new composed work image on primary.
7. Resize saved, replace its contents with the corrected under-background, retain the new top-left, and unlock/present both rectangles.

The three buffers have distinct roles: saved is the old location's cursor-free content, under is the corrected background at the new location, and work is that new background plus cursor. Copy ordering handles overlap between old/new positions. These are indexed-pixel software surfaces; the cursor is not a separate hardware sprite created by this module.

Underlying blits/locks determine clipping, pixel layout and presentation. [Surface services](graphics-surface.md) and [blit primitives](graphics-blit.md) record their own historical descriptor/calling-contract limitations. This reference follows cursor dataflow without assuming that every loose forward declaration is a modern type-safe graphics API.

## Shared MAIN/MOUS lock protocol

Mouse drawing and main-thread sprite changes coordinate through a held word, an owner tag and the auto-reset event initialized by [keyboard support](gui-keyboard.md). The cursor worker uses tag MOUS; main-side mutation uses MAIN. Acquisition exchanges the held word with its tag. A prior zero grants ownership and stores the owner tag; a matching owner tag is treated as a nested acquisition. Otherwise it waits indefinitely on the event and retries.

Release only clears owner/held and signals the event when the acquisition returned prior zero. A nested acquisition leaves the outer ownership intact. This is tag-based nesting, not a Windows mutex recording thread identity. Two unrelated callers using the same owner tag are not distinguished, so the design relies on the intended main/worker participation pattern. Waiting has no local timeout and event creation failure is not repaired here.

SetCursorSprite (`0x4c2b20`) takes the MAIN lock, changes the borrowed frame pointer and releases only its own outer acquisition. It does not allocate/free the sprite, resize scratch storage or preserve a reference count on its containing GAF. GetCursorSprite (`0x4c2ba0`) returns the shared pointer represented as an integer without taking that lock. Asset owners must retain the frame allocation through all drawing uses.

## Worker cadence, start and stop

MouseThreadProc (`0x4c2990`) receives the display pointer and sets its Windows thread priority to two. While pending is zero, it schedules a target GetTickCount time 33 milliseconds ahead, takes the MOUS lock, redraws when a sprite exists, releases and sleeps for the larger of one millisecond or remaining time. The source overview's word ticks refers here to GetTickCount milliseconds, not simulation ticks. Heavy work can exceed the target and still causes at least a one-millisecond sleep.

The worker does not inspect hide count. Redraw itself checks overlay enabled. When stop is observed, the worker clears pending and returns. That clear is the acknowledgement used by the main thread; no retained OS thread handle is joined in this module.

StartMouseThread (`0x4c2a80`) clears pending, invokes the same StartThread wrapper and sets mode one on success. The field called running stores a boolean success result, not a usable Windows thread handle. It does not reject an already running worker before launching another, so ordinary callers must prevent duplicate starts.

StopMouseThread (`0x4c2ac0`) returns one immediately when running is zero. Otherwise it sets pending one and waits in 100-millisecond sleeps, for at most twenty attempts, for the worker to clear it. Acknowledgement clears running and returns one; timeout returns zero, leaving status/request state rather than forcefully terminating the worker. Success does not reset mode to zero, so this alone does not enable nonworker hide/show behavior.

## Shutdown boundary

ShutdownMouse (`0x4c2cc0`) does work only when the event-buffer pointer is non-null. It attempts the worker stop handshake only when the display's bit-10 active field and running are both set. It waits up to twenty 100-millisecond intervals, then proceeds even without acknowledgement.

It frees work, under, saved and the event buffer and clears only the event-buffer pointer. It does not clear every surface/sprite/mode field. If the active condition skips the stop or the timeout expires while a worker still runs, freeing those surfaces can race subsequent worker use. The source has no mandatory successful join before release. Normal shutdown/lifecycle conditions must avoid that path; the reference does not claim a timeout safely kills the thread.

## Mouse-event ring and retained fallback

ClearMouseEventQueue (`0x4c2bb0`) resets head/tail without clearing storage or the retained event. PushMouseEvent (`0x4c2e30`) copies a full event at the producer head only if advancing modulo capacity would not equal tail. Full input is silently dropped. Like the keyboard ring, one slot distinguishes full/empty, so capacity twenty retains at most nineteen queued events. Capacity zero divides by zero and capacity one retains none; the caller must supply a usable positive value.

PopMouseEvent (`0x4c2d60`) copies/removes the oldest queued record and returns one, advancing/wrapping tail. If empty, it copies the retained current event and returns zero. PeekMouseEvent (`0x4c2de0`) copies the oldest without removal and returns one; if empty it returns the same retained fallback with zero. Empty therefore does not mean the output is cleared or lacks cursor coordinates.

The [display procedure](../../../src/graphics/display.cpp) stores movement messages directly as the retained snapshot, while button/down/up/double-click cases build queued records. Movement uses X/Y extracted from the packed message, key flags from wParam and a scaled timer stamp. The retained snapshot and event queue serve complementary purposes: position can keep changing without allocating a queued record for every motion.

[Application-frame arbitration](ingame-screenshots.md) peeks around GUI processing and sometimes pops, sometimes copies a snapshot. Unit picking, drag selection, build placement and orders occur later. These ring methods contain no ownership/target test and do not themselves transmit a command.

## Completion and runtime limits

All seventeen entries are covered, including every background compositor, thread transition, sprite accessor and queue operation. The module owns ring/scratch allocations and borrows the current sprite. It has no save record or network protocol schema. Exact cursor dimensions, display mode, message timing and lock/shutdown state determine runtime results. No worker, mouse interaction, build or game execution was performed for this reference.
