# Display environment, mode changes, messages and timers

[Systems manual](../index.md) | [Roadmap](../../doc-writeup-roadmap.md)

Module group: graphics/display, 30 functions in [display.cpp](../../../src/graphics/display.cpp), from `0x4b4f50` to before `0x4b6750`. This group owns the shared display environment, DirectX-version check, display-mode discovery, GDI/DirectDraw switching, main window procedure, environment cleanup, ten software timers and frame-rate counter. It supplies infrastructure used by battle rendering and interface code; it does not own the battle simulation scheduler.

## 1. Shared context and consumers

The context at g_display contains instance/window/class/title data; GDI bitmap, DC and palette handles; DirectDraw object, primary/back surfaces, clipper and palette; a saved/override surface; dimensions; activity/option flags; palette-transform pointers; a 256-entry palette; startup dimensions/mode; timer scale; frame counters; and an archive-item list. Partial type definitions in other modules expose only the fields their functions need.

Important stable field locations include window at 0x40, GDI bitmap/DC/palette at 0x44/0x48/0x4c, DirectDraw state at 0x84..0x9c, dimensions at 0xd4/0xd8, activity at 0xe0, tick scale at 0xe8, flags at 0xf0, frame counter at 0x1da and palette entries at 0x214. The context is packed to two-byte alignment around the later startup fields. It is not the same structure as a pixel [Surface](graphics-surface.md).

GetDisplay (`0x4b6220`) returns the global pointer through an integer-shaped interface. GetScreenWidth (`0x4b6700`) and GetScreenHeight (`0x4b6710`) return the current stored dimensions, without querying Windows or a surface. PointInRect (`0x4b6720`) accepts inclusive edges and returns zero outside any of the four bounds. It does not normalize inverted rectangles.

The display's pixel format is indexed eight-bit in the ordinary game paths. [Palette services](graphics-palette.md) own lookup-table generation/updates, [surface services](graphics-surface.md) own frame locking/presentation and cursor composition, and [blitters](graphics-blit.md) manipulate bytes. Display size affects those downstream allocations and clipping rules; SetResolution alone is not a complete reconstruction of every game viewport or map cache.

## 2. Defaults and option mapping

InitDisplayDefaults (`0x4b52e0`) preserves videoFlags' upper six bits, clears its lower ten bits, installs a 640 by 480 startup size, startup rectangle [0,0,639,479] and scale 1.0. It does not initialize the entire context, create a window or enter a display mode.

InitEnvironment later maps videoFlags bits 1..8 to context flag bits 2..9, retains the other relevant flag bits and sets context bit 0. The mapped fields are:

| Context flag bit | Meaning established by its use |
| ---: | --- |
| 0 | Environment option/initialized-state bit installed during setup |
| 1 | Current fullscreen selection, changed by SetFullScreen |
| 2 | Create/use the window and GDI/DirectDraw display infrastructure |
| 3, 4 | Retained options without an additional meaning established here |
| 5 | Allocate/free alpha table at context 0xc0 |
| 6 | Allocate/free shade table at 0xc4 |
| 7 | Allocate/free light table at 0xc8 |
| 8 | Allocate/free gray table at 0xcc |
| 9 | Allocate/free blue table at 0xd0 |
| 10 | Input-start option copied from videoFlags bit 9; passed to InitMouse |
| 11 | Quitting flag |

Startup videoFlags bit 0 separately selects fullscreen versus windowed mode. Context bit 10 is `mouseThread`: InitEnvironment passes it to InitMouse (`0x4c2bd0`) as its start flag, which starts the mouse thread only when it is set, and ShutdownMouse (`0x4c2cc0`) tests it before stopping that thread. It does not select audio.

## 3. DirectX compatibility check

CheckDirectXVersion (`0x4b5070`) first loads dsetup.dll, resolves DirectXSetupGetVersion and, if successful, splits the returned two DWORDs into four 16-bit version components. It frees the library after the attempt. If loading/resolution/query fails, it detects the Windows platform and queries HKLM's Software/Microsoft/DirectX key.

On the NT fallback it reads InstalledVersion and uses its low byte as the major-low component. On the other fallback it reads Version into a 30-byte buffer and tokenizes four dot-separated decimal components using strtok/atoi. It closes an opened registry key. A failed open/query returns zero. The text path assumes the expected token count and termination; it does not validate missing components before atoi.

NT fallback compares its installed value with the fifth requested argument. Other paths compare the four components in order, with a retained discrepancy in the first differing-major branch: major-high is compared with requested argument two rather than argument one. A successful DLL query leaves the local NT flag false because OS detection occurs only in the fallback. These details mean the result is not simply a clean generic lexicographic version comparison.

