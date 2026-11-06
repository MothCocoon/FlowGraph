---
title: Fast Forward
layout: default
---

Reproducing a bug often requires playing a part of the game first: walking through triggers, using interactions, waiting for cutscenes. Fast Forward automates this. It pushes the game from its current state to the node chosen by the user, executing the same graph logic that a playthrough would execute, without waiting for the player or for passing time.

It's a debugging feature available during PIE. The subsystem running it and the route finder live in the `FlowDebugger` module, so they're not included in shipping builds.

## Usage

* Start PIE and open a Flow Asset. Select an instance in the debugger, if you'd like to target a node of a specific instance.
* Right-click a node that was never activated and choose **Fast Forward to Here** from the **Execution Override** section.
* You can also pick a node in a graph that isn't instanced yet, i.e. an asset assigned to a SubGraph node that hasn't started. The route is found through SubGraph nodes instancing this asset.
* If there's no route, a notification explains why: the node is already active, no active node leads to it, or the node doesn't belong to a running graph.

## Finding the route

`FFlowFastForwardRouteFinder` searches backward through exec connections, starting from the chosen node. The first active node reached through one of its outputs starts the route. The search is breadth-first, so it finds the nearest active node and the shortest route to the chosen node.

The route passes a node only through a pair of input and output pins that can be connected by the node logic, e.g. Timer's Skip input never leads to its Completed output, and an output of MultiGate opened already doesn't open again. Nodes declare this by overriding `CanFastForward`. The same check applies to the active node starting the route, with no input: an active Timer can't trigger Skipped without receiving Skip.

The search crosses graph boundaries.
* Reaching the Start node leads up to the Start pin of the SubGraph node that created this graph, if this graph isn't running.
* Reaching a Custom Input node leads up to the matching custom input pin of the SubGraph node, if this graph is running. SubGraph node passes custom inputs only to its running graph.
* Reaching an output of a SubGraph node leads into the graph of this SubGraph node, to the Finish node or the Custom Output node sending this output.
* Graphs not instanced yet are searched through their templates. Only templates referencing the asset of the chosen node, directly or through other assets, are loaded, found through the Asset Registry.
* SubGraph node in PassThrough mode doesn't run its graph, so the route passes it like any other node.

The search doesn't restart anything that already runs. If the route would need to start a running graph again (including Root Flow), there's no route. Disabled nodes don't pass the signal, so the route never goes through them.

The result is `FFlowFastForwardRoute`: a list of segments, each listing steps within a single graph. A step identifies a node by its Guid, the input receiving the signal and the output passing it further. Nodes are identified by Guid, since graph instances below the starting one are created only while fast forwarding.

## Executing the route

`UFlowFastForwardSubsystem` receives the route and pushes the signal from the active node starting it. While it runs, every node on the route receives `FastForwardInput` instead of `ExecuteInput`. Nodes outside the route, e.g. other outputs of a Sequence, execute as usual. The same applies to route nodes in other instances of the same asset.

The rules are simple:
* Game logic executes for real, as during a playthrough. Nodes spawn actors, send notifies, enable interactions, save the game, start SubGraphs.
* Only what the node waits for is emulated: player action, world event, passing time. The emulated event comes from its real source, so every listener reacts to it, not only the node. If the node has no way to reach the source, it receives the event directly.
* Nodes choosing the output themselves (Branch, Switch, MultiGate) trigger the output required by the route.
* The SubGraph node passing the signal into its graph, the Finish or Custom Output node passing the signal out of a graph and the target node receive the regular `ExecuteInput`.

Fast Forward ends when the signal reaches the target node.

## Implementing FastForwardInput

The default `UFlowNode::FastForwardInput` executes the node logic (including AddOns) and checks whether the node triggered the expected output by itself. If the node still waits for something, the expected output is forced. The node isn't finished by Fast Forward, finishing it remains up to the node logic, as in the game.

Every output forced because the awaited event couldn't be emulated is logged with the node class and the number of outputs forced for this class in the current session. This log tells which node classes need their own implementation. Outputs triggered by Branch, Switch and MultiGate aren't logged, since following the route is their designed behavior.

Override `FastForwardInput` if the node waits for something that can be reproduced.
* Execute the input the same way as `ExecuteInput` does, so the node starts its work.
* Emulate the awaited event at its source, i.e. broadcast the delegate the node subscribed to.
* Trigger the expected output, if the event didn't trigger it.
* Finish the node with the forced output, if this output finishes the node in the game. Otherwise the node keeps waiting and triggers the output again when the real event comes.
* `PinName` is None for the active node starting the route, since it received its input before.

Override `CanFastForward` if some inputs of the node can't lead to some of its outputs, so the route never requires such a pass.

Nodes derived from `UFlowNode_ComponentObserver` only need to override `FastForwardEvent`, which emulates a single event observed by the node.

`FastForwardInput` isn't exposed to blueprints yet. Blueprint nodes use the default implementation.

## Nodes provided by the plugin

| Node | Fast Forward behavior |
|------|------|
| Play Level Sequence | Evaluates remaining frames at once by the Play method, so sequence events fire and actors end in the state set by the sequence. Completes the sequence or plays it up to the nearest Flow Track event on the route, never backward. If the sequence doesn't complete this way, it jumps to its end, or to its start if played in reverse. If the event doesn't fire, e.g. its section is muted, the output is forced. |
| On Notify From Actor | The observed actor sends the notify itself. |
| On Actor Registered | Starts observing, actors already registered trigger the event. Otherwise, the event is received directly. |
| On Actor Unregistered | The event is received directly. |
| Nodes observing components (above) | Completed output emulates as many events as the success limit requires. Only the Stop input leads to the Stopped output. |
| Timer | Starts the timer, then fires all steps and completes at once. Step output fires a single step, then the timer continues from there. Only the Skip input leads to the Skipped output. |
| Branch, Switch, MultiGate | Execute their AddOns, then trigger the output required by the route. |
| Counter | Increment leads to Step or Goal, Decrement to Step or Zero, and Skip only to Skipped. Forced Goal or Zero finishes the counter. |
| AND | Forced output finishes the node, as if all inputs were triggered. |
| OR | Only numbered inputs of the enabled node lead to the output. |
| Other nodes | Default implementation. |

## Limitations and open questions

This is an early implementation.
* The subsystem follows the route step by step. If a node on the route finishes without triggering the expected output, Fast Forward stops and logs the step where the route broke.
* If a node on the route receives the signal from outside the route, or the next node isn't reached for 5 seconds (e.g. asynchronous `FastForwardInput` that never completes), an error is logged and Fast Forward keeps waiting for the expected step. Implement `FastForwardInput` of the nodes involved or report the case on Flow Discord.
* Only one Fast Forward runs at once, since Flow runtime reaches the subsystem through a single global gate.
* Should fast forwarded graphs be saved like regular ones?
* Multiplayer wasn't verified yet.
