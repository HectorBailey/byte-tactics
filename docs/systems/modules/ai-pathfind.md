# Route search, terrain knowledge and shared search budgets

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `ai/pathfind`. [Pathfinder implementation](../../../src/ai/pathfind.cpp) covers the interval from `0x40d5e0` up to the aircraft-order boundary at `0x40f200`. It owns the reusable search grid, node pool, preliminary obstacle probe, weighted search, route traceback and per-player scheduling. [Movement classes](units-movement-class.md) provide footprint-aware terrain grades; [order destinations](orders-order-targets.md) provide goals and consume waypoints; [steering](units-unit-scripts.md) actually moves units.

## 1. State and lifetime

One game-owned Pathfinder runs one active search at a time. It stores the requesting unit, its movement class, AiSearchGoal, destination, start/found cells, probe aim, search costs, heap and grid. Ten player cursors and budgets persist across searches. Create/destroy wrappers live in the order-targets module.

Construction (`0x40e9e0`) initializes empty pool/heap state and sizes the grid from map tile width/height. Cell count rounds upward to a multiple of eight. Each cell stores flags, incoming direction and a 16-bit node-pool index. A dirty bitmap tracks one bit per eight cells, grouped into 32-bit words covering 256 cells. The allocation is tagged AISearch touched mapentries. Initialization marks the allocated region for clearing, with a special final-word path. This assumes ordinary nonempty map dimensions; zero/tiny pathological sizes are not independently validated before unsigned indexing/allocation arithmetic.

The default scheduler allowance is 0x535, or 1333 accounting steps per invocation. Base heuristic scale is 0x18000, or 1.5 in 16.16. Per-player budgets start at zero, cursors begin from their player unit storage, and global scale entries start at the base. Destruction (`0x40eb30`) frees dirty bitmap, grid cells, node pool and heap pointers. It does not serialize an in-progress search.

## 2. What the search knows about a cell

GetCellState (`0x40d7b0`) first checks movement-class bounds. It converts the candidate origin into the coarser visibility grid using half cell coordinates plus a footprint-quarter offset. Outside visibility bounds returns blocked grade 0. If the player's visibility-mask bit is absent, it returns grade **2 without reading actual terrain grade**. Otherwise it returns the class's packed value.

| Result | Search interpretation |
| --- | --- |
| 0 | Blocked, unless a special already-visited probe condition applies during expansion |
| 1 | Traversable with an additional cost of 30 |
| 2 | Unknown-to-player terrain treated as traversable without that additional cost |
| 3 | Preferred traversable terrain without the additional cost |

Class generation establishes water-depth limits, slope, footprint clearance, features and occupancy. Search does not infer navigability from water appearance or model category. Unknown terrain can initially look cheaper than known grade-1 terrain. The visibility-mask consumer alone does not establish a new sensor modality; its caller supplies the game's existing knowledge mask.

This separation explains a ship initially routing toward an unexplored obstruction and revising later. It also prevents interpreting search results as permission to overlap live units: immediate collision and positioning checks are separate.

## 3. Starting and marking a destination

StartSearch (`0x40e630`) takes the requesting unit's motion movement class, captures its current cell and refreshes moved-unit occupancy. It resets movement/turn cost tables and clears the previous search's dirty cells. The destination fills a vector of goal cells. Every in-bounds returned cell gets goal flag 4, and the nearest candidate by squared cell distance becomes the probe's aim.

Goal enumeration and actual ContainsCell/IsGoal can differ. A circular destination reports one center while accepting a radius; a rectangle reports a border; ring conventions have the documented discrepancy. Search uses both enumeration and the destination's distance heuristic, rather than replacing all targets with a single clicked point.

If the current cell already satisfies IsGoal, it reports source-order flag 0x100, supplies zero waypoints and releases the requesting unit. An out-of-grid start reports 0x200 and also supplies zero. The destination heuristic is evaluated before that explicit start-bound check. A destination returning an empty goal vector does not establish a new aim in this loop; valid goal enumeration is a caller precondition for the probe.

The preliminary probe returns zero when it reaches a goal/zero-heuristic cell. That reports 0x100 but still seeds the heap for search and traceback. A positive result reports 0x200. If it has made no heuristic improvement over the start, the search finishes with no waypoints. These flags are probe/search-status notifications, not the later physical arrival flag 0x20.

After a usable probe, the heap resets, start enters with g=0, f=scaled heuristic and straight-step count 100. Initial direction rounds unit heading into eight sectors. The first expansion permits turns from -4 through +4. The scheduler later narrows to -2 through +2.