The frontend's concrete call in [frontend.cpp](../../../src/frontend/frontend.cpp), `0x4263b0`, requests components 4, 5, 0 and 0x9b, with NT threshold 3. This reference describes that historical startup gate rather than recommending a current platform configuration. No installation or compatibility test was performed.

## 4. Available mode list

GetDisplayModes (`0x4b5370`) uses the presence of a GDI bitmap to choose its discovery path. The GDI path resets the list count and adds 640x480, 800x600 and 1024x768 unconditionally. It adds 1280x1024 and 1600x1200 only if both corresponding desktop dimensions are large enough. Refresh values in this fixed list are zero. The first three sizes are not individually filtered to the desktop.

The DirectDraw path queries IDirectDraw2, enumerates its modes and releases that queried interface afterward. EnumModesCallback (`0x4b5330`) records only modes whose reported RGB bit count is eight, storing width, height and refresh rate and continuing enumeration. There is no sorting, duplicate removal or array-capacity argument. Callers must allocate sufficient mode storage and provide an initialized DirectDraw context. Query/enumeration failure returns zero; the fixed GDI path returns one.

## 5. Environment initialization sequence

InitEnvironment (`0x4b5980`) installs g_display before subsequent work. It reads GlobalMemoryStatus, storing total physical memory in the field provisionally named availPhys. It is not storing currently free physical memory. It initializes the frame counter, clears DirectDraw pointers and override state, clears quitting/window state and sets the input-start option.

Two SystemParametersInfoA calls use numeric actions 0x5e and 0x5d, saving a value at context 0xec and supplying zero on the second call. Shutdown later supplies that saved value to 0x5d. The current source comments describe a work-area rectangle and an overlap with flags, but Windows assigns these actions to mouse-trail get/set operations. The pointer passed on the get therefore receives an integer setting; the call is not SPI_GETWORKAREA. The misleading nearby field/comment names do not establish an actual work-area overwrite.

It initializes a 30-entry key queue and a 20-entry mouse queue, saves the starting directory and installs an initial current mouse event with X, Y and buttons zeroed. The other local event fields are not explicitly initialized here. It clears archive-item pointer/count, copies startup dimensions, maps the option flags and conditionally allocates the five palette transforms. Shade/light pointers are explicitly zeroed on their disabled branches; not every disabled table field receives a corresponding explicit assignment in this function. Allocation results are not combined into an environment failure result.

When context flag 2 is clear, initialization returns one without creating a window or display surfaces. Otherwise it zeros GDI handles/callback, copies the startup mode, builds a window class and registers it. The class uses WindowProc, style 8, a black background brush, the application icon, a raw cursor resource value 103 and the supplied menu identifier. It creates a visible popup/system-menu app window at the startup dimensions, shows and updates it, then calls SetFullScreen with videoFlags bit 0.

Successful mode creation returns one. Class registration, window creation or mode failure releases DirectDraw/GDI handles, displays the environment-initialization error, destroys the window and returns zero. This local failure path does not undo all earlier input, archive-directory or table setup. Complete teardown is a separate responsibility.

## 6. Exclusive mode change and synchronization

SetFullScreen (`0x4b5510`) acquires the shared graphics lock by InterlockedExchange with the MAIN tag 0x4d41494e. A free lock is marked owned. If the owner is already the same tag, it proceeds without arranging a second release. For a different owner it waits on the lock event and retries. This is tag-based reentry behavior, not proof of arbitrary thread-recursive safety.

Before attempting either mode, it clears haveBackBuffer, releases existing DirectDraw objects, deletes GDI DC/palette/bitmap handles and resizes the window to the stored dimensions. It therefore dismantles the old mode before knowing whether the replacement succeeds. It does not keep an automatic rollback copy of the old object graph.

For a nonzero mode it sets flag 1 and performs these dependent operations:

1. Create DirectDraw and set cooperative-level flags 0x53 on the game window.
2. Set the requested width/height at eight bits per pixel.
3. Create a primary flipping complex surface with one back buffer.
4. Acquire the attached back buffer and mark haveBackBuffer.
5. Create a clipper, associate the window and attach it to the primary.
6. Create a 256-entry palette from stored display entries and attach it to the primary when creation succeeded.
7. If activeSurface is nonnull, lock a temporary screen Surface, copy that saved image at 0,0 and unlock.

Most failed HRESULTs branch to the failure exit, but failed CreatePalette has no matching failure branch and can fall through to the common success path. The saved-image restoration does not check LockScreen's returned result before using its local Surface. These are relevant limits to the apparent all-or-nothing setup sequence.

The failure exit releases a newly acquired lock and returns zero. It does not locally release every partly created new DirectDraw object, clear the fullscreen flag, restore old dimensions or construct a fallback GDI mode. A later cleanup/mode call is responsible for any surviving objects. The ordinary success exit reloads palette colors, releases/signals the lock if this invocation acquired it, and returns one.

