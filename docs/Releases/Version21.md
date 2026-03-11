---
title: Flow 2.1
---

June 14, 2025.

This release includes pull requests from the community: 39M (Nil), Deams51 (Mickaël), fchampoux (pixelchamp), HomerJohnston (Kyle Wilcox), IRSMsoso, jnucc, LindyHopperGT, Maksym Kapelianovych, MichaelCenger (Michael Cenger), Numblaze, Rhillion, Ryan DowlingSoka, Soraphis.

This is the first release for UE 5.6, and the last for UE 5.4.

* [Flow 5.6](https://github.com/MothCocoon/FlowGraph/releases/tag/v2.1-5.6)
* [Flow 5.5](https://github.com/MothCocoon/FlowGraph/releases/tag/v2.1-5.5)
* [Flow 5.4](https://github.com/MothCocoon/FlowGraph/releases/tag/v2.1-5.4)

Flow Solo (sample project) releases
* [Flow Solo 5.6](https://github.com/MothCocoon/FlowSolo/releases/tag/v2.1-5.6)
* [Flow Solo 5.5](https://github.com/MothCocoon/FlowSolo/releases/tag/v2.1-5.5)
* [Flow Solo 5.4](https://github.com/MothCocoon/FlowSolo/releases/tag/v2.1-5.4)

## Flow Subsystem
* Modified `UFlowSubsystem::RemoveSubFlow`. Now we're invalidating the `AssetInstance->NodeOwningThisAssetInstance` pointer after calling `AssetInstance->FinishFlow`, as this point may be needed in the FinishFlow method. (contributed by fchampoux)
* Exposed `CreateFlowInstance` as a public method. Useful in projects where Root-SubGraph relations are "replaced" with a loose set of graphs, i.e. card games.
    * Moved `LoadSynchronous()` calls out of this method, so external code is allowed to use async asset loading.
    * Methods operating on asset templates - `AddInstancedTemplate`, `RemoveInstancedTemplate` - turned back to `protected`. It turns out, it doesn't make sense to duplicate the logic of the CreateFlowInstance method.
* Allow multiple instances on loading Root Flow. (contributed by Numblaze)

## Flow Asset
* Graph refresh refactored. (contributed by HomerJohnston)
   * The goal of this PR is to make the graph editor stop refreshing the entire graph during as many events as possible. It also fixes minor bugs related to orphaned pins, running undo/redo commands on edge cases, and unnecessary graph dirtying on edge cases, such as when nodes contain orphaned or invalid pins.
   * **BREAKING CHANGE**. Public void `UFlowGraphNode::RefreshContextPins(bool)` changed to protected void `UFlowGraphNode::RefreshContextPins()`.      The existing system has some logic that could result in attempts to recursively call ReconstructNode -> RefreshContextPins -> ReconstructNode -> RefreshContextPins. Currently, there are booleans to guard against this, but both methods are exposed and this feels unclean.
    * `UFlowGraphNode::PostLoad` now rebuilds the unserialized InputPins and OutputPins arrays using the serialized Pins array.
    * `UFlowAsset::HarvestNodeConnections` - made this function able to harvest a single node, or harvest the whole graph (pass in nullptr to harvest the whole graph, or pass in a single node to harvest just that node). Then `UFlowGraphNode::NodeConnectionListChanged` updates node connections for only a single node when its connections are changed.
    * Changed the right-click context menu command, "Refresh Context Pins", to "Reconstruct Node".
    * Modified the following functions to only refresh individual nodes instead of the whole graph by changing NotifyGraphChanged() calls to NotifyNodeChanged(Node): `FFlowGraphSchemaAction_NewNode::CreateNode`, `UFlowGraphSchema::TryCreateConnection`, `UFlowGraphSchema::BreakNodeLinks`, `UFlowGraphNode::RemoveOrphanedPin`, `UFlowGraphNode::AddInstancePin`, `UFlowGraphNode::RemoveInstancePin`.
    * `FlowGraphSchemaAction_NewNode::CreateNode` now runs ReconstructNode() instead of only spawning default pins.
    * `UFlowGraphNode::OnGraphRefresh` now runs ReconstructNode() instead of RefreshContextPins(), on all nodes.
    * `UFlowGraphNode::RefreshContextPins` - no longer runs if an editor transaction is in progress (running undo/redo).
    * `UFlowGraphNode::HavePinsChanged` - now disregards any orphaned pin connections.
    * Added a new delegate `UFlowGraphNode::OnReconstructNodeCompleted` to trigger the rebuild of SFlowGraphNode widgets whenever ReconstructNode is called.
    * `SFlowGraphNode` added destructor to unbind from UFlowGraphNode delegates.
    * `UFlowGraphSchema`added virtual bool ShouldAlwaysPurgeOnModification() const override { return false; } to reduce unnecessary graph refreshes.
    * `UFlowNodeAddOn` - GetContextInput/OutputPins functions changed to ignore invalid pins and write a log warning for invalid pins.
    * Post-refactor fix: copy-paste breaking reroute node directionality. (contributed by Ryan-DowlingSoka)
* Added "Select" button, which will select and focus the Custom Event node with the same name. If such a node does not exist in the graph, the button will be inactive. (contributed by Maksym Kapelianovych)
* Added menu and toolbar extensibility managers. This allows new options to be added to the menu and toolbar for custom flow asset types without the need to extend/override FFlowAssetEditor. (contributed by MaksymKapelianovych)
* Fixed Linux packaged build by passing by reference. (contributed by IRSMsoso)

## Flow Node
* `UFlowNode::TriggerOutput` won't execute its logic if a given node is deactivated. (contributed by 39M)
* Fixed `UFlowNode::Branch` never deactivating after checking its condition. (fix suggested by Geckostya)
* Added missing calls to the parent class `Cleanup` method. This fixes calling Cleanup on the attached AddOns. (contributed by Rhillion)
* Fixed: Node pass-through does not work under closed-loop conditions. (contributed by MaksymKapelianovych)
* Several improvements to copy/pasting nodes. (contributed by MaksymKapelianovych)
    * Reset EventName in `UFlowNode_CustomInput` after duplicating or copying/pasting.
    * Fix pasting nodes into Flow Asset when Flow Asset cannot accept such nodes (node or asset class is denied).
    *  If among selected nodes there are nodes that cannot be deleted, they will stay in the graph as is, and all "deletable" nodes will be deleted (currently, none of the nodes will be deleted in such a case).
* Exposed ability to override node's Category via `OverridenNodeCategories` list in Flow Graph project settings. This way, you can fully reorganize the Flow Palette to your liking. (inspired by LindyHopperGT's proposal)
* The first-ever update to built-in node categories. (inspired by LindyHopperGT's proposal)
    * I partially accepted a proposed update to the category layout. I updated the defaults, which are flatter than proposed, as this would be enough for projects starting with a limited number of project-specific nodes.
    * It feels good to refresh categories after a few years of using that palette. The `World` category has been effectively renamed `Actor` since a growing number of projects embrace ECS. It's useful to separate nodes tied to the OOP paradigm from multi-threaded ECS.
    * **BREAKING CHANGE**. Flow Node source files have been moved to matching folder names. If your code contains C++ classes referencing built-in Flow Node classes, you might need to update your includes.
* Fixed the ability to paste nodes while some need is still selected. (contributed by jnucc)
* Added custom Make/Break implementation for the `FFlowPin` struct to avoid using the "BlueprintReadWrite" specific - since that would block users from compiling out cosmetic properties in non-editor builds. (contributed by LindyHopperGT)
* Added additional constructors for `FFlowDataPinResult_Enum(`) used in an AI Flow node: "GetBlackboardValues". (contributed by LindyHopperGT)
* `SubGraph` node: added null checks to fix crash while attempting to load Flow Asset removed with "Force Delete" option.
* Removed a deprecated `Call Owner Function` feature.
* Adaptive Build fix for when the entire FlowGraph is built as separate source files. (contributed by jnucc)

## Flow Debugger
* There are several improvements to Flow breakpoints.
    * Node and pin breakpoints are now stored locally for every user. This is thanks to moving data structures out of `UFlowGraphNode`; we no longer need to save assets to remember added breakpoints. Data is now stored in .ini, which was created for this purpose, `UFlowDebuggerSettings`. Logic is handled by the new `UFlowDebuggerSubsystem` class. (contributed by Maksym Kapelianovych)
        * Any breakpoints set on older plugin versions will vanish.
        * POTENTIALLY BREAKING CHANGE: `FFlowPinTrait` struct has been renamed to `FFlowBreakpoint` and moved to the `FlowDebugger` module. If you utilize this struct in your custom code, please re-add it to your project code.
        * Added a new module to the Flow plugin, `FlowDebugger`, which is a `DeveloperTool` module. This is where `UFlowDebuggerSubsystem` lives. This is extended by the pre-existing editor class: `UFlowDebugEditorSubsystem`.
    * Refactored old breakpoint logic to open doors to building a separate cook-only graph debugger. If anyone would be willing to implement such a feature, of course. This would require much work, but the Flow Graph community keeps surprising!
        * Decoupled triggering breakpoints from the Flow Editor module. `UFlowGraphNode` no longer operates on breakpoints. Instead, `UFlowDebuggerSubsystem` binds directly to the runtime `OnPinTriggered` delegate.
        * Pin breakpoint is now identified as `NodeGuid` and `PinName` instead of `UEdGraphPin`. Thanks to this, it's now possible to bind to the `OnPinTriggered` delegate outside of the editor! Events shall be received in the Standalone game, non-shipping game builds.
        * Wrapped `UEdGraphNod` pointer in runtime Flow Node class with `WITH_EDITORONLY_DATA` since this isn't even loaded in a Standalone game.
        * Now it's possible to add custom logic to "On Pin Triggered" logic outside of the runtime module. You can add what you want by extending UFlowDebuggerSubsystem or UFlowDebugEditorSubsystem.
* Prevented occasional crashes while dispatching permanent error messages. (contributed by jnucc and Deams51)
* Added `CanPlaceBreakpoints()` functionality in `UFlowGraphNode`, return false on `UFlowGraphNode_Reroute`. (contributed by jnucc)
    * Added conditions to check if `CanPlaceBreakpoint()` on the selected node is true. Also, since multiple breakpoints on nodes can be set/unset at the same time, added the checks in these functions too. Since they are in a for loop, don't return immediately at the first element, but check if it makes sense to return true on the first item that returns true.
* Fixed breakpoints that could erroneously be added on reroute nodes through the context menu, or through F9, while not being visible at all.
    
## Flow Component
* Added `TriggerRootFlowCustomInput` method. (contributed by Soraphis)
* Renamed methods related to Custom Output events, now called `BP_OnRootFlowCustomEvent` and `OnRootFlowCustomEvent`.
* Fixed a few issues with `IdentityTags` replication. (contributed by MichaelCenger)
    * Changes to identity tags while offline (NM_Standalone) did not replicate to clients if they went online at some later point.
    * Changes to identity tags on the server before BeginPlay was called on the Flow Component would never replicate to clients.
    * Multiple calls to `AddIdentityTag(s)` or `RemoveIdentityTag(s)` within a single net update did not get replicated properly. It would only replicate the last of each respective operation.



