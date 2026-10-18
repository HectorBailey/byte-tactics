# 02. Time and simulation execution

[Manual index](index.md) | [Previous: startup and assets](01-startup-assets-lifecycle.md) | [Next: economy](03-economy.md)

## 1. Four distinct kinds of progress

An application frame processes Windows/game input and calls the handler for the current application state. A battle-loop iteration budgets simulation and draws the battle view. A simulation tick advances game state. A subsystem's own deadline or counter determines when its periodic work occurs.

These are related but not interchangeable. Rendering can occur without a simulation tick; a battle iteration can run several ticks; a subsystem can be invoked each tick but perform expensive work only on a deadline. The engine also uses a loading thread during match setup, which is outside this ordinary battle-tick schedule.

Primary anchors: [application frame](../../src/ingame/screenshots.cpp), `0x499890`; [battle loop](../../src/game/main_loop.cpp), `0x496790`; [pacing and simulation](../../src/ingame/game_dialogs.cpp), `0x495230`, `0x495490`.

## 2. Application-frame dispatch

`MainFrameTick` first handles selected global keys, including fullscreen, screen dismissal, and screenshots. Screenshot capture resets the last simulation-budget timestamp, preventing the spent capture time from being treated as an ordinary uninterrupted budget interval on the next measurement.

It samples mouse events around the menu update, services CD activity classification and queued speech, chooses a mouse-event record using the before/after state, updates timers, and calls the current game callback unless the display context blocks that dispatch.

Consequently, input and speech activity is not proof that the battle simulation advanced. The same application frame can dispatch menu, loading, or battle callbacks depending on application state.

Evidence: [frame dispatcher](../../src/ingame/screenshots.cpp), `0x499890`; [state callback mapping](../../src/game/game_state.cpp), `0x490b30`.

## 3. Game speed and clock units

The public speed setter clamps its requested step to 1 through 20, uses 10 as the normal display point, assigns requested/effective speed state, and can broadcast a speed-control packet. The frame pacer uses the effective step multiplied by 0.1 as its initial rate factor.

The pacer's elapsed quantity comes from `GetTicks`, not directly from a call whose contract is established here as milliseconds. Profile charging separately uses a milliseconds function. Timer ticks, profile milliseconds, simulation ticks, weapon seconds, and rendered frames must not be silently combined into one unit.

Weapon loading and several gameplay calculations establish a convention of 30 simulation ticks per gameplay second. Converting that to wall-clock speed requires the timer contract and effective pacing conditions. This chapter records the actual multiplier and gates without claiming every machine always simulates exactly 30 ticks per wall second.

Evidence: [speed controls](../../src/game/game_state.cpp), `0x490df0`, `0x490ee0`, `0x490f10`; [pacing](../../src/ingame/game_dialogs.cpp), `0x495230`; [weapon time conversions](../../src/weapons/weapon_types.cpp).

## 4. Budgeting simulation steps

### 4.1 Fractional carry

The pacer measures the elapsed timer ticks since its prior call, updates the measurement timestamp, and forms a budget from elapsed ticks times its rate plus a carried fractional remainder. It takes the floor as the initial whole-step count and stores the residual fraction for later.

Small intervals are therefore not simply rounded independently and lost. Several substep intervals can accumulate into a whole step. A user-visible delay can originate in budgeting, a later authority gate, or subsystem scheduling; they should be separated during investigation.

### 4.2 Remote-player lag

The pacer scans active type-3 player records that still have units and finds a synchronization tick below the current local tick. It records the slowest qualifying record and the difference from the local game tick.

At lag 900 or more it reduces the effective rate. The lag used by the scale is capped at 3600; the scale is proportional to 3600 minus that lag and has a minimum of 0.01. Below the threshold it clears its lagging indicator. These values are tick-state comparisons, not ping measurements in milliseconds.

The scan's controller-type and unit-count conditions matter. It does not simply choose the slowest number in all ten player slots regardless of their role or participation.

### 4.3 Step cap and adaptive speed

After calculating whole steps, negative counts are clamped to zero. Pause forces the count to zero and returns from the pacer. Otherwise, a budget over five is capped to five.

Repeated capped budgets increment a hysteresis counter. Above ten, that counter resets and the effective speed decreases by one if it is above one. Noncapped budgets decrease the counter; below minus one hundred, it resets and the effective speed increases by one toward the configured control speed.

The stored fractional remainder comes from the precap floor operation. Whole steps discarded by the five-step cap are not explicitly added back into that fractional value in this routine. This is an adaptive budget policy, not an unlimited catch-up loop promising to replay every elapsed interval immediately.

Evidence for all budgeting details: [UpdateFramePacing](../../src/ingame/game_dialogs.cpp), `0x495230`.

