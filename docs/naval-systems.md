# Naval systems technical reference

This document explains the naval behavior recoverable from TA: Byte Tactics at commit `97d7ac59b460417bcdfa629cf90da99f31818c65`. It describes the existing implementation in prose, tables, and mathematical relationships. It contains no copied source listings or pseudocode implementations.

The subject includes surface vessels, submerged units, hovercraft, amphibious units, shipyards, construction vessels, aircraft attacking water targets, shore weapons, sensors, projectiles, particles, wreckage, and the common systems connecting them.

The broader [systems manual](systems/index.md) develops the shared engine systems in dependency order. Its [economy chapter](systems/03-economy.md) extends the resource and tidal-generation analysis in this reference.

## Contents

1. [Scope and evidence](#1-scope-and-evidence)
2. [The engine's model of naval units](#2-the-engines-model-of-naval-units)
3. [Water, terrain, depth, and placement](#3-water-terrain-depth-and-placement)
4. [Shipyard construction and launching](#4-shipyard-construction-and-launching)
5. [Construction wireframes and materialization](#5-construction-wireframes-and-materialization)
6. [Why different products look different inside shipyards](#6-why-different-products-look-different-inside-shipyards)
7. [Movement, steering, and obstruction](#7-movement-steering-and-obstruction)
8. [Scripts, wakes, bubbles, and smoke](#8-scripts-wakes-bubbles-and-smoke)
9. [Visibility, sonar, radar, and jamming](#9-visibility-sonar-radar-and-jamming)
10. [What can shoot at what](#10-what-can-shoot-at-what)
11. [Aiming, firing, and projectile motion](#11-aiming-firing-and-projectile-motion)
12. [Impacts, damage, and destruction](#12-impacts-damage-and-destruction)
13. [Other unit interactions](#13-other-unit-interactions)
14. [Multiplayer and persistence](#14-multiplayer-and-persistence)
15. [Worked traces and diagnostic guide](#15-worked-traces-and-diagnostic-guide)
16. [Source index and remaining research](#16-source-index-and-remaining-research)

## 1. Scope and evidence

### 1.1 What this reference establishes

The executable's systems are distributed across generic unit, order, terrain, graphics, script, weapon, and player modules. A ship's behavior cannot be understood by reading one ship module. This reference follows those dependencies and identifies the functions supporting each explanation.

References link to repository files. Function addresses identify the relevant routine when a file contains many functions. Names in the decompilation are provisional; loaded data keys and script entry names provide stronger naming evidence than an inferred local variable name.

**Confirmed** means a described condition or action is directly present in the inspected implementation. **Derived** means a stated consequence follows from those conditions. **Asset-dependent** means the engine supplies the mechanism but the actual unit definition, model, animation, or weapon data determines the result. **Unresolved** means the inspected files do not justify a more specific conclusion.

### 1.2 Limits on a complete unit roster

The loader reads unit definitions from FBI files, models from 3DO files, behavior from COB files, weapons from TDF files, movement classes from `moveinfo.tdf`, and build options from side and download data. The tracked source is not a complete, decoded catalogue of those game assets. Consequently, this document establishes the engine rules and conditional interaction matrix, rather than inventing a named roster of every ship's statistics or scripts.

An exact statement such as which named destroyer attacks which named submarine requires the actual loaded definitions and scripts. A label such as destroyer, cruiser, submarine, or shipyard does not by itself establish a weapon's flags, target preferences, build position, or particle emission rate.

This was a static source investigation. No match checker, placement build, game session, or visual capture was run for the documentation. No runtime claims about a particular screenshot or asset configuration are implied.

Evidence: [unit and asset loading](../src/units/unit_types.cpp), especially `0x42bf40` and the FBI/COB/3DO load paths; [weapon loading](../src/weapons/weapon_types.cpp), `0x42e440`.

## 2. The engine's model of naval units

### 2.1 A naval unit is a combination of independent properties

There is no single ship flag in the inspected decision paths that controls all naval behavior. The following inputs are independently significant:

| Input | Role | Does not establish by itself |
| --- | --- | --- |
| `floater` | Surface-height handling and special water-target eligibility | Weapon type, sonar, or construction appearance |
| `waterline` | Vertical draft relative to sea level | Minimum navigable water depth |
| `canhover` | Hover height behavior, water speed exemption, special water-weapon rejection | Universal immunity to all damage |
| `upright` | Selects a height-update branch | Amphibious capability |
| `amphibious` | Used in underwater permission checks, including aircraft-related checks | A torpedo weapon |
| Movement class | Footprint, permitted water depths, slope limits, pass map | Model size or rendering topology |
| Model extents | Top, bottom, bounds, visibility samples, damage geometry | Navigational footprint |
| `canfly` and current movement mode | Aircraft handling, order routing, air-target tests | Every weapon's permission to fire |
| Weapon definition | Range, air restriction, water permission, trajectory, damage, effects | Automatic selection priority |
| Target categories | Per-weapon preference and pursuit restrictions | A replacement for geometric reachability |
| COB script | Build piece, moving parts, aim completion, wake and smoke requests | Changes to terrain depth limits |

The runtime unit also has current position, heading, health, remaining construction, activation state, movement state, three weapon slots, and attachment relationships. Definitions describe capabilities; runtime state determines which capabilities can operate now.

### 2.2 Reliable flag meanings

The FBI loader maps the first definition flag word as follows:

| Bit | Mask | Loaded meaning |
| --- | --- | --- |
| 11 | `0x800` | `canfly` |
| 12 | `0x1000` | `canhover` |
| 18 | `0x40000` | `activatewhenbuilt` |
| 19 | `0x80000` | `floater` |
| 20 | `0x100000` | `upright` |
| 21 | `0x200000` | `amphibious` |
| 25 | `0x2000000` | `noshadow` |
| 27 | `0x8000000` | `hoverattack` |
| 30 | `0x40000000` | `digger` |

Some file-local views use informal field names such as “floats” for bit 12 or describe bit 20 as water-related. Those comments should not override the loader: bit 12 is loaded from `canhover`, and bit 20 from `upright`. This distinction matters when interpreting height logic.

The same caution applies to runtime flags versus definition flags. For example, runtime `0x200` is used for sonar detection; definition `0x200` is a different property. Never transfer a bit meaning between unrelated words.

### 2.3 Coordinates and time

World positions use signed 16.16 fixed-point components: the upper half is the integer coordinate and the lower half the fraction. Horizontal motion and most range tests use X and Z. Y is vertical. Sea level is stored as an unsigned byte and promoted for comparisons.

Terrain occupancy cells span 16 world units. Footprint-to-cell conversions therefore combine fixed-point scaling with that cell size. Sight and particle visibility sampling also uses coarser grids in several paths; a terrain cell and a visibility cell should not be treated as the same object.

Many loader values expressed in seconds are converted to ticks by multiplying by 30. Construction work is divided into per-tick contributions using 30 as well. Tick counts describe simulation time, not a guarantee of 30 rendered frames each second.

Evidence: [definition loader](../src/units/unit_types.cpp), `0x42bf40`; [shared definition](../src/units/unit_def.h); [position representation](../src/units/unit_position.cpp); [weapon time conversions](../src/weapons/weapon_types.cpp).

## 3. Water, terrain, depth, and placement

### 3.1 Sea surface and seabed are separate

Water is treated as a sea-level plane over terrain. Terrain supplies interpolated ground height, cell minimum and maximum heights, blocking features, and occupancy. A vessel can be visually afloat while its footprint fails a seabed depth condition.

The ground-height query interpolates four neighboring height samples using the position within a 16-unit cell. It returns an out-of-map sentinel when the required neighbors are unavailable. Placement tests instead inspect height bounds and occupancy over multiple cells. A center-point height query cannot establish that an entire hull footprint fits.

### 3.2 Depth limits become terrain-height limits

For sea level S, terrain height H, minimum water depth Dmin, and maximum water depth Dmax, allowed terrain is bounded below by S minus Dmax and above by S minus Dmin. Equivalently, water depth must remain between the configured minimum and maximum.

This gives several consequences:

- A positive minimum depth excludes sufficiently shallow seabed and land.
- A larger minimum depth can isolate a deep-draft movement class from a shallow channel.
- A finite maximum depth can exclude deep water, even if a unit otherwise looks suited to water.
- Negative depth limits allow configurations that extend above the sea plane.
- These are data constraints; hull width and the draft used for display are separate inputs.

Movement-class defaults are maximum water depth 10000, minimum water depth -10000, and slope limits of 255. A named class or local definition supplies the restrictive values. The class loader also clamps the slope-related values against their corresponding limits.

### 3.3 Named movement classes take precedence

The unit loader first attempts to resolve `movementclass`. If successful, its footprint and water/slope limits are copied into the unit definition. If no class resolves, it reads equivalent movement information from the unit's own record. Changing an FBI field that is subsequently replaced by a named class will not necessarily change the resulting unit.

### 3.4 Placement checks the footprint

The build-placement scan considers the requested footprint, yard-mask requirements, cell occupancy, blocking features, map flags, visibility conditions, slope, and depth. Some yard cells contribute ground-support samples; others contribute overhead or water constraints. A mask can produce a placement height based on sea level minus draft when the sampled support-height interval is empty.

The movement-position footprint probe likewise rejects out-of-bounds positions, blocking features, occupied cells, height extrema outside the depth interval, and excessive local slope. It allows the unit's own ID in occupancy checks. One visibility branch returns success before detailed obstacle checks when the relevant cell is not visible, so a planning probe is not omniscient knowledge of every obstruction.

For an aircraft without the amphibious flag, that probe raises its permissible lower terrain bound to sea level when the otherwise allowed bound would extend below water. This is a specific permission check, not a general assertion that all aircraft can land underwater.

### 3.5 Yard maps are navigational data

For building definitions, `YardMap` characters are converted to per-cell bit masks. Recognized characters include `.`, `f`, `o`, `c`, `O`, `w`, `C`, `y`, `Y`, and `G`. Uppercase and lowercase forms can produce different masks. They should not be normalized as formatting.

Opening a yard calls a footprint-clearance test first. If successful, the runtime yard-open state changes, the footprint cells are reclaimed for the new state, and movement-class pass maps are refreshed. A visible door animation alone does not establish that the yard's occupancy state changed successfully.

This explains a useful diagnostic split: an exit can be obstructed by surrounding vessels, a closed or unsuccessful yard transition, terrain depth, or a product footprint that does not fit. All can present as a ship failing to leave.

Evidence: [ground interpolation](../src/units/units_485070.cpp), `0x485070`; [movement classes](../src/units/movement_class.cpp), `0x4402e0`, `0x440340`; [definition class and yard loading](../src/units/unit_types.cpp); [placement and yard state](../src/map/terrain.cpp), `0x47d2e0`, `0x47dac0`; [position footprint probe](../src/map/terrain_47e2d0.cpp), `0x47e2d0`.

## 4. Shipyard construction and launching

### 4.1 Shipyard production uses the common factory order

The order table maps `BuildingBuild` to the factory-style production handler at `0x402640`. Despite a misleading nearby comment mentioning a mobile builder, its body queries a build piece and attaches the product to the producer. The separate mobile site-building handler is at `0x403a20`.

The factory sequence is:

1. Clear the previous product reference and check the queued count.
2. Activate the producer and wait for its script-controlled build stance.
3. Ask `QueryBuildInfo` for the model piece used as the production position.
4. Convert that piece to a world position and test the product's footprint there.
5. Create an unfinished product, attach it to the producer's piece, and assign its `GetBuilt` order.
6. Add paid construction progress, emitting nanolathe particles when progress succeeds.
7. Complete and detach the product under the completion routine's conditions, update the queue and interface, and continue production.

The script's build-stance state can delay this sequence. A yard whose opening or preparation animation has not reported readiness can remain in the wait state before productive work begins.

### 4.2 Creation can wait before any hull exists

At the production position, a failed placement test causes a 15-tick retry. Failed allocation or unit creation causes a longer 300-tick wait and the “Unable to create any more units” message. These waits happen before the ordinary progress loop and should not be diagnosed as resource stalls.

The target must fit using its own definition and footprint. A shipyard's successful placement does not certify that every possible product fits at every surrounding obstruction state.

### 4.3 An unfinished product is a real unit

The product has an ID, definition, health, position, model state, remaining construction fraction, and orders. It is attached through the same parent/child relationship used by transport and other attachment mechanisms. The parent maintains a child list and the child records its parent and attachment-piece index.

Consequently, the construction hull is not merely a decorative overlay. It can have state synchronized across machines, participate in references and damage processing, and be rendered as part of its parent composite. Its operational weapons are managed by the under-construction order.

### 4.4 Progress, resources, and health

`buildLeft` is the fraction still unbuilt: 1 means no progress and 0 means complete. A successful positive work contribution reduces it by work divided by the product's build time, clamped to the valid interval.

The actual change in that fraction determines proportional energy and metal charges. The resource account records demand and only accepts work when its energy and metal stall conditions permit it. The hull therefore does not have to advance every tick just because it remains visible and the yard remains active.

Hit points are incremented by the difference between integer health contributions before and after the work step, capped at maximum health. Damage already suffered is not automatically erased by treating current health as a simple percentage of construction. Completion and full health are separate questions.

The factory handler contributes integer worker time divided by 30 before conversion to floating point. Very small rates can therefore lose their fractional contribution in that path. Exact build duration should be calculated from the implementation and actual data, including integer truncation and resource stalls, rather than from an ideal continuous formula alone.

Nanolathe emission in this handler occurs after `AddBuildProgress` reports success. Its target region uses the product position and model bounds, including the upper vertical bound. The final argument of 6 selects a particle-system list; it is not a count of six visible sparks. The nano emitter attempts five particles per emission and can skip a particle whose calculated travel duration is zero.

### 4.5 Completion is a set of state changes

`FinishConstruction` clears the remaining fraction, invalidates the cached drawing state, marks a redraw/state flag, handles detachment for a qualifying mobile product, optionally activates `activatewhenbuilt`, updates the build interface, and broadcasts the builder/product link under the relevant owner conditions.

The product's `GetBuilt` order then recognizes completion. For mobile products it converts queued producer move and patrol instructions into suitable product orders. It also copies standing fire and movement settings under readiness conditions, and copies the group for a human owner. If no movement or patrol instruction was inherited, it adds `PARK`.

This is the source-level explanation of factory rally behavior: queue entries are interpreted for the new product. It is not evidence of a special hard-coded naval launch animation or fixed launch distance.

### 4.6 Cancellation and abandoned construction

Factory cancellation can refund already invested metal, finish/detach the target through the common path, and destroy it with the cancellation damage type. Some computer-player difficulty branches scale refunds to 50% or 70%; the ordinary branch credits the full computed metal refund. The inspected cancellation path does not establish an energy refund.

The unfinished unit's `GetBuilt` order has wait and decay states. It initially waits 300 ticks, then uses 30-tick waits and a conditional 11-tick decay contribution. Decay calls the same progress function with negative work. Negative work raises the remaining fraction, removes health, and credits metal through its refund path. At a fully unbuilt state, the target is destroyed. The exact scheduling depends on order flags, so “all abandoned hulls decay immediately” would be incorrect.

### 4.7 Construction vessels build at sites

The mobile construction handler snaps a requested site to a footprint-aligned position, installs a footprint marker, checks reach and occupancy, and retries a blocked site. It creates the target at the site and starts the builder's directional construction script. It does not use the factory handler's attachment to `QueryBuildInfo`.

Both paths share progress accounting, under-construction behavior, and the materialization renderer. The difference is how the target is positioned and how the producer reaches and services it. Assistance and aircraft construction use additional order handlers but also call the common progress routine.

Evidence: [factory production and under-construction orders](../src/orders/unit_orders.cpp), `0x402640`, `0x402da0`; [site construction](../src/orders/unit_orders_403a20.cpp), `0x403a20`; [build-stance wait](../src/orders/order_queue_4384a0.cpp), `0x438700`; [completion and progress](../src/ingame/control_panel.cpp), `0x41b8d0`, `0x41ba60`, `0x41bcd0`; [resource requests](../src/game/economy.cpp), `0x4011c0`; [attachment](../src/units/unit_position.cpp), `0x48ab70`.

## 5. Construction wireframes and materialization

### 5.1 The display has three interacting layers

Construction rendering combines an ordinary model image, a secondary per-pixel plane carrying height/depth information, and a polygon-edge pass. Remaining build progress changes which image pixels survive and which colors replace them. Edges are then drawn over the result subject to the rasterizer's tests.

The resulting appearance can include an outline, a moving colored band, colored filled regions, transparent regions, and normally textured regions at the same time. “Wireframe” is a useful visual description but does not mean the engine switches to a completely separate line-only ship model.

### 5.2 Projection and the secondary plane

Visible model pieces are transformed and projected. Horizontal screen position uses model X; vertical screen position uses the negated model Z adjusted by half the model Y. The vertex depth/height value is model Y plus a bias, normally 50, with a different bias for the digger path. The lit rendering path also maintains lighting information.

This means the construction sweep is tied to model-space height as transformed for rendering. The inspected path does not normalize each hull's entire height to a common zero-to-one range before applying the construction threshold. A tall mast, a low deck, and a deep keel therefore intersect the same threshold differently.

Unfinished units force allocation of a two-plane bitmap. A completed model can use a simpler one-plane bitmap when other conditions allow it. Both ordinary and lit piece paths relax their usual piece-color grouping filter while construction remains nonzero, while still respecting piece visibility.

### 5.3 Remaining progress selects five visual regimes

The renderer truncates remaining construction multiplied by 255 to an integer A. It computes two animated palette indices within `0xA0` through `0xAF`. Unit ID changes their phase; game ticks advance them at two different rates. Construction colors can therefore continue pulsing while paid progress is stalled.

| Remaining-progress integer A | Recolor behavior | Approximate completed portion |
| --- | --- | --- |
| 236 to 255 | Hide pixels above and below a narrow active band; color the band with the first animated index | Initial 0 to 8% |
| 201 to 235 | A second sweep with the same hide/band operation and a different threshold scale | About 8 to 22% |
| 116 to 200 | Hide the upper region, color the lower region with the first index, and color the transition with the second | About 22 to 55% |
| 31 to 115 | Color the upper region, retain the lower region's original pixels, and use the second index at the transition | About 55 to 88% |
| 0 to 30, while remaining progress is still nonzero | Retain original pixels on both sides and color only the moving band | Final roughly 12% |

The percentages are orientation aids, not extra gameplay thresholds. Integer conversion determines the actual boundaries. Exactly zero remaining progress bypasses this construction-recolor routine completely, including its final edge pass.

The first two threshold sweeps scale their respective ranges across the byte interval. The middle two use signed intermediate arithmetic followed by byte conversion, which wraps negative results into the byte range. Replacing that with clamped continuous interpolation would change the display. The final sweep scales A across the final 30-step range.

### 5.4 The active band is four shade levels wide

For a threshold L, pixels with a secondary-plane value below L minus four form the lower region. Pixels at or above L form the upper region. The interval between them is the active band. When L is smaller than four, its lower bound is clamped to zero.

Each region's operation is independently selected: retain its existing pixel, replace it with the transparent color key, or replace it with an animated palette index. Transparent source pixels are skipped by the recolor scan. Thus construction does not fill the rectangular image bounds with color; it operates on rasterized model pixels.

### 5.5 Where the edges come from

The edge pass visits visible pieces, transforms their vertices, and walks their polygon index lists. It closes each face by appending its first vertex to the temporary face contour. The downstream routine derives scanline spans and plots their endpoints rather than drawing an unrestricted X-ray grid through the world.

The span-end plotter uses the secondary-plane information when it is present: an endpoint is drawn when its height value is at least the stored value, and drawing updates that stored value. Without that plane it writes the endpoints directly. Occlusion and edge visibility therefore depend on rasterization and stored height values. Individual polygon topology matters: a faceted hull and a broad flat hull can produce noticeably different outlines even at identical build progress.

Faces marked through the model's special first-face convention are skipped consistently with the piece-rendering path. Hidden pieces are excluded. The renderer does not infer a new scaffold topology from the unit's footprint.

### 5.6 Rendering and economy are coupled only through state

Successful construction changes remaining progress and health and marks state dirty. Rendering reads that state and the current tick. It does not spend metal or energy, and its pulsing animation does not prove the build account accepted work that tick.

Conversely, particle emission is requested from the successful-work branch of the construction order. A hull can visibly pulse without new nanolathe particles, or have its particles hidden by visibility while its progress changes. Diagnose progress from `buildLeft` and resource/order state, not from a single visual cue.

Evidence: [bitmap allocation and construction recoloring](../src/graphics/model_render.cpp), `0x4586a0`, `0x458d30`, `0x458dd0`, `0x459830`; [lit piece rendering](../src/graphics/model_render_4581e0.cpp), `0x459c70`; [piece edges](../src/graphics/model_render_458fa0.cpp), `0x458fa0`; [polygon-edge rasterization](../src/graphics/draw.cpp), `0x4c0820`, `0x4c0a90`.

## 6. Why different products look different inside shipyards

### 6.1 Product attachment determines the starting transform

The producer's script chooses the build piece. Its hierarchy and current translations and rotations determine the world position. The product's own model hierarchy then determines where its deck, keel, turret, mast, and hidden pieces lie relative to that origin.

For an attached moving unit, the position updater asks for the parent's piece position and angles. A floater receives an additional floor at sea level minus its own draft. If the parent piece lies lower than that floor, the product position is raised. If it lies higher, it is not forced downward by this particular clamp.

Thus two products sharing one shipyard can differ vertically because they have different waterlines or model origins, even if `QueryBuildInfo` returns the same piece. This is a direct consequence of the attachment-height rule. It does not establish the numeric offset of any named vessel without its data.

### 6.2 The yard and hull are composited with depth

The model renderer expands the producer's composite image bounds to include eligible attached children. Each child receives its own forced two-plane picture and construction shading. The child picture is offset by the difference between parent and child world positions and merged using `DrawFrameDepth`.

As a result, the yard's foreground structures can occlude parts of the unfinished vessel while other parts remain visible. The outcome depends on model geometry, piece transforms, depth values, and position. It is not simply “draw the entire hull over the yard” or “draw the entire yard over the hull.”

Children marked with the runtime hidden-attachment flag are skipped by this visible child loop. Attachment presence alone therefore does not guarantee that a child image appears.

### 6.3 Water processing happens after child composition

After combining the parent and children, the renderer calculates the sea-level threshold relative to the parent image's height basis. If the parent origin lies below sea level, it either cuts the below-threshold pixels or tints them according to ownership and the runtime sonar flag.

For a nonlocal owner lacking sonar detection, below-threshold pixels are made transparent. The local owner's corresponding pixels, or those with the sonar flag, are remapped through the blue palette table. The lower-level operations compare the secondary-plane value with the threshold using an inclusive boundary.

This is palette remapping and transparency in a software-rendered image. The inspected code does not implement a physically refracted water volume. For attached children, the composite's shared depth basis and the child's vertical offset are relevant to where the water cutoff lands.

### 6.4 Conditional product comparison

| Product configuration | What the traced renderer predicts |
| --- | --- |
| Wide, low surface hull | Its broad deck occupies a narrow height range, so many pixels can change together during a sweep |
| Tall surface hull | Mast and superstructure intersect different thresholds from the deck; the outline is taller and more segmented |
| Large waterline offset | The floater floor is lower relative to sea level, changing the exposed versus tinted/cut parts |
| Submerged model without floater behavior | The special attached-floater floor does not apply; attachment and subsequent height handling must be examined |
| Script hides selected pieces | Those pieces do not contribute to the visible picture or construction edge pass |
| Product at an animated build piece | Its construction transform can follow the piece; exact motion is script-dependent |
| Product extending beyond the yard | Composite bounds grow to include it; overlapping depth still determines visible portions |
| Enemy construction with no sonar | Underwater pixels can be removed by the composite water pass |

These are configuration classes, not claims that every stock ship in a class uses the same setup. In particular, a submarine is not necessarily rendered by a dedicated submarine-construction branch. The traced construction code has no named-ship switch selecting different wireframes.

### 6.5 Placement preview is a separate stage

The mouse-driven placement validity and footprint marker exist before creation of a runtime product. A placement ghost, a queued build footprint, a materializing hull, and a completed ship are different objects or stages in the interaction. A red or invalid placement indicator does not describe the same state as a resource-stalled hull already attached inside a shipyard.

Evidence: [attached movement](../src/orders/order_list.cpp), `0x43d6d0`; [piece world transforms](../src/units/unit_scripts.cpp), `0x43def0` and following position helpers; [composite bounds](../src/graphics/model_render.cpp), `0x458310` and `MergeIntoComposite` at `0x4589c0`; [composite drawing and water pass](../src/graphics/model_render_4589c0.cpp), `0x459200`; [blue tint and cutoff](../src/graphics/gaf.cpp), `0x4b96e0`, `0x4ba1b0`; [placement interaction](../src/ingame/build_placement.cpp).

## 7. Movement, steering, and obstruction

### 7.1 Ships use the common nonair movement pipeline

The relevant motion stages are path selection, steering, velocity calculation, candidate-position integration, occupancy validation, spatial-map updates, height updates, and script movement notifications. A naval hull changes the inputs and permissible terrain; the inspected ground steering path is shared machinery.

Movement-class pass maps cache two-bit cell values. Their build process accounts for the full footprint with horizontal and vertical filtering and edge treatment. They can be refreshed over a rectangle after yard or occupancy changes. The pathfinder uses an open heap, estimated goal cost, eight directions, and additional turn/cell penalties.

The neighbor expansion discourages frequent changes of direction: a turn made after a short straight run adds an extra cost. These costs explain why the chosen route need not be the visually shortest polyline. They do not establish real-world hydrodynamic steering.

### 7.2 Acceleration and braking

The mover maintains a scalar speed and derives horizontal velocity from heading. Speed is increased or reduced by the requested acceleration/braking amount and clamped to zero and a type-dependent cap.

The cap is scaled by a table indexed by pitch, clamped to an eleven-entry interval. If the origin is below sea level and the unit is neither a floater nor a hover unit, the cap is halved. This produces a specific underwater slowdown for qualifying nonfloating, nonhover units. It does not halve the speed of every unit that visually touches water.

### 7.3 Turning and stopping distance

Steering samples three path points. It can adjust the immediate aim point along a path segment, computes the desired heading, and clamps the turn by the unit definition's maximum turn amount. It then decides between acceleration and braking using distance, current speed, heading error, and a stopping-distance calculation.

Consequently, a ship can slow near a turn or destination and cannot necessarily instantaneously rotate to a newly requested heading. Hull size, turn rate, maximum speed, acceleration, braking, and the current path all influence the visible response.

### 7.4 What happens at a blocked cell boundary

The candidate position is the current position plus velocity. If it stays in the same footprint cell and mode, the position can be accepted without a full cell transition. A transition invokes the footprint placement test for the relevant authoritative player types.

When blocked, the unit is clamped inside its current cell. If its speed exceeds half the type maximum, it is capped to that half maximum and velocity is recalculated. When accepted, the unit is removed from its old occupancy, assigned its new cell and mode, reinserted, and its line of sight is updated.

This is not a continuous hull-against-hull physics solver. Crowding at a yard mouth can produce cell-clamping and path-replanning behavior even when parts of the visible models seem to leave a gap.

### 7.5 Vertical position

There are several height paths, and their conditions matter:

- The snap helper places hover units at the higher of ground height and sea level minus draft, and other units on ground height.
- The periodic height update is gated by a dirty flag or hover capability and by current movement state.
- In that update, `upright` selects a branch that uses the hover floor for hover units and ground height otherwise.
- A qualifying floater in the other branch is placed at sea level minus draft.
- Remaining cases use terrain alignment.
- Attached floaters have the separate parent-piece floor described in section 6.

These are exact branch roles, not a claim that invoking one helper in isolation reproduces the full unit lifecycle. The loader's names must be used to interpret the flags.

### 7.6 Water is not visibly simulated as wave physics here

The traced height paths use terrain height, sea level, draft, and attachment transforms. They do not calculate buoyant forces from hull volume or integrate wave displacement. Any bobbing, moving superstructure, propeller animation, or build-piece motion beyond these paths requires script/model evidence.

Evidence: [pathfinder](../src/ai/pathfind.cpp), especially `0x40da70`; [pass maps](../src/units/movement_class.cpp), `BuildPassMap`, `0x440830`, `0x440a40`; [steering and integration](../src/orders/order_list.cpp), `0x43cbb0`, `0x43cc20`, `0x43cd20`, `0x43d6d0`; [height](../src/units/unit_position.cpp), `0x48a7f0`, `0x48a870`.

## 8. Scripts, wakes, bubbles, and smoke

### 8.1 Movement tells scripts when behavior changes

The movement-rate updater chooses zero when stopped, constrained, or attached, otherwise chooses one of three speed bands from the definition thresholds. A transition from stopped invokes `StartMoving`; a transition to stopped invokes `StopMoving`; the selected nonzero band invokes `MoveRate1`, `MoveRate2`, or `MoveRate3`.

The callbacks allow individual scripts to start and stop propellers, wake emitters, smoke, or other animation loops. The engine does not prove that every ship emits a wake continuously just because its scalar speed is positive. Actual requests come from the script.

### 8.2 Scripts receive an occupancy classification

`setSFXoccupy` is called when an occupancy state changes. For units in the qualifying movement states, the implementation starts from the previous classification and applies these tests in order:

| Condition | Assigned value |
| --- | --- |
| Origin above sea level | 4 |
| Otherwise, origin fewer than five whole units below sea level | 1 |
| Origin plus draft equals sea level | 2, overriding the preceding assignment |
| Origin plus model-top height is strictly below sea level | 3, overriding earlier assignments |
| Outside the qualifying runtime movement states | 0 |

The underwater branch contains independent tests, not an exclusive chain. Equality at the sea plane and the order of overrides matter. If none of the underwater tests assigns a new value, the previous value remains. These numbers are the script interface; a unit's COB program decides how to react.

### 8.3 Effect emission is visibility-gated

`EmitSfx` first checks whether the local player can see the emitting unit. If not, it returns without creating that local effect. It then refreshes the object's transformed state and derives either one world point from a piece or a pair of points from the piece's stored effect offsets.

This visibility gate can remove cosmetic wakes or smoke without altering movement or weapons. It should not be used as a networking or simulation rule for damage.

### 8.4 Script effect IDs

| Script effect ID | Engine request |
| --- | --- |
| 0 | Thrust particles, first timing variant |
| 1 | Thrust particles, second timing variant |
| 2 | Wake particles between the first and second effect points, period 16 |
| 3 | Same direction, period 8 |
| 4 | Wake particles with endpoints reversed, period 16 |
| 5 | Reversed endpoints, period 8 |
| `0x101` | White smoke at the piece point |
| `0x102` | Black smoke at the piece point |
| `0x103` | Bubble helper from the piece point toward the same X/Z at sea level |

Reversing endpoints changes the wake direction. The one-point smoke/bubble IDs use the high-bit selection for deriving the source point. These interfaces explain available effects, not which named stock unit uses each one.

### 8.5 Wake particle lifecycle

The wake initializer stores the two supplied points, normalizes their difference to a fixed step of approximately half a world unit, stores period and lifetime controls, and emits immediately. The shown initializer has no guard for identical endpoints before its division, so a zero-length emission direction is a concrete edge case for script/asset investigation.

Each emitted wake particle is jittered by up to three whole units on each axis around the source. It stores the direction as velocity, expires at current tick plus six times its period, and uses a palette cycle from `0x61` to `0x67` or in the reverse direction. The emitter advances its next emission time by one tick in this path.

On update, a particle adds its velocity to position and advances the palette value at the selected period, wrapping within the configured bounds. On draw, it is a one-pixel rectangle at its projected position, gated by particle-position visibility. It is not a model of a wave crest with physical width and displacement.

Expiration occurs either after its deadline or when the terrain under its current position is no longer strictly below sea level. The test is on ground height beneath the particle, not a rule that the particle's own Y must remain underwater. This makes shoreline termination terrain-dependent.

### 8.6 Construction particles

Nanolathe systems use source and target regions, with the initializer narrowing the supplied segments to their middle portion before emitting. Individual particles move by velocity and cycle an animation counter from 1 through 7; expiry is based on the tick deadline. Forward and reverse emission entry points support construction and dismantling-style effects.

The emission routine attempts five particles, sampling source and target positions within the supplied regions. It derives travel duration from distance at approximately four world units per tick, skips zero-duration cases, and sets velocity to cover the displacement in that duration. Visible output can therefore contain fewer than five particles from one emission.

Their visual path is separate from construction recoloring. The hull's edge pattern is generated from geometry, while nano particles are separate transient objects.

The bubble helper actually creates the same wake particle class as the wake helper. It selects the descending palette-cycle direction, while ordinary wakes select the ascending direction. Bubble travel toward the sea surface comes from the supplied endpoints, not a separate buoyancy solver.

Particle systems are stored in indexed lists. The append helpers remove the oldest system when the selected list already contains more than 400 systems, then append the new one. This is a limit on emitter/system objects, not on the number of individual particles. It is another reason cosmetic output under heavy activity need not be an exact count of all effect requests.

### 8.7 Smoke and debris effects

Smoke particles drift horizontally using the game's wind values. Their vertical advance uses a game scalar in the particle step. Animation advances after a partly randomized countdown, and the puff expires after its configured animation rounds. The draw path applies explored/current sight rules and blends the selected GAF frame.

Weapon smoke trails, script smoke, damage effects, explosion sequences, exploded model pieces, and wakes are separate producers. A ship can have several at once. The existence of one particle class should not be treated as evidence that all naval effects share its lifetime or visibility rule.

Evidence: [movement callbacks](../src/orders/order_list.cpp), `0x43da70`; [occupancy callback](../src/units/unit_scripts.cpp), `0x43db50`; [effect dispatcher](../src/units/unit_script.cpp), `0x480eb0`; [particle systems](../src/graphics/particles.cpp), `0x4720d0`, `0x472430`, `0x472530`, `0x4739b0`, `0x473b50`, `0x474580`, `0x4745e0`, `0x474720`, `0x474760`, `0x474880`; [nano emission](../src/graphics/particles_473d50.cpp), `0x473d50`; [smoke drawing](../src/graphics/particles_474b80.cpp).

## 9. Visibility, sonar, radar, and jamming

### 9.1 Four questions must remain separate

1. Is the map area explored or currently in line of sight?
2. Does the local sensor update mark the target with radar or sonar detection?
3. Does the target satisfy full model-visibility rules?
4. Does a weapon and its target-acquisition path permit an attack?

A sensor contact is not automatically a fully drawn model or a valid target for every weapon. The water rendering pass also consumes sonar state, adding another visible consequence of detection.

### 9.2 Sensor classification uses geometry

The detection visitor skips empty units, the local player's own units, and units carrying a particular definition exclusion bit. It calculates squared horizontal distance and applies independent tests:

- Sonar marks a target when its origin is at or below sea level and its horizontal distance is strictly within sonar range.
- Radar marks a target when its model top reaches or exceeds sea level and its horizontal distance is strictly within radar range.

A surface vessel with an underwater origin and an above-water superstructure can satisfy both. A fully submerged unit can fail the radar geometry test while satisfying sonar. These are geometric conditions, not a comparison against a category string named submarine.

### 9.3 Active sensors and jammers

The sensor update initializes local/qualified shared units' detection state, clears other detection bits, then visits active eligible sensor emitters. Sonar range is squared directly. The radar distance calculation includes a contribution from the emitter's whole Y coordinate, while the visitor enumeration radius uses the larger configured radar/sonar range. Therefore, an increased computed radar threshold should not be read in isolation as an unlimited enumeration radius.

Jammer passes occur after detection. Radar jamming clears runtime `0x100`; sonar jamming clears `0x200`; both set `0x400`. Their emitters require the active state and the ownership conditions in this local update. Later visual-sight handling can restore radar-style visibility flags where sight permits, so a jammer is not a universal invisibility switch.

### 9.4 Full unit visibility

The model-visibility predicate immediately accepts a unit owned by the querying player. For other units it rejects the relevant cloak/hidden activation state. It then constructs sample points from the model bounds.

Without the sonar flag, a model whose top is strictly below sea level is rejected before normal visibility sampling. With the necessary underwater detection, the function still performs its explored/current-sight sample tests. The samples use model corners rather than simply checking the origin once.

The strict versus inclusive comparisons differ between these systems. Radar accepts a top exactly at sea level, while ordinary nonwater weapon reachability rejects a top at or below the sea plane. A boundary contact can therefore be detectable yet not reachable by that weapon check.

### 9.5 Target lists have their own maintenance

The enemy-radius query searches the player's maintained visible-unit list and filters by horizontal distance and readiness. A special fallback can search the known-unit list if no visible candidates were found. Player-AI list refresh is periodically scheduled, rather than being implied to rebuild synchronously for every gun every frame.

This prevents a simplistic equivalence between a minimap contact, this frame's local detection bits, the maintained target list, and a gun's current target reference.

Evidence: [sensor update](../src/ingame/info_panel_467440.cpp), `0x467440`; [detection and jammers](../src/ingame/info_panel.cpp), `0x467840`, `0x467960`, `0x467980`; [full unit visibility](../src/game/players_465ac0.cpp), `0x465ac0`; [enemy lists and radius queries](../src/ai/ai_player.cpp), `0x40ad80`, `0x40b2c0`; [sensor range overlays](../src/ingame/markers.cpp), [minimap ranges](../src/map/radar.cpp).

## 10. What can shoot at what

### 10.1 The attack pipeline

An attack requires compatible order routing, target acquisition or an explicit target, a working weapon slot, geometric eligibility, aim readiness where applicable, a clear reload state, ammunition/resources, a successful firing handler, and ultimately projectile contact or blast proximity.

These checks answer different questions. A weapon's reachability result is not a guarantee that the projectile will hit. A target category preference is not a ballistic trajectory solver. A legal manual coordinate order is not proof of successful automatic unit acquisition.

### 10.2 Ordinary nonwater weapons

For a unit target, the shared reachability routine rejects an ordinary weapon when either the shooter's model top or the target's model top is at or below sea level. It then applies the air-only restriction, optional ballistic solution test, and horizontal range check.

Thus a gun can attack a qualifying surface hull or shore structure whose top is above water. It cannot acquire a fully submerged model through this unit-target reachability routine merely because sonar reveals it.

`toairweapon` rejects targets whose current low movement-state bits are not the air state 2. This tests runtime state in this routine, not only the definition's `canfly` flag.

### 10.3 Water weapons

The water-weapon branch instead applies these rules:

- A target without `floater` is rejected if its origin is above sea level.
- A hover target is additionally rejected if its origin plus half its model-top height is above sea level.
- If those tests pass, horizontal range decides reachability in this routine.

This branch returns before the ordinary air-only and ballistic checks. Combining unusual weapon flags can therefore produce behavior that cannot be predicted by treating every flag as an independent universal veto.

The important surface-vessel exception is `floater`: an origin above the plane does not trigger the first rejection for floaters. Hover handling is a separate geometric rule. “Torpedoes cannot hit hovercraft” is too broad as an engine statement; the exact half-height condition is what this routine implements.

### 10.4 Conditional target matrix

Assume a present weapon, valid target reference, adequate range, and all additional order, visibility, aim, and firing requirements. “Eligible” below refers only to the stated unit-target reachability stage.

| Target state | Ordinary nonwater weapon | Nonwater `toairweapon` | Water weapon |
| --- | --- | --- | --- |
| Surface floater, model top above sea | Eligible if shooter top is also above sea | Rejected unless target is in air state | Eligible, subject to hover test if also flagged hover |
| Fully submerged nonhover unit | Rejected | Rejected by ordinary submerged test | Eligible if origin is not above sea |
| Ground unit standing above sea | Eligible if shooter top is above sea | Rejected outside air state | Rejected when nonfloater origin is above sea |
| Seabed amphibious unit fully underwater | Rejected | Rejected | Eligible under origin/hover checks |
| Hover unit over water | Ordinary top and range tests | Air-state restriction still applies | Rejected when origin plus half model-top height exceeds sea |
| Airborne nonfloater with origin above sea | Eligible for nonair-only weapons if other tests permit | Eligible in air state | Rejected by above-sea origin test |
| Underwater structure with origin below sea and top above sea | Eligible if shooter top is above sea | Rejected outside air state | Eligible under origin/hover checks |
| Model top exactly at sea | Rejected by ordinary top test | Rejected by ordinary top test | Uses water-branch conditions instead |

Eligibility says nothing about damage overrides or whether a ship actually carries that weapon. Named-unit matchups require the loaded loadout.

### 10.5 Automatic selection and bad-target preferences

The automatic selector gets candidate enemies within weapon range or the unit's search range, then examines at most 50 candidates in randomized order. It rejects unsuitable lifecycle/status cases and normally calls unit-target reachability. A kamikaze definition branch can bypass that reachability condition in this selector.

It maintains a preferred result and a fallback result. Membership in the weapon's bad-target category sends the candidate to the fallback group. A preferred result wins if one exists. Within each group, the comparison uses a randomized value based on squared horizontal distance, so the winner is not guaranteed to be the strictly nearest candidate.

`noChaseCategory` participates when the selector is used in its non-weapon-range search mode. Retaliation and already assigned orders have additional category checks. Do not reinterpret a bad-target category as “can never damage this unit.”

### 10.6 Manual attacks and coordinate attacks

Order routing distinguishes aircraft, mobile chasing attackers, stationary attackers, air strike paths, and underwater compatibility. Its underwater target test considers model-top position and whether the primary or another available slot is a water weapon.

A coordinate target is converted to a position whose Y is the higher of ground and sea level. The position reachability routine checks horizontal range first, then returns success immediately for water weapons. It does not apply the unit-target branch's target floater/hover classification because there is no target unit to inspect.

That difference explains why a coordinate can be accepted while automatic acquisition of a nearby unit fails. It does not make the coordinate shot ignore seabed, terrain, projectile collision, or weapon lifetime.

Evidence: [reachability](../src/weapons/weapons.cpp), `0x49aa80`, `0x49abb0`; [automatic selection](../src/ai/ai_player.cpp), `0x40b7b0`; [order dispatch](../src/orders/order_dispatch.cpp), `0x43f0e0`; [retaliation](../src/orders/unit_orders_406300.cpp); [target position resolution](../src/units/unit_targets.cpp), `0x48a1e0`; [category loading](../src/units/unit_types.cpp).

## 11. Aiming, firing, and projectile motion

### 11.1 Three independent weapon slots

The unit definition resolves `weapon1`, `weapon2`, and `weapon3`, and the runtime has three corresponding slots. Slots keep their own target, aiming state, reload counter, and stockpile count where relevant. One ship can therefore combine weapons with different naval target permissions.

The common update decrements reload counters, resolves the target position, starts script aiming if required, checks position reachability and resources, invokes the selected firing routine, and charges ammunition/resources only after successful firing.

### 11.2 Script aiming and muzzle positions

Turret weapons compute a desired heading and pitch. Ballistic weapons use a launch-angle solution; line-of-sight weapons use the aim-angle calculation. The engine starts the appropriate aim script with a callback, while the firing routines apply their tolerance/readiness conditions. Muzzle and aiming positions are obtained from script-selected model pieces.

The hull's heading, turret piece animation, muzzle location, and target sweet spot can all differ. A turret that appears pointed toward a ship may still be outside tolerance or waiting on its script completion.

Unit targets use `GetSweetSpot`; coordinate targets use the ground/sea position described above. A veteran-related path can lead moving unit targets using their motion, distance, and a projectile parameter when its conditions hold. It is not a universal perfect intercept calculation for every shot.

### 11.3 Reload and resources

Nonstockpile firing checks available energy and metal. On success, it spends the per-shot costs. The reload interval is adjusted by current health and a kill-count bonus. The kill contribution uses one point per five kills, capped at five points, and reduces the relevant percentage by six per point. Health contributes a separate factor.

Stockpile weapons require built ammunition and decrement it on successful fire. Their production uses a distinct weapon-building order, rather than constructing an attached ship hull. A ship carrying stockpile weapons is therefore governed by both unit construction and ammunition construction systems.

### 11.4 Water entry and torpedo-like motion

For a live self-propelled water weapon at or above sea level, the updater applies falling behavior and resets pitch. Once below the plane, it follows the propelled branch: speed approaches configured maximum by acceleration, optional guidance selects an aim point, turn limits adjust orientation, and velocity is reconstructed from heading, pitch, and speed.

This supports a projectile dropped from above the surface before underwater propulsion. The actual configuration still requires `waterweapon`, self-propulsion, and suitable launch flags and values. Merely naming a weapon torpedo does not select this behavior.

Two-phase weapons change their seeking/lifetime state according to the relevant flags. When a propulsion lifetime ends, `burnblow` can detonate; otherwise falling and phase-transition behavior applies. Tracking and guidance have separate roles in retaining target references and steering.

### 11.5 Other trajectories

Line-of-sight projectiles advance by velocity until their lifetime ends, with beam weapons maintaining additional start/end behavior. Ballistic and dropped projectiles add velocity and wind displacement and reduce vertical velocity by gravity. Ballistic lifetime handling can detonate or emit smoke and remove the projectile. Propeller animation increments a projectile rotation field; it does not make the host ship move.

Burst scheduling can copy a projectile template into later shots and refresh muzzle position under its timing conditions. A moving vessel's later burst shots can therefore be emitted from updated muzzle positions rather than the first shot's original world point.

### 11.6 Surface crossing creates a separate effect

After movement and collision, a still-live projectile crossing from strictly above sea level to at-or-below it can request the weapon's splash sequence. This requires an actual water cell with its high terrain bound below sea level and a map that permits sea-level effects.

A water-entry splash is separate from a later underwater impact. The same projectile can cross the surface, continue underwater if its flags permit, and detonate later.

Evidence: [weapon update and firing](../src/weapons/weapons.cpp), `0x49d580`, `0x49d880`, `0x49e010`, `0x49e1a0`; [aim target and lead](../src/units/unit_targets.cpp), `0x48a1e0`; [projectile update](../src/weapons/weapons_49b720.cpp), `0x49b720`; [weapon data conversions](../src/weapons/weapon_types.cpp).

## 12. Impacts, damage, and destruction

### 12.1 Collision uses cells and vertical bounds

Projectile collision first resolves the cell at its current position. Leaving the map marks it dead and handles camera-follow state. A projectile can also detonate near an intercepted projectile.

The cell holds two unit references. The first collision branch tests a foreign owner's unit against its upper bound; the second tests a full lower-to-upper vertical interval. This asymmetry is part of the current implementation and should not be silently generalized into one perfect 3D hull collision test.

The owner test compares player indices, not alliance relationships. Area damage has its own rules. Consequently, target selection, direct projectile interception by another unit, and friendly blast damage are separate mechanisms.

### 12.2 Sea level is a collision boundary for ordinary shots

After unit checks, `unitsonly` returns before the feature and terrain/sea checks. Otherwise the routine resolves blocking features and tests ground height. A ground-bouncing weapon can reverse and reduce vertical velocity at a ground contact.

For a projectile not yet below terrain, `waterweapon` permits continued travel without the ordinary sea-level detonation. Nonwater shots remain alive above the sea plane but ordinarily detonate after entering below it. A map flag can suppress that sea-level trigger.

Therefore, ordinary shots normally striking the water do not become torpedoes. Water weapons still interact with seabed and features; bypassing the sea surface is not bypassing terrain.

### 12.3 Water-impact art versus hull-impact art

The detonation routine classifies the cell as water from its terrain height relative to sea level. If the location is water and no direct unit target was supplied, it plays `soundwater` and uses the water explosion sequence. With a direct unit hit, it uses the ordinary hit sound and ordinary explosion/no-explosion path instead.

This explains why hitting a ship can look different from missing into the water immediately beside it. A geographic water location alone does not force the water-impact branch for a direct hull hit.

The no-sea-level-trigger map condition can remove a water detonation without a unit target before the normal effects and damage path. Explosions and surface-entry effects are therefore also map-dependent.

### 12.4 Direct and area damage

Weapon damage begins with default damage, then looks for a case-insensitive unit-name override. It applies the incoming scale, a capped kill-based attacker bonus, optional global damage scaling, and then dispatches normal or paralyzer damage. A unit-specific damage override can make a weapon weak against a hull even though targeting and collision succeed.

For small area-of-effect values, a direct unit hit uses the direct-damage path. Otherwise the area routine scans nearby cells and deduplicates units. It calculates distance from the explosion to the target's axis-aligned bounding box, including Y, rather than simply measuring center-to-center horizontal distance.

The effective unit blast radius in that routine is half the loaded area-of-effect value. Within it, falloff is quadratic between full center damage and `edgeeffectiveness` near the edge. If the explosion lies inside the box, its distance is zero and the scale is full.

The shown area scan does not apply the weapon acquisition water/air filters again or universally exempt allies. It excludes the attacking unit and tracks friendly/enemy damage separately. This is why “cannot select that unit” does not imply “cannot damage it with a nearby explosion.” Vertical separation still affects blast reach through bounding-box distance.

### 12.5 Construction hulls and damage

An unfinished hull accumulates health as progress is accepted, but it remains a runtime unit. Damage processing and construction progress can both alter its health. Its wireframe opacity is driven by remaining build fraction rather than current hit-point percentage. A heavily damaged almost-complete hull can therefore look nearly materialized.

### 12.6 Destruction has several outputs

The loaded definition selects `explodeas`, `selfdestructas`, and a corpse feature. Unit death can involve the COB death behavior, exploded model pieces, generic debris, an explosion weapon, and a wreck feature. These outputs should not be conflated into one universal sinking animation.

Generic debris moves and falls under gravity. At water contact, eligible pieces request water or lava splash art according to map settings and are removed. Exploded-piece updates have their own timer and water-plane removal path.

Moving feature spots behave differently: if above the seabed but below sea level, they zero horizontal velocity and descend at a fixed vertical step of `0x2CCC` fixed-point units, approximately 0.175 world units per update. At ground contact they stop on terrain. This is a recoverable mechanism for sinking moving features, not proof that every ship corpse uses it or has the same appearance.

Evidence: [collision, detonation, damage](../src/weapons/weapons.cpp), `0x49b090`, `0x499eb0`, `0x499cd0`; [area damage](../src/weapons/weapons_49a120.cpp), `0x49a120`; [unit damage application](../src/units/unit_targets.cpp), `0x489bb0`, `0x489ce0`; [debris and pieces](../src/weapons/explosions.cpp), `0x420f30`, `0x4213b0`; [moving features](../src/map/features.cpp); [death definitions](../src/units/unit_types.cpp).

## 13. Other unit interactions

### 13.1 Repair across the surface boundary

The repair capability check requires a target, the repair permission, a target not already at maximum health, and a target outside the excluded air state. It then applies a water reach condition.

For a flying repairer without amphibious capability, a target whose model top is strictly below sea level is rejected. For a nonflying repairer, the target's model top must not be below sea level minus the repairer's maximum water depth.

Thus repair of submerged units is constrained by repairer capability and target geometry, not just by alliance and horizontal build range. The order still has to approach and service the target. The capability predicate alone does not establish a completed repair action.

### 13.2 Reclaim

The unit reclaim predicate requires reclaim permission, rejects the excluded runtime air state, and excludes the relevant target capability flag. Reclaim orders additionally handle approach, work state, and progress/effects. Unfinished construction can also be dismantled through negative build progress, which has its own metal-credit and destruction behavior.

Wreck reclamation operates on features rather than the original live vessel. A shipwreck's blocking, resource content, model, and reclaim behavior require its feature definition. The ship's original weapon and sensor flags no longer describe that feature.

### 13.3 Transport eligibility

The load predicate checks transport permission, capacity count, the target's movement object, target footprint width versus the carrier's capacity field, forbidden-transport definition flag, runtime mode, water conditions, and exact completion.

For a nonflying carrier, a target with nonnegative minimum water depth is rejected. All targets whose model top is at or below sea level are rejected by this predicate. A target with any nonzero remaining build fraction is also rejected.

The result is more specific than “boats can carry boats” or “naval units cannot be transported.” Exact permitted cargo depends on data, current geometry, and carrier type. The common attachment mechanism used inside shipyards does not mean that a normal transport can load an unfinished hull: normal loading has these extra eligibility checks.

### 13.4 Guarding, patrol, and retaliation

Guard and patrol orders reuse common attack, repair, target-search, and approach routines. Stationary and moving units receive different order routes. An armed ship guarding another unit is still limited by its own weapons' target eligibility; a repair-capable ship on repair patrol is still limited by repair reach and build stance.

Retaliation examines standing orders, category restrictions, and weapon reach. A ship need not answer an attack with every slot, and an attacker that can hurt a submarine with blast damage does not necessarily become an eligible unit target for all of that submarine's weapons.

### 13.5 Hazardous water

The main unit update applies configured map water damage once every 30 ticks when water damage is enabled and nonzero, the unit's origin is at or below sea level, and it lacks the hover bit. The tested exemption is `canhover`, not `floater`. Ownership/authority conditions also gate this processing.

This is a precise engine rule. Do not assume every water-capable vessel is immune to every map's hazardous liquid. The map settings and actual unit flags are required to establish a particular outcome.

### 13.6 Economy and AI roles

Naval construction shares the same resource accounting as other production. FBI fields also include tidal generation, storage, normal energy/metal production, and usage. The subsequent [general economy trace](systems/03-economy.md) establishes that the active tidal branch credits map tidal strength multiplied by the unit's rating, with the applicable income scaling. Map loading supplies nonnegative mission tidal strength or a default of 0.5. Exact output for a named generator still requires its loaded definition and the relevant update cadence.

AI grouping includes a branch placing qualifying ungrouped units with positive minimum water depth into a naval-associated squad slot. Builder and aircraft branches precede it. A construction vessel can therefore be grouped as a builder rather than by the water-depth branch. AI grouping is not the authoritative weapon-type system.

Evidence: [repair, reclaim, and loading predicates](../src/units/unit.cpp), `0x489960`, `0x4899b0`, `0x489a90`; [guard and patrol orders](../src/orders/unit_orders.cpp); [retaliation](../src/orders/unit_orders_406300.cpp); [hazardous water](../src/units/unit_position.cpp), `0x48ad30`; [resource accounting](../src/game/economy.cpp); [AI squad assignment](../src/ai/ai_player.cpp), `0x408830`.

## 14. Multiplayer and persistence

### 14.1 Construction state is transmitted

The unit state reader restores health and an eight-bit construction value scaled back into remaining build fraction. The writer reserves zero for exact completion and encodes nonzero remaining progress into values starting at one, using a scale of 254 and integer truncation. A changed fraction marks drawing state dirty. It also reads activation state and can apply an attachment record identifying the parent and piece.

This means another machine can receive both that a hull is partially constructed and where it is attached. Construction appearance then uses local rendering and tick-dependent palette phases. The network representation's quantization should not be confused with the local floating-point resource calculation.

### 14.2 Attachment and completion are explicit events

The attachment path uses unit IDs, a piece parameter, and a small mode value. Completion can broadcast a builder/product link, and network packet handling routes that link into the common completion routine. Unit references and attachment lists must remain consistent when products finish, are destroyed, or detach.

### 14.3 Weapons and damage have authority rules

Weapon fire and projectile hits have packet representations and handlers. Damage and area damage contain ownership/player-type conditions to avoid treating every machine's visual projectile as an independent authoritative hit.

The local script-effect dispatcher is visibility-gated. A missing cosmetic wake on one client is therefore not sufficient evidence of a different authoritative vessel position or damage result. Conversely, matching visuals do not establish synchronized gameplay state.

### 14.4 Saved state

The repository has unit save/load, script-state, motion-state, order-state, and per-unit resource account persistence. Resource accounts are saved under a unit-ID-based entry and restored when present. Motion persistence records velocity, an auxiliary motion vector, speed, turn amount, a path-lock timestamp, mode, and a flag in a unit-ID-based motion entry. Restoring motion is therefore more than restoring the vessel's position alone. A detailed save-file schema for every naval-related field would require following the remaining serializers field by field; it should not be inferred from the network packet layout.

Evidence: [unit state reader/writer and attachment](../src/units/unit_position.cpp), `0x48b200` through `0x48b920`; [completion packet handling](../src/network/net_game_453d40.cpp); [weapon event handlers](../src/weapons/weapons.cpp), `0x49af90`, `0x49d270`; [unit persistence](../src/units/unit_save.cpp); [resource account persistence](../src/game/economy.cpp), `0x4010b0`, `0x401110`.

## 15. Worked traces and diagnostic guide

### 15.1 A surface ship completes in a yard

The yard's build stance becomes ready. `QueryBuildInfo` selects a piece; placement checks the product footprint. Creation makes an unfinished unit and attaches it to that piece. Accepted work reduces remaining fraction and adds health, with nano emission requests. The renderer shades the child's actual model and merges it with the yard using depth.

The child's floater flag and draft can raise its attached position to its sea-level floor. After composition, underwater portions can be tinted or removed according to the observer's state. When remaining fraction reaches exactly zero, completion clears cached drawing state, detaches the qualifying product, and the product takes inherited move/patrol orders or parks. Its subsequent path must still satisfy depth and occupancy.

### 15.2 A gunship or shore cannon sees a submarine

Sonar can mark a fully submerged unit, and full visibility has its own sampling requirements. An ordinary nonwater weapon's unit reachability rejects a target whose model top is at or below sea level. The contact therefore does not authorize that gun to acquire it.

A water-capable slot can follow the water branch instead. Whether the unit actually has one, which slot is chosen by order dispatch, and whether its script and firing handler can launch it remain necessary checks. A nearby blast has its own three-dimensional radius calculation and may damage a target that ordinary direct acquisition rejected.

### 15.3 A torpedo-like projectile is dropped from an aircraft

The launch handler creates the configured water/self-propelled projectile. While at or above sea level, its water propulsion branch applies falling behavior. Crossing into actual water can request entry splash art. Below the plane, propulsion and optional guidance take over. Collision still checks units, features, and seabed. A later hull hit can use ordinary hit art, while an unassociated water detonation can use water art and sound.

This trace requires the relevant flags. It is not a claim that all aircraft bombs have underwater propulsion.

### 15.4 An amphibious unit enters water

Its route remains governed by its movement class and footprint. Its height path determines whether it follows terrain or a hover/floater floor. If its origin is below sea and it lacks both hover and floater flags, the common velocity cap is halved. As its top crosses the sea plane, ordinary weapon eligibility and radar classification can change at their respective boundaries. The script can receive a new occupancy value and change its effects.

### 15.5 Symptom-to-cause table

| Symptom | First state or data to inspect | Why |
| --- | --- | --- |
| Yard active but no hull appears | Build stance, `QueryBuildInfo`, placement, unit creation failure | Production may not have reached the work loop |
| Hull pulses but progress stalls | Remaining build fraction and resource-account stall state | Palette motion continues with game ticks |
| One hull appears lower than another | Product waterline, floater flag, model origin, parent piece transform | They establish different geometry relative to sea |
| Wireframes differ at the same percentage | Model vertex heights, topology, piece visibility, integer A | The sweep is not normalized to each hull's height |
| Hull partly hidden by yard | Composite depth, child offsets, foreground yard pieces | Parent/child composition is depth-tested |
| Enemy underwater hull disappears | Sonar bit, full visibility, composite cutoff | Water pixels can be cut instead of tinted |
| Completed ship remains near yard | Inherited orders or `PARK`, yard-open result, obstruction/depth | Completion does not guarantee a free path |
| Yard door opens but exit remains blocked | Yard-open state and occupancy/pass-map refresh | Animation and navigational state are separate |
| Ship cannot use a narrow channel | Full footprint, minimum depth, per-cell extrema | Center-point water depth is insufficient |
| Slow submerged movement | Hover/floater flags, depth of origin, pitch cap | Common mover can halve underwater speed |
| Wake stops at shore | Terrain height beneath each wake particle | Wake expiry tests seabed versus sea level |
| Moving ship has no wake | COB emission requests, local visibility, particle lifetime | Movement callbacks do not themselves guarantee emissions |
| Sonar contact is not shot at | Weapon loadout, unit reachability, target lists, categories, aim | Detection is one stage of the pipeline |
| A bad target still gets shot | Preferred group empty, explicit orders, retaliation | Automatic bad-target classification permits fallback |
| Coordinate attack works but unit attack does not | Position versus unit reachability branches | Water target classification is absent for coordinates |
| Shot splashes before reaching submarine | Water-weapon flag, trajectory, sea collision trigger | Ordinary shots do not automatically continue underwater |
| Hull hit and near miss use different effects | Direct-unit argument and water-cell classification | Detonation selects different art/sound branches |
| Allied ship takes blast damage | Area scan and bounding-box distance | Acquisition alliance rules are not a universal blast veto |
| Transport refuses a ship | Transport flags, width/capacity, minimum depth, top height, completion | Loading has explicit naval and construction exclusions |
| Underwater repair fails | Repairer flight/amphibious/depth capabilities and target top | Repair has separate reach rules |

These checks describe where to investigate. They are not assertions that any particular observed failure has already been reproduced.

## 16. Source index and remaining research

### 16.1 Primary source map

| Subject | Files and anchors |
| --- | --- |
| Definitions and asset resolution | [unit_types.cpp](../src/units/unit_types.cpp), [unit_def.h](../src/units/unit_def.h), [weapon_types.cpp](../src/weapons/weapon_types.cpp), [weapon_def.h](../src/weapons/weapon_def.h) |
| Depth, footprint, and yard occupancy | [terrain.cpp](../src/map/terrain.cpp), [terrain_47e2d0.cpp](../src/map/terrain_47e2d0.cpp), [movement_class.cpp](../src/units/movement_class.cpp) |
| Production and release | [unit_orders.cpp](../src/orders/unit_orders.cpp), [unit_orders_403a20.cpp](../src/orders/unit_orders_403a20.cpp), [control_panel.cpp](../src/ingame/control_panel.cpp) |
| Construction image and edges | [model_render.cpp](../src/graphics/model_render.cpp), [model_render_458fa0.cpp](../src/graphics/model_render_458fa0.cpp), [draw.cpp](../src/graphics/draw.cpp) |
| Shipyard child composition and water | [model_render_4589c0.cpp](../src/graphics/model_render_4589c0.cpp), [gaf.cpp](../src/graphics/gaf.cpp) |
| Motion and height | [order_list.cpp](../src/orders/order_list.cpp), [pathfind.cpp](../src/ai/pathfind.cpp), [unit_position.cpp](../src/units/unit_position.cpp) |
| COB interfaces and effects | [unit_script.cpp](../src/units/unit_script.cpp), [unit_scripts.cpp](../src/units/unit_scripts.cpp), [particles.cpp](../src/graphics/particles.cpp) |
| Sensors and model visibility | [info_panel_467440.cpp](../src/ingame/info_panel_467440.cpp), [info_panel.cpp](../src/ingame/info_panel.cpp), [players_465ac0.cpp](../src/game/players_465ac0.cpp) |
| Acquisition and order routing | [ai_player.cpp](../src/ai/ai_player.cpp), [order_dispatch.cpp](../src/orders/order_dispatch.cpp), [unit_targets.cpp](../src/units/unit_targets.cpp) |
| Weapon simulation and blast damage | [weapons.cpp](../src/weapons/weapons.cpp), [weapons_49b720.cpp](../src/weapons/weapons_49b720.cpp), [weapons_49a120.cpp](../src/weapons/weapons_49a120.cpp) |
| Death, pieces, and wreck movement | [explosions.cpp](../src/weapons/explosions.cpp), [features.cpp](../src/map/features.cpp) |
| Repair, reclaim, and transport gates | [unit.cpp](../src/units/unit.cpp) |
| Accounts, synchronization, saves | [economy.cpp](../src/game/economy.cpp), [unit_position.cpp](../src/units/unit_position.cpp), [unit_save.cpp](../src/units/unit_save.cpp), [net_game_453d40.cpp](../src/network/net_game_453d40.cpp) |

### 16.2 What needs asset inspection or further tracing

The following are explicitly unestablished here and must not be filled in from unit names or expectations:

- A complete named ship/shipyard/submarine roster, faction-specific build trees, costs, movement values, sensor ranges, damage overrides, and weapon matchups.
- Every stock shipyard's `QueryBuildInfo` result, piece hierarchy, door timing, build stance, emitter positions, and launch animation.
- Every product's waterline, origin, hidden pieces, topology, and exact appearance at each progress boundary.
- The exact COB wake/bubble/smoke schedules of each named hull and their relationship to moving, health, and occupancy states.
- Asset-specific death scripts, wreck creation choices, feature metal values, and whether a particular ship's wreck enters the moving-feature sinking path.
- A complete interpretation of each yard-map character's occupancy semantics across all consumers. The loader's distinct masks are established, but a full character reference requires tracing each mask consumer.
- All save-file fields, all AI strategic naval-build decisions, and asset-specific economic outputs. The shared tidal-product and account-allocation calculations are now covered in the [economy chapter](systems/03-economy.md).
- Exhaustive interceptor/antiweapon behavior, targeting-upgrade behavior, cloaking economy, transport unloading and beach selection, and visibility-sharing configurations beyond the branches described above.
- Pixel-perfect visual confirmation of the construction stages, clipping boundaries, palette appearance, shadows, and particular model combinations in an actual game session.

Those limits do not reduce the shared mechanisms to guesswork. They identify the additional evidence needed to turn this engine reference into an exhaustive asset-by-asset naval catalogue. The documented condition checks should be retained when adding that catalogue, since surface position, model geometry, weapon flags, script state, and observer state can change an interaction even for the same named pair of units.