## 7. Windowed DIB mode and resolution requests

For zero mode, SetFullScreen clears fullscreen flag 1 and creates a compatible memory DC. It constructs an uncompressed eight-bit top-down DIB by supplying a negative bitmap height. The initial bitmap color table is zero; palette colors are applied at the common exit. Its embedded Surface uses the requested dimensions, returned pixel pointer and pitch rounded upward to a multiple of four. It selects the bitmap into the DC and makes the window non-topmost without moving/resizing it again.

These GDI allocation/selection calls are not individually checked before the function returns success. A return of one therefore does not prove every possible resource allocation succeeded. [Surface locking/presentation](graphics-surface.md) consumes the created descriptor and distinguishes the windowed path from DirectDraw.

ToggleFullScreen (`0x4b5910`) requests the opposite of the current fullscreen flag and discards SetFullScreen's result. SetResolution (`0x4b5940`) first writes new width/height and then rebuilds the current selected mode, also discarding the result. Neither verifies a listed supported mode, validates positive dimensions or rolls back the stored dimensions on failure. Game loading and state transitions make concrete 640x480 and configured-size requests in [game_load.cpp](../../../src/game/game_load.cpp) and [game_state.cpp](../../../src/game/game_state.cpp).

## 8. Palette restoration and window messages

ApplyPalette (`0x4b4f50`) selects/realizes an existing GDI palette through the window DC and returns one on that path without propagating individual GDI errors. Otherwise it reattaches an existing DirectDraw palette to the primary and returns whether that HRESULT was DD_OK. Missing primary/palette returns zero.

WindowProc (`0x4b5cc0`) translates Windows messages into display/input operations:

| Message/path | Behavior |
| --- | --- |
| Create | Return zero |
| Destroy | Switch out of fullscreen if selected, post quit and return zero |
| Activate | Set active to zero for inactive low-word state, otherwise one |
| Close | Invoke installed close handler with its argument; otherwise destroy window |
| Key down | HandleVirtualKey with ordinary-key flag zero |
| Character | PushKeyCode with the character value |
| System key down | HandleVirtualKey with flag one, then default window handling |
| System command 0xf100 | Suppress the key-menu command; other commands use default handling |
| Mouse move | Update the current event without queueing a button event |
| Left/right button down/up | Queue an event with flag zero |
| Left/right double click | Queue an event with flag one |
| 0x219, 0x3b9 | Forward to optional three-argument display callback and return one |
| 0x30f | Realize GDI palette or reattach DirectDraw palette and report success |
| 0x311 | Ignore notification originating from this window; otherwise reapply palette |
| Other | DefWindowProcA |

Mouse X/Y are extracted as unsigned 16-bit halves of lParam, not sign-extended coordinates. The event also carries button state, message, flag and a timestamp computed from GetTickCount scaled to the display tick rate. Thus negative coordinates encoded in a halfword become large positive integers here. The timestamp is a UI clock, not a replicated simulation tick.

SetCloseHandler (`0x4b4fd0`) stores a global callback and argument. Handling WM_CLOSE through that callback does not automatically destroy the window, so callers can implement their own exit decision. InitEnvironment's separate three-argument display callback is cleared on its window path and serves the numeric notification messages, not close-handler dispatch.

## 9. Timer storage and time arithmetic

InitTimers (`0x4b62d0`) stores the requested tick rate, clears g_timerCount, marks ten slots free with interval -1 and records the scaled current time. [main.cpp](../../../src/game/main.cpp) supplies 30 ticks per second. GetTickRate (`0x4b6330`) returns the scale; GetTicks (`0x4b6340`) returns GetTickCount*scale/1000; GetMilliseconds (`0x4b6560`) returns the unscaled GetTickCount. ResetTimers (`0x4b6510`) disables all slots and restarts the elapsed-time origin without changing the scale.

Each 16-byte slot contains callback, callback argument/ID, interval and countdown. The handle returned by AddTimer is the slot index, not the supplied ID. The source/global layout in [symbols.csv](../../../data/symbols.csv) places ten slots at 0x51fbd8, the first interval at 0x51fbe0, close-handler fields after the slots and g_timerCount at 0x51fc80.

The scaled multiplication uses the existing 32-bit arithmetic before division. It is not a widened monotonic 64-bit clock, so multiplication wrap can occur before GetTickCount's own wrap. Elapsed differences are signed values derived from scaled unsigned samples. A large gap, wrap or changed scale is not normalized into a guaranteed positive duration. Initialization/reset establishes a fresh origin but does not change this arithmetic.

These are cooperative timers polled by code, not Windows scheduled tasks, worker threads or one thread per slot. [screenshots.cpp](../../../src/ingame/screenshots.cpp) calls UpdateTimers as part of frame services. CD playback/fades and sample streaming use AddTimer/RemoveTimer in [cd_audio.cpp](../../../src/sound/cd_audio.cpp) and [samples.cpp](../../../src/sound/samples.cpp). No module-doc follow-up scheduler is created by these game timers.

