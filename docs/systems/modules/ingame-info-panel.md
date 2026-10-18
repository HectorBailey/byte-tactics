# Sensor contacts, battle composition and information panels

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md) | [Minimap](map-radar.md) | [Sight](map-line-of-sight.md)

Module group: `ingame/info_panel`. Sources: [panels and drawing helpers](../../../src/ingame/info_panel.cpp), [sensor update](../../../src/ingame/info_panel_467440.cpp), [battle composition](../../../src/ingame/info_panel_468cf0.cpp), [feature drawing](../../../src/ingame/info_panel_46a610.cpp) and [unit/feature information](../../../src/ingame/info_panel_46a860.cpp). It owns the interval `0x467440` through `0x46bc70`, including sensor contacts beyond its UI-oriented name.

## 1. Contact flags and observation context

UpdateSensorRadarAndCloak (`0x467440`) returns when numPlayers is below two. Otherwise it processes the current displayed player, not every possible viewer simultaneously. Runtime unit bits 0x100 and 0x200 are radar/surface and sonar contact channels; 0x400 marks a jammer-affected result and 0x1000 a proximity cloak obstruction. Full visual identification uses a different [player visibility service](game-players.md).

The first pass visits live-bit units, clears proximity flag 0x1000 and grants both contact bits to own units, qualifying shared-allied units or a displayed-player visibility override. Shared qualification uses the unit owner's alliance table toward the viewer and owner PlayerInfo flag +0x97 bit 6. Viewer override requires active player and PlayerInfo +0x9b bit 6. Other units clear 0x700. The granting branch ORs contact bits without clearing a preexisting 0x400, an asymmetry relevant to interpreting that marker alone.

These packed fields are a transient result for the current observer. Their absence/presence does not by itself mean a unit can be selected, identified, shot, captured or hit by a projectile.

## 2. Radar and sonar emission

The second pass visits the displayed player's own units. A source needs live bit, absence of runtime 0x4000 and activation bit 0. With nonzero radar or sonar definition range it performs a spatial visit using the larger **unboosted** configured range.

DetectionVisitor (`0x467840`) ignores empty slots, viewer-owned targets and definition stealth bit 8. Horizontal distance uses fixed-coordinate squared products shifted to world-unit scale. Sonar contact requires target world Y at/below sea and distance strictly below sonar-range squared. Radar contact requires target world Y plus model maximum Y at/above sea and distance strictly below the source's adjusted radar-range squared.

Adjusted radar radius is configured range plus twice source whole altitude, then squared. Sonar radius is configured sonar only. Broad spatial visitation still uses max(configured radar, configured sonar), so that altitude adjustment does not independently expand the visitor's outer collection radius. The terrain/spatial helper's own candidate policy also matters.

A partially submerged ship can satisfy both height tests. A fully submerged model can satisfy sonar while failing radar. Stealth suppresses both branches of this visitor; it is not merely a final minimap-icon style. Exact sea equality is included in the height tests while distance equality is excluded.

## 3. Jamming, cloak obstruction and direct sight

The next pass considers live, active sources whose owner differs from the viewer. It visits radar-jammer and sonar-jammer radii separately. These source tests do not repeat the earlier 0x4000 exclusion or add an alliance exclusion. RadarJamVisitor (`0x467960`) clears 0x100 and sets 0x400 on every visited unit. SonarJamVisitor (`0x467980`) clears 0x200 and sets 0x400. The callbacks themselves add no owner/stealth/range checks; the spatial query supplies the radius.

A further pass handles live units with active local role 1/2 ownership and cloak capability flags2 0x2000. HasReadyUnitInRange at mincloakdistance can extend workTime to current tick plus 90 and set 0x1000. [Economy cloak processing](game-economy.md) consumes runtime state and work deadlines; this pass does not itself pay cloak energy or directly clear the cloak activation bit.

Finally, live units lacking radar bit 0x100 and lacking cloak activation bit 4 gain 0x100 when the viewer's projected sight sample passes. Dynamic sight uses current byte coverage; otherwise it uses permanent mapping bit. This visual restoration occurs **after jamming**, so jamming is not absolute invisibility to direct sight. It restores radar/surface bit only, not sonar bit, and does not clear the 0x400 marker.

DetectionVisitor itself does not reject cloak activation, whereas direct-sight restoration does. An unidentified contact and a fully drawn/identified enemy consequently have separate eligibility. [IsUnitVisibleToPlayer](game-players.md) grants own-unit visibility first, hides foreign cloaked units and needs sonar contact for fully submerged model bounds before its sight samples. Weapon targeting combines those services with [weapon reach](weapons-weapons.md).

## 4. Battle composition order

DrawBattleFrame (`0x468cf0`) binds the offscreen surface, hides software cursor, clips to the battle view and profiles phases. It draws terrain tiles/debug grid, resets clipping for resource panel, unit information and pending minimap, then restores battle clipping for world objects.

