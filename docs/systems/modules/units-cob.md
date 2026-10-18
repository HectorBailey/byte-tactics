# COB interpreter, cooperative channels and piece animation

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `units/cob`, `0x4b0610` through the boundary before `0x4b2290`, implemented in [cob.cpp](../../../src/units/cob.cpp) with shared [CobScript/Channel declarations](../../../src/units/cob_script.h). It executes compiled unit animation scripts through eight cooperative channels, integrates per-piece movement/rotation, exposes virtual game callbacks and saves/restores execution state. Loading the shared COB asset belongs to data-file services; interpreting its runtime program belongs here.

## 1. Shared program versus per-unit execution

ScriptTable holds script-entry count, piece count, static-variable count, entry offsets, names and integer code words. Each unit's CobScript binds that shared table but has separate static variables, channel stacks and piece-motion state. Per-unit statics are shared between that unit's channels, not one global variable bank for every unit of the same definition.

The 0x540-byte interpreter contains tick-rate scale, table/checksum, statics/pieces, animation-changed state, eight 0xa4-byte channels and active-channel count. A channel holds encoded execution/wait state, program counter, stack pointer, delay, waited piece/axis/child, signal mask, callback and 32 integer stack slots.

`CobScript::CobScript`, `0x4b0610`, initializes null table/storage, clears channel states/count and captures GetTickRate as scale. Destructor/deleting-destructor entries `0x4b06f0`/`0x4b06b0` release per-unit piece/static storage. They do not free the shared program table.

`SetCob`, `0x4b0720`, stores the table, obtains its checksum, resizes piece-motion records to pieceCount times 0x4c and statics to staticCount times four, and zeros the piece records. It does not explicitly clear all resized static-variable bytes or reset every existing channel in this body. Binding/rebinding must follow the intended initialization lifecycle. GetCob (`0x4b07a0`) returns the pointer; the unresolved helper at `0x4b07b0` returns zero.

## 2. Names, channel allocation and capacity

`FindScript`, `0x4b07c0`, compares entry names with strcmp, making lookup case-sensitive, and returns minus one for a miss. It expects a bound table. `StartThreadByName`, `0x4b0830`, adds a null-table guard before lookup/start.

`StartThread`, `0x4b08c0`, rejects an invalid script index and finds the first channel with state zero. It sets running state 0x01000000, entry PC, stack pointer minus one, null callback and signal mask 1, then increments activeCount. Invalid index or all eight occupied returns minus one. There is no dynamically growing ninth execution channel.

Channels are cooperative execution contexts on the game update thread, not operating-system threads. Starting several animation routines does not introduce parallel access to model state. Their numeric channel order and explicit waits determine when they run.

## 3. Starting scripts and immediate update

StartScript (`0x4b0940`) and StartScriptByIndex (`0x4b0a10`) allocate a channel, attach an optional completion callback and return 1 on success or zero on failure. An update argument requests a zero-elapsed RunThread pass over all eight channels, not only the new one, followed by AnimatePieces(0).

The argument forms StartScriptWithArgs (`0x4b0a70`) and StartScriptWithArgsByIndex (`0x4b0b00`) store four supplied integers, then set the channel stack pointer to count minus one. A missing script/full-channel failure in the indexed argument form also calls the supplied callback with zero. The simpler start methods do not provide that same failure callback branch.

Callers are expected to supply a count consistent with the four available arguments and stack layout. The body does not independently cap arbitrary counts. Zero-elapsed execution can perform state changes, spawn effects or reach a wait immediately, but AnimatePieces returns without advancing gradual motion at elapsed zero.

## 4. Query calls and callback lifetime

QueryScript (`0x4b0bc0`) resolves a name and calls QueryScriptByIndex (`0x4b0c40`). The latter allocates a channel, places four pointed-to input values or zeros in its initial stack slots, runs that channel with elapsed zero, then copies the four slots back to nonnull caller pointers. It returns 1 because allocation/execution was initiated, not a separate guarantee that the queried bytecode reached return.

A query that sleeps or waits can remain active after this call. Query-style assets are expected to supply their result through the intended fast convention. Treating the function as an unrestricted synchronous wait-until-finished routine would be incorrect.

RemoveCallback (`0x4b0d20`) clears matching callback pointers on active channels without terminating them. Unit/order/weapon owners use callback lifetime separately from thread lifetime. The [weapon aim callback](units-unit-script-calls.md) interprets a nonzero completion value as aim-ready; it still depends on firing-side callback reset and other firing gates.

## 5. Scheduling and waits

RunScripts (`0x4b0d60`) runs channels 0..7 with the supplied elapsed amount when any are active, then animates pieces. RunThread (`0x4b0da0`) ignores inactive channels and rechecks waiting channels before interpreting instructions.

