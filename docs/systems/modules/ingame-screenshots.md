# Application frame input, screenshots and callback dispatch

## Scope and frame boundary

The `ingame/screenshots` group owns one entry, MainFrameTick at `0x499890`, before the next group begins at `0x499a30`. The entire body is in [screenshots.cpp](../../../src/ingame/screenshots.cpp). Despite the module name, its central responsibility is one application-frame dispatch: selected global keys, GUI mouse sampling, CD/speech service, timers and the retained state callback.

It does not encode a screenshot itself. That operation is [SaveScreenshot in PCX support](../../../src/graphics/pcx.cpp), already covered in [the PCX and capture reference](graphics-pcx.md). The application [message pump](game-main.md) calls this frame while it can perform idle work; the installed callback may represent a menu, loading screen, battle or endgame. Calling MainFrameTick is therefore not proof of a simulation tick.

## Global key cases

MainFrameTick peeks one key once and uses that retained integer for three separate tests. It does not repeatedly fetch another key after consuming the first. The cases are:

| Key code | Gate and effect |
| --- | --- |
| `0x7e` | Only with UI option bit 1 at `+0x37f2f` set: consume the key and call ToggleFullScreen. Otherwise leave it queued here. |
| `0xe3` | Only with orders-panel flag value 1 set: consume it, request PopUntilNamedLayout with force zero and clear that flag. For mission type other than 3 also clear bit 0 of the separate flag word at `+0x38a51`. |
| `0xd6` | Consume the key, create screenshot directories, call SaveScreenshot and reset lastSimBudgetTick to GetTicks. |

The screen-dismissal case discards PopUntilNamedLayout's result, so clearing its flag does not establish that the named HUD was actually reached. That helper can defer because of other interface flags. The `+0x38a51` low bit is the pause state used by other simulation code; dismissing this local/non-type-3 screen clears it, while the multiplayer type does not take that clear.

For screenshots, the frame first attempts to create the configured base directory, formats a path beneath it using the `%s\screenshots` format, attempts to create that path and invokes capture with prefix `SHOT`. The literal format/prefix are established in [globals.csv](../../../data/globals.csv). The local path buffer is 256 bytes with an unbounded formatter. Directory creation status and capture success are ignored, so consuming the key does not prove an image was written.

Capture resets the simulation-budget timestamp even if the capture operation fails. Its purpose in the frame sequence is to keep time spent on capture from automatically appearing as uninterrupted simulation budget later. [Movie capture](game-main-loop.md) uses the same lower-level capture service from a different schedule and prefix; this global-key path does not enable movie mode.

## GUI and mouse-event arbitration

After the key tests, the frame peeks a six-integer mouse record into a first local snapshot, updates the GUI/menu, and peeks another snapshot. GUI processing can change which queued event is exposed. It then updates activity-driven CD category selection and calls PlayNextSpeech before choosing the game-facing mouse record.

The comparison uses field index 4 of the two snapshots, not every field. If those values are equal, it pops a mouse event into the retained game input record. If they differ and the first field-4 value is `0x205` or `0x202`, it retains the entire first snapshot. Otherwise it retains the entire second snapshot.

The two differing-event branches copy a snapshot without an explicit PopMouseEvent in this body. Equal field-4 values do not imply equal coordinates or timestamps, and the equal branch obtains the popped record rather than simply copying the first. These details matter when tracing a click shared by GUI processing and world input. Mouse queue ownership, event production and the meaning of all six fields belong to the separate mouse module.

CD classification and queued speech execute before the eventual state callback gate. Their activity therefore does not prove that battle simulation or a state callback ran in the same application pass. The audio policies are explained in [game sound](sound-sound.md) and [CD audio](sound-cd-audio.md).

## Timers and state callback gate

The function calls UpdateTimers after mouse arbitration. It then reads the display context through the game's display pointer and tests flag value 8 at context offset `+0xf1`. If that flag is clear, it invokes the stored callback directly. It performs no null-callback check in this body.

The gate suppresses only that final callback here: global key actions, menu update, mouse arbitration, audio service and timer update have already occurred. A blocked callback thus does not mean an entirely inactive application frame. Conversely, an invalid/null callback installed by an unsupported game mode is not repaired by this dispatcher.

[Mode selection](game-game-state.md) maps states to handlers. [Loading](game-game-load.md) uses the callback to service synchronization while its worker builds state, and [battle passes](game-main-loop.md) choose whether to simulate before drawing. Timer dispatch semantics and mutation during timers are described in [display services](graphics-display.md).

## Ownership and completion

This single entry owns no persistent image buffer, packet schema or save record. Its local mouse snapshots are copied into existing game state; captured-file naming/encoding/allocation are delegated. It does not authorize construction, target selection or unit orders, which are later callback/input responsibilities.

The complete owned body, constants and direct capture/state/timer/audio dependencies have been traced. Actual key mapping, current GUI layers, filesystem access, display flags and callback state determine runtime results. No screenshot was taken, directory was created by game code or application was run during documentation.