It builds per-row unit lists from the previously computed visible-unit IDs. Rows use whole Z relative to scroll divided by 16 plus a margin, rather than sorting every model polygon by exact depth. It scans a terrain rectangle with extra margins, drawing low features immediately and marking tall features for the later interleaved row pass. Feature flag 8 and stored owner nibble can require IsFootprintVisible before display.

Particle lists 0,1,2 draw before low features; lists 3,4 follow. The row pass draws runtime-mode-1 units and selection outlines when object drawing is enabled, then tall features for that row. Particle list 5 follows. With object drawing enabled it then draws list 6, projectiles, explosions, list 7 and units outside mode 1. Particle list 8 follows, then modifier-requested order overlays and optional own-unit health/group labels; list 9 is inside that enabled-object block.

Fog draws after those world layers when object drawing is enabled. Selection-drag/build-preview rectangles draw afterward, followed by score/status/network/messages, debug data, pause/endgame/hourglass/time indicators, menu layers, profiling bars and options scroll control. It restores software cursor and flips only when both caller presentation arguments are nonzero. Some particles, terrain and panel work still run when the first argument disables the full object/message/fog path.

This layer order explains why a surface effect, ship hull, flying model, fog edge or selection rectangle can overlap differently. A build-preview rectangle is an interface overlay, distinct from a product model's construction wireframe. Actual [model rendering](../../../src/graphics/model_render.cpp) consumes buildLeft and script-piece state separately.

## 5. Resource HUD values and smoothing

The battle renderer takes a resource snapshot for the displayed player. Amounts approach the integer target by one eighth of integer difference, with a minimum one-unit step in either direction, then cap above at storage. Fractional display amounts are discarded by that smoothing helper. Income/use values refresh when a per-player deadline is strictly below game tick, advancing the deadline by 30. The whole resource block is compared before redrawing.

Private snapshot fields called metal are used for the energy layout and vice versa in this source view. Actual accesses and side-layout rectangles establish the channel, consistent with other provisional naming mismatches. Energy shows integer amounts/rates with K suffix above 99999 income and a particular negative usage threshold; metal rates use one decimal and absolute usage. Positive sharing threshold below current amount adds a two-pixel threshold marker.

Storage greater than zero guards fill-ratio drawing. Side definitions provide font, rectangles, colors and logo. These numbers are presentation snapshots, not another economy settlement. A delayed/smoothed HUD does not mean a resource account received the displayed value on that exact draw call.

## 6. Unit and build-button information snapshot

DrawUnitInfoPanel (`0x46a860`) first handles a debug mode that fills the bottom panel with display/backbuffer, standing settings, pending simulation steps, tick, camera, unit and packet data. Ordinary mode constructs a zeroed snapshot of hovered gadget/unit/feature, health, kill count, order-name pointer, previous resource reports, selected reload timers and own-unit order target/health. Memcmp against the prior snapshot suppresses unchanged redraw.

Only weapons with reloadtime greater than 30 and enabled slot bit 1 contribute displayed timer fields; others use -1. Some visible decisions are downstream of a snapshot that caches pointers/values rather than every external option, so surrounding refresh invalidation is part of the UI contract.

A hovered build gadget resolves its name truncated to 16 characters. A valid definition other than CORBUILD displays title, integer metal/energy cost and description, then returns. This does not create an order or validate that a builder can reach a site.

## 7. Identified units, unknown contacts and feature text

For a hovered live unit, IsUnitVisibleToPlayer gates detailed identification. A visible multiplayer unit with showplayername or commander definition bits can show its owner's name instead of ordinary unit title. A health bar is shown for own units or definitions without hidedamage bit 14. Owner logo is drawn separately.

Own units, or the debug override, show nonnegative previous metal/energy production and usage with plus/minus formatting. Armed units with kills get singular/plural count and Veteran after four. Their translated current order name appears when present. A nonzero stockpile-build percentage takes priority over secondary target information and is shown only to the owning viewer. Otherwise an own-unit order target can show its title/health when live and visually identified, again respecting hidedamage.

An unidentifiable unit shows translated Unidentified object prefixed S when sonar bit 0x200 is set, otherwise R. The existence of a hover/contact ID does not authorize revealing its definition, health or production. Feature text is suppressed by feature flag 4 unless debug; it uses internal name in debug or description normally. Resource amounts appear only under the feature flag policy, and zero values omit their labels.

## 8. Feature GAF/model drawing

BlitFeatureGaf (`0x46a610`) centers by feature footprint, projects using the average of four terrain shade/height samples and selects placed versus shared animation paths. Placed state can draw an animated shadow under draw flag 0x10 and an animated sprite, or populate a reusable unit-like wrapper with feature model state/rotation/position and call DrawUnit.

