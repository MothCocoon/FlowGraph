---
title: Flow 1.5
---

September 8, 2023.

This release includes pull requests from the community: Deams51,  Drakynfly (Guy Lundvall), eddieataberk (Eddie Ataberk), growlitheharpo (James Keats), jhartikainen (Jani Hartikainen), LindyHopperGT (Greg Taylor), ryanjon2040 (Satheesh), soraphis, twevs.

This is the first release for UE 5.3, and the last for UE 5.0.

* [Flow 5.3](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.5-5.3)
* [Flow 5.2](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.5-5.2)
* [Flow 5.1](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.5-5.x)
* [Flow 5.0](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.5-5.0)

Flow Solo (sample project) releases
* [Flow Solo 5.3](https://github.com/MothCocoon/FlowSolo/releases/tag/v1.5-5.3)
* [Flow Solo 5.2](https://github.com/MothCocoon/FlowSolo/releases/tag/v1.5-5.2)
* [Flow Solo 5.1](https://github.com/MothCocoon/FlowSolo/releases/tag/v1.5-5.1)
* [Flow Solo 5.0](https://github.com/MothCocoon/FlowSolo/releases/tag/v1.5-5.0)

## New: Call Owner Function
This allows us to call blueprint functions on owning objects from the Flow Graph. This is quite a significant feature added by contributed by LindyHopperGT.

## Flow Asset
* Calling `CustomOutput` on the Root Flow instance now attempts to call an event on the owning Flow Component. Event it's called `OnTriggerRootFlowOutputEvent`, and it's available in the blueprint version. (contributed by LindyHopperGT)
* Improved `UFlowAsset::GetDefaultEntryNode` returns an unconnected Start Node if a connected one hasn't been found. (contributed by LindyHopperGT)
* Added `HasStartedFlow` method returning True, if any node recorded a pin activation. (contributed by LindyHopperGT)
* Exposed the `GetNodesInExecutionOrder` method to blueprints. Added to it the `UFlowNode* FirstIteratedNode` parameter, allowing to start iteration from any node.
* Removed the default implementation of `UFlowAsset::PreloadNodes`, it was a forever prototype. You can still implement this method on your own.

## Flow Node
* Added the `DeinitializeInstance` method called on all Flow Nodes in the graph from `UFlowAsset::DeinitializeInstance`. (contributed by LindyHopperGT)
* Exposed the `GetConnectedNodes` method to blueprints. (contributed by eddieataberk)
* Removed `RecursiveFindNodesByClass` method, superseded by `UFlowAsset::GetNodesInExecutionOrder`.

## Specific Flow Nodes
* Allow the `UFlowNode_ExecutionSequence` node to execute new connections. (contributed by jhartikainen)
* `Play Level Sequence`
   * Added Pause and Resume input pins. (contributed by twevs)
   * Moved declaration of `FStreamableManager` from `UFlowNode` as no other nodes in the plugin use it.

## SaveGame support
* `UFlowSubsystem::AbortActiveFlows` is now a blueprintable function. (contributed by soraphis)

## Editor
* Added the `Asset Defaults` button to the Flow Asset toolbar. (contributed by LindyHopperGT)
* Added hyperlink allowing to open class of the given Flow Asset. Works the same as with blueprints. It's helpful when subclassing `UFlowAsset`. (contributed by Drakynfly)
* Add validation error on `UFlowAsset` for disallowed node classes. (contributed by Deams51)
* Add the ability for FlowAsset child classes to define if they are allowed in subgraph nodes. (contributed by Deams51)
* Fixed `JumpToInnerObject` not working when SearchTree provides the GraphNode itself. (contributed by growlitheharpo)
* Specified categories and setting names for the plugin's UDeveloper class, so it's now easier to find them. (contributed by ryanjon2040)

## Misc
* Reworked FFlowBreakpoint into FFlowPinTrait - inviting to use for things other than breakpoints.
* Removed the use of `FStreamableManager` in the `UFlowSubsystem`.
* Updated code to build in 5.1 and 5.2 using engine version checks instead of separate code on different plugin branches. (contributed by LindyHopperGT)