## 4. Preliminary obstacle-following probe

ProbeStraightPath (`0x40e160`) is not a geometric line-of-sight ray. It walks along X toward the chosen aim, then along Y. Each traversed cell stores its incoming direction and visited flag 8. It tracks the lowest scaled heuristic encountered and returns zero at a marked goal or when the best heuristic becomes zero.

On an obstacle it walks both outlines in opposite directions, turning through the eight direction choices to find passable neighbors. Either side can resume the original X-then-Y route when it reaches the helper's accepted line segments. A loop/side meeting or inability to progress returns the best heuristic so far. This is a heuristic-improvement probe, not a returned optimal route.

It counts its own greedy/outline iterations into scheduler steps. It does not check the scheduler's 100-step expansion chunk inside these loops. A long probe can therefore consume more than an ordinary expansion chunk before returning. Marked directions and visited cells remain inputs to the later search; the heap reset does not clear the probe grid again.

## 5. Expansion costs and accepted goals

InitCostTables (`0x40d880`) installs turn costs `{0,40,60,80,100,80,60,40}` and alternating cardinal/diagonal step costs 16/22. DirectionFromStep (`0x40d9c0`) maps eight axis/equal-diagonal sign combinations to sectors 0..7, returning -1 for zero or unequal diagonal deltas. It accepts multiples along those directions, not just unit-length steps.

Estimate (`0x40da40`) multiplies the destination's distance by heuristic scale and shifts by 16. Search's inline forms truncate that 64-bit result to integer as used in f-cost. Since base scale is already 1.5 and can rise, this is a weighted heuristic search. The implementation does not promise shortest-path optimality.

ExpandNeighbour (`0x40da70`) adds the selected direction delta to the current origin and checks bounds. Unseen cell state obtains the terrain grade. Blocked grade becomes closed/blocked unless the probe already marked it visited. A candidate's heuristic no greater than the probe's best value gains goal flag 4 even if it was not an originally enumerated goal. The search can consequently terminate at a best-reachable/probe-threshold location, not only at a destination's explicit center/border.

New g-cost adds parent g, turn cost, directional step cost and terrain penalty. Grade above 1 has zero penalty; grade 1 has 30. A nonstraight turn after fewer than five straight steps adds another 75, discouraging frequent bends. Straight steps increment the count; a turn resets it to one. f is g plus weighted heuristic.

An already-open cell changes only if new g is lower. It keeps its terrain penalty, updates incoming direction, f/g and straight-step count and sifts its node upward. Closed cells are not reopened by this branch. No separate diagonal corner-clearance check appears in expansion; it relies on class footprint/passability information rather than explicitly testing both adjacent cardinal cells on each diagonal.

ExpandBestNode (`0x40df00`) implements a standalone pop/goal/expand operation. RunSearches inlines an equivalent operation with its own accounting and direction-range update. Reaching flag 4 records found; otherwise the cell closes with state 2 and candidate directions expand. The first -4..+4 range includes the opposite direction twice after wrapping; state checks handle repeated access. Costs are integer and node indices are stored as 16-bit values without a local pool-size cap protecting that width.

## 6. Open-node storage and heap behavior

The search OpenHeap is a min-heap of pointers to 20-byte nodes. Nodes carry their heap index, or their next-free index while on the free list. SiftUp (`0x40f000`) and SiftDown (`0x40f060`) order by integer f and update moved nodes' indices. Equal priorities remain subject to insertion/child-selection ordering rather than a separate deterministic geometric tie-break score.

A topPopped marker defers removing the expanded top until the next operation. Size excludes that marked top. Pushing can reuse it directly; updating another node can displace and then remove it. RemoveNode (`0x40ef20`) returns a node to the free list, replaces its heap slot from the end and sifts downward. FreeNode (`0x40f1e0`) only threads the free list. These are allocation/priority mechanics, not path waypoints.

GrowNodes (`0x40f110`) allocates a larger pool and heap-pointer array, copies existing nodes and translates pointers using old pool offsets. When requested size is below current capacity, growth uses capacity plus half plus 16; the ordinary -1 request selects that rule. Allocation success, arithmetic bounds and representable cell node indices are assumed rather than comprehensively checked here.

The earlier generic Entry heap helpers (`0x40d620`, `0x40d670`, `0x40d700`, `0x40d740`) use float-key pairs and ordinary less-than max-heap ordering. They are distinct from OpenHeap's min-heap. CopyDwordPair (`0x40d5e0`) and CopyOneByte (`0x40d600`) perform conditional placement copies when destination is nonnull. Adjacent library helpers must not be confused with a second route-selection policy.