Unplaced/shared definitions use sequence frames or frame zero according to their animation flag, with independent horizontal flips for shadow and sprite. Shadow drawing depends on global draw flag 0x10. The feature wrapper path does not create a real owner-controlled unit, queue or target reference.

## 9. Bars, outlines and model primitives

DrawProgressBar (`0x467b60`) clamps value to 0..max, offsets the rectangle and fills full/empty colors; max zero is not locally protected before division. DrawPercentBar (`0x468310`) outlines then fills positive percentage capped to 100, mutating the supplied rectangle's right edge. DrawHitPointBar (`0x46a430`) skips nonpositive health, draws a 34-by-4 framed bar and scales 32 interior pixels by health/maxHealth. Colors change strictly above integer one-third/two-thirds thresholds; maxHealth zero is unguarded.

DrawRotatedQuadOutline (`0x467a50`) rotates four corners, projects X and inverted model Z with half-height displacement and draws four connecting lines. DrawSelectionBox (`0x46a530`) requires UI option bit 2, takes all four lower-Y corners of model bounds and uses unit angles/world position. It is a bounding-footprint outline rather than an exact visible silhouette.

DrawMapTileSelectionOutline (`0x46b9d0`) converts cell/footprint to a projected rectangle using terrain height and chosen palette index; kind 4 shrinks by one pixel. DrawClosedPolygon (`0x46ba80`) joins successive points then closes first-to-last, without a separate zero-count guard.

DrawModel3doProjected (`0x46bae0`) rotates/project vertices into shared scratch buffers. If firstFace is nonnegative it starts at primitive 1 rather than skipping an arbitrary indexed primitive. Solid-color primitives fill polygons; nonsolid primitives draw only quads using a direct texture or sequence reference. Asset counts/indices must fit shared buffers. It does not itself run a unit construction-state animation.

## 10. Light bars, status and messages

LoadLightBar (`0x4679a0`) clears resource/panel caches, finds LIGHTBAR frame 1 and clears its X/Y offsets. BlitGafFrameAtOffset (`0x467a20`) adds frame offsets to the supplied draw position. BlitSideLogoToRect (`0x467c00`) scales the owner-color logos32 image into a destination rectangle plus vertical offset. DrawLightBars (`0x467d70`) clears offscreen, draws side top/bottom/side panels and flips twice, with another side-panel draw between flips.

DrawStatusPanel (`0x4689c0`) rate-limits slide motion by wall-clock milliseconds plus 15. Space/GUI state selects movement toward 0 or -31 by about one third of remaining distance with minimum one, playing transition sounds. It returns without drawing at zero. The shifted panel reports simulation time using 30 ticks/second, local unit count/max and effective speed relative to normal 10, additionally showing requested speed when different.

MarkRecentlyDamaged (`0x467950`) sets timer byte +0xfa to 240. Minimap and other consumers decide how that timer blinks/decrements. ReportGameEventCallback (`0x46bc60`) forwards its integer. ShowGameMessage (`0x46bc70`) routes mode 0 through AddMessage with parameters 16,0,10 and a game flag; mode 1 uses OpenMessageBox with half screen width and returns its result; other modes return initial success without a message operation here.

## 11. Diagnostics and profile timing

DrawUnitStateProbe (`0x467e50`) displays selected probe ID, owner/controller role, build fraction, health, occupancy, automatic-target flags and primary/background order names, states and targets for local role 1/2 units. Invalid live/dead flags clear tracking but do not return immediately from that branch, so the same call can continue reading that slot. A three-name occupancy array is indexed by two runtime bits without a local guard against value 3.

DrawUnitBuilderProbe (`0x4685a0`) displays ownership and each build option's rating/probability. Its invalid-unit/builder test clears the **information** probe fields, not its own builder probe fields, and continues drawing. Both panels update a shared bottom coordinate so later overlays stack.

DrawNetworkStats (`0x468380`) formats sent/received byte rates divided by 1000 as K/s and scales its bars to full at **5600 bytes per rate unit**, as supplied by GetByteRates. The source comment's 56 K/s wording does not match that literal divisor. FrameTimers::AccumulateProfileTime (`0x46a400`) accumulates elapsed wall milliseconds into the supplied phase index and replaces its last timestamp. DrawProfileBarLine (`0x46b900`) displays selected accumulated time divided by total, with no local zero-total guard.

## 12. Lifetime and completeness

This module does not own an independent save/network format for contact bits or UI surfaces. Player/unit persistence, information sharing and simulation scheduling establish their lifecycle. Contact recomputation consumes activation, unit definitions, alliance/share flags, spatial queries and cloak state; battle drawing consumes its result plus the separate visibility service.

All 29 annotated functions and substantive inline render helpers are accounted for, including sensor/jammer/proximity passes, the layered battle frame, HUD snapshots, feature/model rendering and diagnostics. Static tracing identifies differing contact/identification policies, finite numeric/storage preconditions and diagnostic invalidation quirks. No builds or game execution were performed.