## 10. Timer firing, mutation and retained count limits

UpdateTimers (`0x4b6370`) samples the scaled clock, subtracts the saved origin and then samples again to replace that origin. It walks all ten physical slots using an address boundary, not g_timerCount's numeric value. A nonnegative interval means enabled. The elapsed difference is subtracted from countdown; a result at most zero calls the stored callback with its ID and then reloads countdown from the slot's current interval.

At most one invocation occurs per visited slot per poll, even if many periods elapsed. Overshoot is discarded by reloading a whole interval; it is not carried forward or caught up. A zero interval can fire once on each update. Callbacks run synchronously in slot order. A callback can disable/change slots, and the reload observes its current interval. There is no copied callback list, lock or general recursion guard, so mutations of later slots can affect the same scan.

AddTimer (`0x4b63f0`) first performs a similar elapsed update of existing slots, potentially invoking callbacks before installing the requested timer. It then finds the first negative interval among ten physical slots, writes callback/ID/interval/countdown, increments g_timerCount and returns the index. A full table returns -1. It does not reject a null callback or negative requested interval; a negative interval leaves the installed slot immediately reusable despite incrementing the count.

RemoveTimer (`0x4b64d0`) rejects a negative index or one at least g_timerCount, otherwise sets that slot's interval to -1 and returns one. It does not decrement g_timerCount. Reusing slots therefore increases that count beyond the number of active slots and eventually beyond ten. Since RemoveTimer tests that growing count rather than the fixed physical capacity, an invalid caller-supplied index can pass the check after enough reuse and address outside the table. Normal clients retain actual returned indices, but the function itself does not establish safe bounds for arbitrary integers. Reset/initialization is what resets the counter.

## 11. Frame-rate accounting

DrawFrameRate (`0x4b6570`) and GetFrameRate (`0x4b66a0`) both advance the shared counter. They add elapsed milliseconds, replace lastTick and increment the frame count. If accumulated time exceeds 2000, it is clamped to 1000. A separate strict greater-than-1000 test publishes the current frame count, subtracts 1000 and resets frames. Therefore a long gap that triggers the clamp does not publish in that same invocation, and this value is a count over the implementation's sampling window rather than a precisely normalized frames/elapsed ratio.

GetFrameRate is not a passive getter: calling it more often advances the frame count more often. Calling both interfaces for one presented frame can count twice. The counter is presentation bookkeeping, not a gameplay speed controller.

DrawFrameRate additionally obtains the back surface's DC when there is no GDI bitmap. If successful it sets transparent background mode, selects the display font, draws a yellow FRATE label at 0,0, restores the old font and releases the DC. It does not draw this overlay in the GDI bitmap path. It assumes an available back-surface pointer before calling GetDC; failure of GetDC merely skips text. Both functions return the stored rate.

## 12. Release, quit and fatal paths

ReleaseDirectDraw (`0x4b4ff0`) only enters its release block when the DirectDraw object exists. It releases and nulls palette, back surface, primary surface and clipper when present, then releases/nulls DirectDraw. It neither frees activeSurface nor performs an explicit RestoreDisplayMode call here. Inconsistent subordinate pointers with a null DirectDraw pointer are not independently swept.

ShutdownEnvironment (`0x4b6110`) runs pre-cleanup and mouse shutdown, closes each stored HAPI archive, frees the item array, frees option-enabled palette tables, releases DirectDraw, deletes GDI handles and zeros those GDI fields. It restores the saved integer system parameter through action 0x5d. It does not clear every archive pointer/count or make arbitrary repeated shutdown calls safe. Its responsibility is environment teardown after higher-level resources have been handled.

QuitApp (`0x4b6230`) marks quitting, requests windowed mode if fullscreen, optionally shows a message and posts WM_DESTROY. It does not synchronously execute every shutdown step. WindowProc handles that destroy notification by posting quit. FatalError (`0x4b6290`) optionally displays a system-modal error and immediately calls exit(1), without using this graceful sequence. EmptyPostArchiveMountHook (`0x4b62c0`) returns zero and has no additional side effects.

## 13. Coverage and interpretation

All 30 address-assigned bodies and the relevant timer/global-layout, startup, palette, input and surface consumers were inspected. This reference covers both success and retained partial-failure behavior. It distinguishes Windows messages, cooperative game timers and simulation execution, and corrects source-comment interpretations only where actual argument/field use supports doing so.

There is no display-state save record or network protocol in this group. Persistent screen settings and match restoration are supplied by other modules. Runtime allocation success, driver behavior and actual historical machine compatibility were not tested. Documentation uses static source tracing without builds, installations or copied source listings.