| State | Wake condition |
| --- | --- |
| Running, 0x01000000 | Interpret until a yield/termination |
| Wait-turn, 0x02100000 | Selected piece-axis turn speed becomes zero |
| Wait-move, 0x02200000 | Selected piece-axis movement speed becomes zero |
| Sleep, 0x02400000 | Delay minus elapsed reaches zero or below |
| Wait-child, 0x02800000 | Child termination invokes Wake for its channel index |

Wait-child is resumed by an explicit Wake operation, not by the same timer/piece tests. Wake searches all channels waiting on that child index and marks them running.

Piece animation occurs after channel execution in RunScripts. A turn that finishes during AnimatePieces generally releases a wait-turn on a subsequent channel pass. There is no instruction-count watchdog inside the running loop. A malformed nonyielding jump loop can occupy the game thread; finite cooperative yielding is an asset-program contract.

## 6. Piece and presentation instruction families

The interpreter masks code words to identify opcode families, with low bits selecting some operand forms. These are decoded operations rather than source listings:

| Family | Effect |
| --- | --- |
| 0x10001000 | Set translation target and signed speed on immediate piece/axis |
| 0x10002000 | Set bounded turn target/speed, selecting circular direction |
| 0x10003000 | Start spin with target angular speed and acceleration |
| 0x10004000 | Stop spin using a deceleration value |
| 0x10005000 / 0x10006000 | Show/hide piece |
| 0x10007000 / 0x10008000 | Cache/uncache piece |
| 0x10009000 | Invoke legacy explosion callback |
| 0x1000a000 | Invoke legacy sound callback |
| 0x1000b000 / 0x1000c000 | Immediate translation/rotation |
| 0x1000d000 / 0x1000e000 | Enable/disable piece shading |
| 0x1000f000 | Emit scripted SFX at an immediate piece |
| 0x10011000 / 0x10012000 | Wait for piece turn/move |
| 0x10013000 | Sleep for popped milliseconds converted to ticks |

Gradual translation/turn/spin rates divide supplied speeds by captured tick-rate scale. Turn targets are masked to 16 bits, while spin uses target minus one as its unlimited-rotation sentinel. Sleep uses scale times milliseconds divided by 1000, with integer truncation.

The [UnitScript overrides](units-unit-script-calls.md) implement actual model mutation, visibility, wakes/smoke, transport and unit-state operations. The base class has pure piece setters/getters for its required geometry interface, while optional callbacks can be inert. A bytecode instruction's existence does not make every base callback produce a game effect.

## 7. Stack, arithmetic and comparisons

| Family | Effect |
| --- | --- |
| 0x10021000 | Push immediate, local stack slot or static variable according to low selector 1/2/4 |
| 0x10022000 | Reserve/increment stack position |
| 0x10023000 | Pop into local/static slot for selectors 2/4 |
| 0x10024000 | Discard/decrement stack position |
| 0x10031000..0x10034000 | Integer add, subtract, multiply, divide |
| 0x10035000..0x10038000 | Bitwise AND, OR, XOR and complement |
| 0x10041000 | Inclusive integer random range from two popped bounds |
| 0x10051000..0x10056000 | Less, less/equal, greater, greater/equal, equal and unequal |
| 0x10057000 / 0x10058000 | Boolean AND/OR |
| 0x10059000 | Bitwise XOR through the stack helper |
| 0x1005a000 | Boolean NOT |

For noncommutative operations the first popped value is the right operand; the earlier stacked value is left. Boolean results are normalized by their C++ operations, whereas bitwise XOR preserves bit patterns. Random uses upper minus lower plus one and assumes an appropriate valid range.

Push/Pop are direct accesses to the fixed 32-word stack. This module does not independently guard stack overflow/underflow, every local/static index, divide-by-zero or all integer overflow. It interprets trusted compiled programs, not a sandbox for arbitrary downloaded native behavior.

## 8. Thread control, signals and return

| Family | Effect |
| --- | --- |
| 0x10061000 | Allocate another channel and copy argument values, continue caller |
| 0x10062000 | Allocate another channel and copy arguments, then wait for it |
| 0x10063000 | Pop an immediate count into a four-value temporary argument array |
| 0x10064000 | Jump to immediate code offset |
| 0x10065000 | Return/terminate, optionally completing callback and waking parent waiters |
| 0x10066000 | Jump when popped condition is zero |
| 0x10067000 | Terminate channels whose signal masks intersect a popped mask |
| 0x10068000 | Assign current channel's signal mask |

Child creation inherits the caller's signal mask. Arguments are popped/copied only after successful child allocation in these branches. In the waiting-call branch, allocation failure still records the failed child index and enters wait-child, so no valid child return is guaranteed to wake that parent. There is no independent retry queue for channel exhaustion.

Normal return invokes the callback with a popped value when one exists, clears state, decrements activeCount and wakes parents. Signal termination clears matching states, decrements counts and wakes parents, but does not invoke every canceled callback with a synthesized completion result. A signal can also terminate its own executing channel.

