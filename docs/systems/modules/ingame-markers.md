# Build footprints, order icons, path trails and range overlays

## Scope and projection

The `ingame/markers` group owns six entries from `0x438c00` to `0x439b30`, all in [markers.cpp](../../../src/ingame/markers.cpp). They draw queued-build footprints, terrain-following range circles, selected/debug unit ranges, animated order paths, target icons/coverage and an order-target ellipse. They do not allocate units, run construction work, authorize targeting or calculate projectile collision.

[Order-list overlay dispatch](../../../src/orders/order_list.cpp) selects these callbacks and passes a shared position accumulator between orders. The mission-order table supplies a marker-animation ID. The [general order reference](orders-order-list.md) explains that traversal; [data-file resources](game-data-files.md) supplies the cursor/path sequences these routines borrow.

World positions are 16.16 fixed point. Their standard screen projection is whole X minus view scroll X plus 128, and whole Z minus half whole height minus view scroll Y plus 32. Height therefore raises a marker vertically on screen. The border constants are battle-view offsets, not per-ship dimensions. Drawing primitives own clipping and pixel output.

## Queued-build footprint animation

DrawBuildFootprint (`0x438c00`) returns immediately for type ID zero, without writing the output position. Otherwise it adds the selected unit definition's low/high bounds to the order position. It projects both corners, deliberately using the low corner's height for both projected Y values. This makes a flat footprint overlay rather than a wireframe conforming each corner to terrain or the model's full height.

It obtains an age from gameTick minus the order timestamp, with a nominal zero-to-ten clamp. Both fields are unsigned, so a future timestamp wraps before the comparison and can select the ten-tick endpoint rather than a negative-age zero. Ordinary timestamps are expected to be current/past.

Horizontal/vertical sweep offsets are the projected box spans multiplied by age and divided by ten. Four outer lines are displaced one pixel beyond four corresponding inner lines, giving a contrasting two-color shape. The vertical line positions sweep from each edge toward the opposite edge; at the midpoint paired lines meet, and at the ten-tick endpoint they have exchanged sides. This is a short transition of crossed boundary lines, not a percentage visualization of the construction account.

The builder/order unit's flag bit 4 selects the palette pair: GUI palette 3 plus color2 when set, otherwise palette 1 plus palette 9. After drawing, the callback writes the order position into the shared output accumulator.

This queued footprint is separate from the placement ghost and the under-construction unit model. [Model rendering](graphics-model-render.md) describes construction wireframes/clipping; [unit orders](orders-unit-orders.md) describes factory/mobile construction and the shipyard relationship. A footprint marker's age and palette do not decide when an unfinished hull becomes solid, which model pieces are shown or how much build work has been paid.

## Terrain-following range-circle primitive

DrawRangeCircle (`0x438ea0`) does nothing for radius zero. For a nonzero radius it computes an integer segment quantity as radius times approximately two pi divided by eight, then uses a 16-bit angular step of 65536 divided by that quantity. The loop draws quantity plus one segments, so the final segment can overlap the nominal closure. Integer angular division also prevents a claim of exact continuous geometry.

Each endpoint offsets X/Z using the fixed-point trigonometric helpers and sets its whole Y to the greater of center height and sampled ground height. It therefore drapes over higher terrain while never placing a sampled endpoint below the center's height. It retains fixed-point representation and projects those endpoints through the battle view.

Optional text is placed at the endpoint reached by segment index times three, with a four-pixel downward offset. If the retained label location is zero/zero, the final endpoint is used. Multiple range labels use different indices to reduce overlap; this is a heuristic, not text layout or collision avoidance.

There is no minimum-segment guard. For example, a nonzero radius one yields quantity zero and division by zero. Negative/sufficiently invalid radii can bypass segment initialization and leave endpoint variables unsuitable for label fallback. Input radius shifts, segment arithmetic, terrain samples and sequence/pointer state rely on ordinary valid definitions. A ring is a visual sample of a radius, not a test that every covered cell is passable, visible or attackable.

## Unit rings in ordinary and diagnostic modes

DrawUnitRangeRings (`0x4390a0`) uses the order's unit and definition. With showRanges zero, it draws a minimum-cloak-distance circle only when that distance is nonzero and activation flag value 4 is set, using the shadow color. For a kamikaze definition (flag `0x10000000`) with a non-null explosion weapon it additionally draws a pulsing circle derived from half that weapon's area of effect.

The pulse uses gameTick modulo sixty, grows at twice that half-area radius per sixty ticks, has a nominal lower bound eight and caps at the half-area radius. It then draws kamikazeDistance for a unit with nonzero motion state, or sight distance for motion state zero, and returns. These are display choices; the definition/runtime kamikaze trigger still owns actual detonation.

