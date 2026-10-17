# Unit squads and group command dispatch

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `units/squads`. All four annotated functions are in [squads.cpp](../../../src/units/squads.cpp), `0x480190` through `0x480460`. The module stores ten player-owned membership lists and translates a group command into individual unit orders. It does not implement formation geometry, pathfinding, target eligibility or AI strategy.

## 1. Two representations of membership

Each player owns ten Squad objects, indexed 0..9. Each squad stores its owner, index, two initially zero fields and a vector of unit pointers. Every unit separately stores a signed-style group value at offset +0xac, with minus one meaning no squad.

This is one squad assignment per unit, not an arbitrary set of simultaneous memberships. Consistency requires the unit's field and the corresponding vector to agree. The owner used to locate a vector is the unit's current player pointer. Changing that pointer without first handling membership would make a later removal look in the wrong player's vector.

`CreateSquads`, `0x480190`, allocates space for ten squads and constructs each in place with its player and index. The initialization contract expects fresh player state; it does not first release an existing squad allocation. [Player creation and teardown](../../../src/game/players_464290.cpp) call the allocation and release functions as part of player lifecycle.

`FreeSquads`, `0x4801f0`, checks the allocation pointer, destroys the ten vectors, frees their containing storage and clears the player pointer. Destroying these pointer vectors does not kill their unit objects or rewrite every unit's group field. Unit cleanup and player teardown ordering must prevent subsequent access to freed squad storage.

## 2. Moving a unit between squads

`SetUnitSquad`, `0x480250`, first removes the unit from its old squad when its current group differs from minus one. Removal searches the vector for the unit, replaces a found entry with the final entry and erases the final slot. It then appends to the destination vector unless the requested index is minus one, and writes the new value to the unit.

Removal does not preserve vector order. Assigning the same squad again is not a no-op: it removes and appends, potentially changing member order even when membership stays the same. If the old vector does not contain the unit, removal silently does nothing and assignment proceeds. The helper does not independently repair duplicate entries introduced outside this contract.

The method does not range-check indices other than recognizing minus one. Valid callers must supply minus one or a value within the ten allocated squads, and must provide a unit with a valid owning player. This module imposes no unit-type eligibility policy on a group: aircraft, ships, buildings and builders can be assigned by their callers.

The [unit initializer](units-units.md) assigns new units to squad 0, and death handling removes them with minus one. [Selection code](../../../src/ingame/selection.cpp) and [AI code](../../../src/ai/ai_player.cpp) also call this interface. AI categories are decisions in the AI module; a squad index itself does not grant a weapon, locomotion mode or construction capability.

## 3. Dispatching orders to a group

`OrderSquad`, `0x480460`, walks the owning player's entire inclusive unit-slot range. It does not iterate the squad's vector. A slot qualifies when its definition index is nonzero and its stored group value equals the requested key.

For every qualifying unit, the routine calls GetOrderType with the requested command mode, that unit, the optional target and flags. It then calls AddOrder with the returned mission type, caller-supplied removal/queue control, unit, target and remaining arguments. The order dispatcher and order queue retain responsibility for eligibility, command interpretation, queue replacement and later execution.

Consequently a heterogeneous group need not receive the same mission type for every member. Dispatch is per unit. An attack-like input can be interpreted differently according to each unit's capabilities and target context. This function does not itself make every member able to attack the target or move to its position.

The range scan also means vector order has no direct effect on this command's processing order. The unit field determines membership for dispatch, while vectors support other consumers. A disagreement between the two representations can therefore affect AI/vector consumers differently from group-command delivery. Because the key comparison is unrestricted, minus one could match ungrouped units if a caller deliberately supplied it; the function does not reserve commands exclusively to keys 0..9.

## 4. Persistence, networking and boundaries

The squad module owns no packet or save-bank format. [Unit restoration](../../../src/units/unit_save.cpp) reestablishes membership by calling SetUnitSquad from the stored unit record. It reconstructs the vector relationship through the same API rather than serializing raw vector pointers.

Group commands become ordinary unit orders, whose serialization/replication and execution belong to the order/network modules. Freeing a squad vector neither cancels orders nor detaches a transport, and moving a unit to another squad does not itself change a current order.

For a command that misses a member, inspect the player's inclusive slot range, definition index and unit group field before inspecting vector order. For a stale AI member, inspect SetUnitSquad use and unit-death cleanup. For a group whose units react differently, trace GetOrderType per member and the resulting order handler. Formation movement and AI regrouping require those additional systems; they are not hidden behavior of these four routines.