The argument-collection family writes a local four-value temporary in this body; there is no subsequent native-call dispatch using that array shown here. It must not be described as executing an arbitrary external function.

An unrecognized opcode clears the channel and decrements activeCount without the normal return's callback/Wake sequence. A parent waiting on such a termination can consequently remain blocked. Unknown bytecode is not reported as a rich script exception by this switch.

## 9. Unit/game callback instructions

0x10042000 queries a unit value without extra arguments; 0x10043000 supplies four argument values to that query. 0x10044000 asks whether a unit is carried, and 0x10045000 gets transporter ID. 0x10071000 invokes modern piece explosion. 0x10082000 sets a unit value, 0x10083000 attaches a unit and 0x10084000 drops one.

Their game-specific contracts are in [the unit interface](units-unit-script-calls.md), including property IDs, yard checks, drop placement and visibility-gated SFX. The interpreter itself does not repeat alliance/capacity/particle eligibility checks for every operation. It transports decoded values to the virtual callback selected by the current runtime object.

## 10. Piece animation integration

AnimatePieces (`0x4b1c00`) returns when elapsed is zero or no animation is marked changed. Otherwise it visits active piece records and axes, clearing/reasserting activity as work remains.

Translation adds elapsed times signed speed and clamps at the desired coordinate, clearing speed on completion. Angular-speed acceleration adjusts toward target spin speed with signed clamping. That speed adjustment is applied once per animation pass, while the subsequent angle movement uses elapsed times angular speed. They are not both uniformly multiplied by elapsed.

A bounded rotation detects crossing its target around the 65536-unit circle and snaps/clears speed. Unlimited spin retains activity. Final rotation writes are masked to 16 bits. Translation/rotation changes go through virtual setters, allowing UnitScript to invalidate cached geometry/pictures.

The global changed flag remains set when at least one piece continues moving. Animation completion therefore changes the speed fields observed by wait instructions and, on a later pass, lets script threads proceed to build stance, Aim readiness or another script step.

## 11. Optional base callbacks

The base implementations at 0x4b0650..0x4b06a0 provide no-op attach/drop/set-value and zero-valued get/carry/transporter services. The optional presentation callbacks at 0x4b1e50..0x4b1eb0 return zero for visible/cached/shaded and do nothing for legacy explosion, sound, SFX and modern explosion.

These account for the abstract interface's optional behavior without assigning nonexistent particle or sound output to the base class. UnitScript overrides supply the actual game's unit implementation. Destruction uses the virtual deleting-destructor path so per-instance arrays are released through the polymorphic owner.

## 12. Saving interpreter state

SaveScriptState (`0x4b1ec0`) writes to the current box from offset zero:

1. A 0x528-byte record with script checksum, eight channel records and activeCount. Callback pointers are cleared in the saved copies.
2. staticCount times four bytes of per-unit statics.
3. One 0x6c-byte record per piece with movement/turn/spin/acceleration state, current translations/rotations and visible/cached/shaded properties.

Program counters, wait conditions, stacks and signals survive through the channel records. External callback addresses do not. A restored aiming thread cannot obtain a valid old callback simply from the saved address; those relationships require the owner's reconstruction policy.

The bank box is selected by [unit save code](units-unit-save.md), which uses Script plus a sequential unit-save index. The interpreter does not independently choose a global unit-ID account here.

## 13. Loading, checksum rejection and partial failures

LoadScriptState (`0x4b2040`) requires exact box size equal to 0x528 plus current static bytes plus pieceCount times 0x6c. It reads the main record completely and requires its checksum to equal the currently bound program checksum before applying channels. Different script content/layout can therefore reject an otherwise readable saved unit state.

It copies channels while clearing callbacks again, restores activeCount, requires a complete static read, allocates a piece-record buffer and requires a complete read. It then restores motion parameters and current model properties through virtual setters, marks every piece active and sets changed so integration can resume.

A failed static read occurs after channels/count have already changed. A short piece-buffer read returns without releasing that temporary allocation in the shown body. This is not an atomic rollback to pre-load state. Nor does size/checksum validation independently prove every saved PC, stack pointer or waited piece index is within range.

## 14. Completeness and debugging chain

All annotated runtime functions, base stubs and destructor entries are covered, along with every decoded opcode family in RunThread. Asset loading, native unit behavior, particles, rendering and weapon firing retain their own responsibilities. The source establishes channel/stack limits, wait semantics and animation integration without requiring builds or copying source listings.

For a shipyard that never starts work, trace script start success, eight-channel occupancy, waits, piece speeds and build-stance setters. For a gun stuck aiming, trace query/return convention, signals, callback lifetime and return value. For a restored model that resets animation, inspect checksum/size rejection and callback omission before assuming rendering lost the piece state. For a missing scripted wake, trace execution reaching EmitSfx, then its unit-interface visibility and effect-ID gates.