## 5. Battle-loop gates and pause

The battle loop snapshots and resets nine profiling accumulators, establishes a new profiling timestamp, and chooses its simulation path from network/application flags and pause state.

In the network-enabled branch it budgets steps and invokes the network form of `RunGameSteps` when work is pending. If no steps are pending and the game is paused, it can still flush queued packets, receive packets, and send periodic probes. Pause does not mean the connection stops being serviced.

The other branch budgets/runs local steps only when its interface gate and pause conditions permit. Later handling processes game keys and edge scrolling under the interface gate, collects visible unit IDs, resolves selection refresh state, and draws the battle frame. Movie capture is then checked against game tick and its own output schedule.

Therefore, pause has several observable layers: simulation steps can stop while the game continues handling the interface, receiving traffic, drawing, and servicing the application loop. Precisely which paths remain active depends on the branch conditions, not a universal “everything freezes” rule.

Evidence: [MainLoopTick](../../src/game/main_loop.cpp), `0x496790`.

## 6. Exact major tick order

`RunGameSteps` copies the pending step count and repeats the following for each step:

| Order | Phase | Main effect |
| --- | --- | --- |
| 1 | Increment game tick | Establishes the new simulation tick |
| 2 | Receive packets, network form only | Applies received events before this tick's unit phase |
| 3 | Update all units | Weapons, scripts, orders, movement and other per-unit state under their gates |
| 4 | Update projectiles | Advances existing/new projectile records through their motion and collision paths |
| 5 | Update explosions | Advances debris and exploded-piece state |
| 6 | Update players | Player status, path search service, economy and local sensor work under conditions |
| 7 | Update features | Feature motion, animation, fire and other feature work |
| 8 | Step global GAF sequences | Advances registered animation sequences |
| 9 | Update wind | Periodic environment change and notification state |
| 10 | Update meteors | Environment-event scheduling |
| 11 | Update camera follow | Simulation-follow tracking |
| 12 | Update particles | Particle movement, expiry and emission |
| 13 | Update blink state | Transient display-state progression |
| 14 | Share resources and flush queued packets, qualifying network form | Local sharing and outgoing delivery |

After all pending steps, the routine invokes its post-step hooks and expires selected message/marker state. Those after-loop actions are not individually repeated for each tick by this loop.

This table records the direct call sequence. A called subsystem can invoke other systems internally, create events, or defer work; the table should not be interpreted as proof of one isolated execution per object.

Evidence: [RunGameSteps](../../src/ingame/game_dialogs.cpp), `0x495490`.

## 7. Order-dependent consequences

A projectile created during the unit phase can be available to the subsequent projectile phase in the same simulation step, subject to allocation/update-loop conditions. A particle request made before the particle phase can similarly enter that phase without waiting for a rendered frame. Those are consequences of call ordering, not guarantees about when every individual visual is first drawn.

Economy runs after unit actions in this major schedule. Construction requests can therefore consult an account's prior shortage state and contribute new demand for the later settlement pass. This relationship is essential to understanding construction stalls; the request helper is not a fresh global allocation calculation at every call.

Sensors are updated from the player phase, after the unit and projectile phases. Targeting can use maintained information rather than a globally rebuilt current sensor state immediately before every shot. The detailed authority and list-maintenance rules belong to the sight/combat chapters.

Evidence: [tick order](../../src/ingame/game_dialogs.cpp), `0x495490`; [requests and settlement](../../src/game/economy.cpp), [player update](../../src/game/players_464290.cpp), `0x464f80`.

## 8. Per-unit execution

### 8.1 Enumeration and controller gates

`UpdateAllUnits` enumerates active player slots with accepted controller types and walks each player's allocated unit interval. A nonzero unit-definition index identifies an occupied unit slot for this loop. It counts active processed units and performs common work before additional local/controller checks.

Several operations are restricted to controller types 1 or 2, while type-3 records take a different role. This chapter reports the tested values rather than deriving a complete authority model from one loop alone.

### 8.2 Ordering inside a unit update

For an occupied unit, the loop updates wind-generator script notifications, updates weapons for the qualifying owner types, runs existing scripts, decrements damage/transfer timers, updates selected eligibility flags, and periodically refreshes cached health percentage.

For qualifying locally processed units it then applies configured water hazards on the 30-tick cadence, attempts periodic self-repair under its conditions, runs primary orders, runs secondary orders, updates motion when present, and updates height. A marked death/removal condition is handled after those branches.

Thus weapon update precedes the script execution and primary-order work shown later in the same per-unit pass. An aim script launched by weapon update can participate in that script phase, while a target/order change made later need not retroactively alter work already performed earlier. Detailed callback effects still require tracing their consumers.

After a player's unit interval, the loop sends unit states for the qualifying network/owner conditions. It also handles a later camera auto-follow selection cycle outside the core unit walk.