## 7. Traceback and bounded waypoint horizons

TracePath (`0x40e050`) starts at found and follows inverse incoming directions until start. It stores the endpoint and each direction change in a 64-point ring, then start. This compresses straight runs rather than retaining every traversed cell. It emits up to the most recently recorded 64 points in start-forward order, converting each to world-unit shorts with `(cell * 2 + footprint) * 8`.

AiSearchGoal SetWaypoints further caps the delivered array to 20. A long, bend-heavy path therefore becomes a near-start horizon; the final requested goal can lie beyond the retained points. The goal module requests another search when that horizon runs short. Multiplayer sends at most three points from it. These three limits, 64 traceback turns, 20 local points and three transmitted points, are distinct and explain why a displayed/replicated path need not contain the entire route.

Traceback assumes the predecessor chain reaches start and has no independent cycle/length guard in its loop. Source state is internally generated; this is not a validator for arbitrary damaged search grids. World-unit short conversion can truncate outside representable map coordinate ranges.

## 8. Scheduler, heuristic adaptation and fairness limits

RunSearches (`0x40eb70`) returns for zero configured players, otherwise divides 1333 by numPlayers to obtain each active player's credit increment. Eligibility requires active role 1/2/3 and a non-10 player index. Ten budget slots accumulate credit, and their positive/negative balances contribute to a shared total.

Every 150 calls it computes each player's scan load divided by unitsPerPlayer. Ratios below one select heuristic scale base times six (9.0), below two base times three (4.5), otherwise base (1.5), then reset load counters. Despite the global name playerBudgetCap, these values become **heuristic scale**, not a clamp on accumulated credit. A unitsPerPlayer zero divisor is not guarded locally.

With no active search, it cycles player index until finding positive credit, advances that player's unit cursor with wraparound, and charges one accounting step for the scan. A live definition, motion object and movement class qualify for a planner's repath claim. The [goal claim](orders-order-targets.md) enforces its own 60-tick throttle. An accepted request charges 100 additional accounting steps, adopts that player's heuristic scale and runs StartSearch, including the probe's work.

With a search active, it expands until reaching a goal, exhausting open nodes or taking 100 counted expansion steps in the chunk. Found paths trace and release; exhausted search supplies zero waypoints and releases. Chunk cost subtracts from shared total and the current player's budget. The outer loop can continue the same active search even after that player's balance goes negative, while total credit from others remains. Player selection rotates only when no unit is active. This is shared accounting with persistent active-search ownership, not strict equal wall time or strict per-player preemption every 100 nodes.

Credit integer division discards remainder. Scans, probes and expansion use different accounting charges; this budget is not milliseconds. Map complexity and search demand can affect route latency. Repeated pending claims do not imply one completed path every 60 ticks.

## 9. Cleanup, cancellation and empty results

ReleaseHeldPathLock (`0x40e9a0`) asks the movement class to refresh the requester if stale and clears object/owner. Normal finish uses the same refresh before dropping ownership. AbortIfGoalMatch (`0x40e9c0`) instead clears object, path, target and owner only when the supplied goal matches; it does not run the refresh helper in that body. Goal replacement/destruction calls this cancellation service so the scheduler cannot later write into that old goal.

An empty SetWaypoints is interpreted by AiSearchGoal: when its owner does not already satisfy the destination it reports action flag 0x40 and clears active path. Thus a no-route result can become an order-level path failure through a separate module. Destination satisfaction and the later unit movement check remain distinct from the probe's 0x100/0x200 notifications.

No active heap/grid serialization occurs here. Saves restore unit/order goals through their serializers; new search work reconstructs route state. Local route generation and remote waypoint holders have different authority roles even though the scheduler's player eligibility includes controller role 3.

## 10. Naval interpretation and evidence limits

For a ship unable to cross a channel, inspect class depth/slope/footprint before search costs. For a circuitous route, inspect visibility grade 2, known grade-1 penalties, turn penalties, heuristic scale and target geometry. For repeated stops, inspect bounded waypoint horizon, repath claims, scheduler credit and immediate collision separately. A route that reaches the probe's best heuristic location can still require further approach or fail actual destination satisfaction.

This reference covers construction/destruction, every search phase, both heap families, placement-copy support, all annotated entry points and scheduler/lifetime interactions through static source tracing. It does not claim optimal routes, safe arbitrary map sizes, measured frame costs or confirmed gameplay reproduction. No build or game execution was performed.