With showRanges nonzero, nonzero definition radii are drawn and labeled in GUI palette 14: mincloak, sight, radar, sonar, radarjam, sonarjam, build distance, maneuver and kamikazedistance. The label index advances across most fields. Weapon range rings use alternating color1/palette12 according to gameTick parity, with labels weapon1 range through weapon3 range.

Slots one and two require their own weapon-slot flag value 2 and a nonzero weapon range. The third slot instead tests the first slot's flag while dereferencing the third slot's weapon pointer. This can suppress a valid third ring or dereference an absent third weapon when the first slot is active. It is a specific display-path inconsistency, not evidence that combat uses slot one's eligibility for weapon three. [Weapons](weapons-weapons.md) supplies the actual firing/acquisition contracts.

The diagnostic radii are loaded values. Showing a build-distance circle does not bypass worksite acceptance or movement goals. Showing sonar/radar radii does not account for every contact's cloak, underwater state, jammer, alliance or maintained sensor-map result. Those are described in [radar](map-radar.md), [LOS](map-line-of-sight.md) and combat references.

## Target position, remembered locations and order icons

DrawWeaponCoverage (`0x439740`) first resolves its marker position. Without a target unit it uses the order position. With a target, it asks whether the target is visible to the ordering unit's player. An invisible target with cache-valid flag `0x200000` uses cached whole X/Z but the target's current Y. Otherwise it uses the target's current full position, refreshes cached X/Z and sets cache-valid.

Thus an invisible target with no prior cached location can initialize its cache from live position in this drawing path. An already cached invisible target retains old horizontal coordinates but can reflect current height. This routine's marker state is not identical to a weapon's aim target or an authoritative fog-of-war position cache. The render callback itself mutates the order's cache.

A marker-animation ID zero writes the resolved position to the output accumulator and returns without an icon. In diagnostic showRanges mode, animation IDs one or two also draw enabled weapon slots' area-of-effect and coverage rings plus nonzero attack-run length. Weapon labels use zero-based indices in this path. The area-of-effect value is passed directly as the circle radius here, whereas the ordinary kamikaze pulse uses half its explosion weapon's area value. Neither display convention changes actual damage geometry.

For a nonzero marker ID, the callback selects the corresponding cursor sequence, computes a frame from gameTick divided by twice the sequence duration modulo frame count, draws it blended at the projected position, and writes that position to output. There is no null-sequence, zero-duration, zero-count or marker-index guard. Asset/mission-table consistency is a precondition. The frame is global-tick based, rather than restarted at the order's timestamp.

## Animated connections between orders

DrawPathAnim (`0x4394e0`) snapshots the incoming output position, invokes DrawWeaponCoverage to draw/resolve the current order, and returns immediately when its flag is zero. Otherwise it computes the three-dimensional fixed-point difference between previous and resolved positions and its integer Euclidean length. Distances below one world unit are skipped.

It uses cursorPathIconSeq for trail frames. The initial path offset advances across 48 world units over thirty game ticks, based on order age modulo thirty. Trail samples then occur every 48 world units while offset is less than total distance. Each sample interpolates X, Y and Z using 64-bit multiply/shift arithmetic and is projected with its interpolated height; it is not separately snapped to terrain along the route.

The initial animation frame uses order age divided by at least one duration, modulo frame count. Every successive sample advances the frame modulo that count. Duration zero is guarded in this path, but frame count zero and null sequence are not. Endpoint icons use the separate DrawWeaponCoverage timing above. A future/wrapped timestamp is not comprehensively normalized merely by the source's max-with-zero expression.

This connects successive displayed order destinations with a straight three-dimensional interpolation. It does not draw every A* cell, steering waypoint, collision detour or guaranteed traversable naval channel. [Pathfinding](ai-pathfind.md), [order targets](orders-order-targets.md) and [unit steering](units-unit-scripts.md) own the actual movement route.

## Target ellipse

DrawOrderRangeRing (`0x4399f0`) uses the target unit's live position and definition field at `+0x178` when a target exists. Otherwise it uses the order position and radius 32. It makes a screen-space ellipse with horizontal radius equal to that field and vertical radius truncated from 0.89 times it, draws sixteen segments in palette 12 and copies the chosen world position to output.

Unlike DrawRangeCircle, it does not resample ground around the perimeter. Unlike DrawWeaponCoverage, it does not perform a visibility/cache lookup for its target. Its radius is a model/definition field, not a loaded weapon range. Callback selection and visibility gates in the surrounding overlay traversal therefore matter; this function alone provides neither targeting permission nor a universal contact-disclosure policy.

## Completion and display boundaries

All six owned entries are covered, including cache mutations, animation timing, projection, range modes and boundary failures. They borrow units, definitions, orders, GAF sequences and the destination surface; they own no persistent allocation, packet format or save account. Exact asset sizes, durations, palette values and current order flags affect the rendered output. No images, game execution or rendering tests were produced for this source-based reference.