Evidence: [UpdateAllUnits](../../src/units/unit_position.cpp), `0x48ad30`.

### 8.3 Motion subphases

The motion update first services its behavior object, chooses aircraft versus ground steering from the definition flight bit, integrates/validates position, updates movement-rate script callbacks, and updates the script's SFX occupancy classification. The caller then performs height update.

Movement is therefore more than adding velocity. A candidate position can be rejected or clamped, spatial occupancy can change, sight can be refreshed, and a script can be notified of a speed-band or environment-state transition.

Evidence: [motion update](../../src/orders/order_list.cpp), `0x43dd20`; detailed naval cases in the [naval reference](../naval-systems.md).

## 9. Per-player execution

The player update services pathfinder searches before enumerating player records. Within the player loop it handles participation and endgame/watch transitions, invokes economy only for qualifying controller/status conditions, and performs local observer sensor/minimap updates when the loop reaches the current view player's record.

The economy gate requires an eligible active record, an accepted team state, a qualifying unit-count/creation state, controller type 1 or 2, and the shown endgame conditions. It is incorrect to say every slot settles its economy on every tick regardless of elimination or authority.

Multiplayer economy reporting uses another counter and sends every fourth qualifying local pass in this path. That reporting frequency is distinct from settlement frequency and HUD refresh frequency.

Evidence: [UpdatePlayers](../../src/game/players_464290.cpp), `0x464f80`.

## 10. Subsystem deadlines and periodic work

The game mixes exact modulo tests, decrementing counters, and absolute tick deadlines. Examples confirmed in inspected paths include health-percentage refresh every 30 ticks, environmental water damage every 30 ticks, self-repair on the eight-tick mask condition, particle expiry deadlines, and wind changes scheduled several gameplay seconds apart.

Wind update sets a notification/enabled value when it selects new wind and clears that value on intervening calls. It does not zero the stored wind vector or normalized strength in that intervening branch. Clearing the notification is therefore not equivalent to physically calm wind.

Construction, landing, aiming, and network retries can use their own deadlines. An order continuing to exist in the queue is not evidence that it performs its expensive work every tick. The order chapter will define those scheduling and return-status contracts in full.

Evidence: [unit periodic work](../../src/units/unit_position.cpp), `0x48ad30`; [wind](../../src/game/game_state.cpp), `0x490c40`; [particles](../../src/graphics/particles.cpp); [orders](../../src/orders/unit_orders.cpp).

## 11. Presentation and sampled state

The battle renderer consumes world and observer state after the battle loop's simulation work. Its resource panel smooths displayed amounts and refreshes rate figures on its own tick deadline. It redraws the panel when the cached display record changes. Reading a bar's current position is therefore not the same operation as reading the player's account balance at an earlier simulation phase.

Visual animation can also use game tick directly. Construction palette cycling is one example: its colors can move while remaining construction is unchanged. Cosmetic state must be interpreted according to the producer and clock it actually uses.

The renderer draws terrain, interface resources/radar, sorted world objects, and later effects/overlays through separate paths. Its full draw order belongs to the graphics chapter. The timing conclusion established here is that drawing does not imply one fresh simulation tick per picture.

Evidence: [battle renderer](../../src/ingame/info_panel_468cf0.cpp), `0x468cf0`; [construction rendering](../../src/graphics/model_render.cpp), `0x458dd0`.

## 12. Profiling and diagnostic interpretation

Profiling accumulators are charged between subsystem calls using elapsed milliseconds. The battle loop copies prior bucket totals into display values and clears the new accumulation interval. A minimum total avoids a zero denominator in later presentation.

Bucket names and boundaries are instrumentation, not ownership guarantees. Miscellaneous work shares a bucket, and a called subsystem may include internal work belonging conceptually elsewhere. A large bucket identifies an investigation region rather than proving one particular function dominates it.

| Observed behavior | Distinguish first |
| --- | --- |
| Smooth cursor with stationary units | Application/UI frames versus simulation pause or zero budget |
| Several movements between rendered images | Multiple simulation steps in one battle iteration |
| Speed drops under load | Five-step cap and effective-speed hysteresis |
| Slow network game despite low displayed ping | Sync-tick lag policy versus latency measurement |
| Builder pulses without progressing | Tick-based palette motion versus accepted resource work |
| Resource bars lag a direct balance read | Display smoothing and rate sampling |
| Wind script updates intermittently | New-wind notification versus persistent wind vector |
| No economy update after defeat | Controller, unit-count, endgame, and watch-state gates |

Open details for later chapters include the complete timer implementation, every packet/authority branch, all order scheduler contracts, and full rendering order. The direct phase ordering above is established independently of those remaining details.
