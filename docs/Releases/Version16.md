---
title: Flow 1.6
---

April 27, 2024.

This release includes pull requests from the community: DoubleDeez, IllusiveS (Patryk Wysocki), InfiniteAutomaton (Mark Price), jnucc, LindyHopperGT, Maksym Kapelianovych, sergeypdev, Soraphis, VintageDeveloper.

This is the first release for UE 5.4. It's the last for UE 5.1 and UE 5.2.

* [Flow 5.4](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.6-5.4)
* [Flow 5.3](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.6-5.3)
* [Flow 5.2](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.6-5.2)
* [Flow 5.1](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.6-5.1)

Flow Solo (sample project) releases
* [Flow Solo 5.4](https://github.com/MothCocoon/FlowSolo/releases/tag/v1.6-5.4)
* [Flow Solo 5.3](https://github.com/MothCocoon/FlowSolo/releases/tag/v1.6-5.3)
* [Flow Solo 5.2](https://github.com/MothCocoon/FlowSolo/releases/tag/v1.6-5.2)
* [Flow Solo 5.1](https://github.com/MothCocoon/FlowSolo/releases/tag/v1.6-5.1)


## Flow Asset
* Added Graph Alignment feature known from blueprints. (submitted by VintageDeveloper)
* Add graph action to delete node and reconnect pins if possible. (submitted by MaksymKapelianovych)
* Change the C++ protection level of remaining runtime methods from private to protected.
* Fixed crash when doing a diff of a Flow Graph vs Perforce. (submitted by InfiniteAutomaton)
* Fixed: Nodes with Context Pins, loose ALL pins on CTRL+Z / can lead to a crash. (submitted by MaksymKapelianovych)
* Fixed incorrect `MustImplement` paths. (submitted by DoubleDeez)
* Fixed Palette does not indent headers for sub-categories. (submitted by Soraphis)
* Fixed: Reroute node does not format correctly when going in reverse. (submitted by VintageDeveloper)
* Fixed: Improve drawing for reroute nodes that go backwards when they are selected, connection is executing ot executed during PIE. (submitted by MaksymKapelianovych)
* `bHighlightInputWiresOfSelectedNodes` set to True by default in the Flow Graph Editor Settings. (submitted by VintageDeveloper)
* Removed unconditional dirtying asset which might occur on constructing graph editor.

## Flow Node
* Exposed the `CanFinishGraph` and `Finish` methods as public C++ methods.
* Exposed `SetGuid` and `GetGuid` methods to blueprints.
* Add `EFlowNodeDoubleClickTarget::PrimaryAssetOrNodeDefinition` to support opening the Asset and falling back to opening the Node Class if the node doesn't have an asset. (submitted by DoubleDeez)
* Exposing gameplay tag `Match Type` on the `NotifyActor` node. (submitted by IllusiveS)
* Allow optional hot reload for native Flow Nodes. (submitted by sergeypdev)
* Fixed UFlowNode_SubGraph in cooked builds where Asset member could dereference to nullptr, when trying to get the PathName after converting to UObject*. Just try to get the Path Name from the SoftObjectPtr itself instead. (submitted by jnucc)
* Improvements for working with Input/Output pins in BP FlowNodes. (submitted by MaksymKapelianovych)
    * Before this change, FlowGraph reflected changes in BP FlowNode only after pressing Compile. If the user wanted to revert changes, Ctrl+Z was reverting only changes in BP FlowNode, but not in the FlowGraph. To show changes in the Graph, Compile needed to be pressed again. Now every change inside Input/Output pins is immediately reflected in opened FlowGraphs and pressing Ctrl+Z will revert changes both in BP FlowNode and FlowGraph 
    * UFlowNode now also executes OnReconstructionRequested if pin info has been changed inside the pin array (PinName, PinFriendlyName, PinToolTip).
    * Removed OnBlueprintPreCompile() and OnBlueprintCompiled() from UFlowGraphNode, because all possible node reconstructions, handled by these functions, now are triggered from UFlowNode::PostEditChangeProperty()
    * SFlowGraphNode now hides pin name only if there is one valid pin in the array, not just Pins.Num() == 1.
* Fixed a bug in `CallOwnerFunction` where it would not bind correctly. (submitted by LindyHopperGT)
* Fixed a bug where flow nodes were in the palette, even in inaccessible plugins (now the Flow Node palette respects plugin access rules). (submitted by LindyHopperGT)
* Added expanded `CustomOutput` access. (submitted by LindyHopperGT)
* Clarified editor-accessible vs. runtime-accessible information for custom in/outputs. (submitted by LindyHopperGT)
* `CallOwnerFunction` improvements: Display node description above a node (as for triggers, timers etc). (submitted by MaksymKapelianovych)
* `CallOwnerFunction` improvements: Support for context pins (as for PlayLevelSequence). Now if Params type is changed in referenced function or pins inside Params are renamed/added/removed, CallOwnerFuntion node will not be refreshed. Pins will appear only if user changes referenced function to some other function and then returns back, which is not very convenient. After my changes, Refresh action will update node pins as well. Also, deleted some now excessive function to unify logic for pins refreshing. (submitted by MaksymKapelianovych)
* Fix crash in `UFlowNode_CallOwnerFunction`. (submitted by MaksymKapelianovych)

## Flow Component
* Split out some functionality for `Flow Subsystem` registration to be called separately (to support our object pooling model). (submitted by LindyHopperGT)

## Reducing compilation times
* Moved definitions of the log channels to separate headers: `FlowLogChannels` and `FlowEditorLogChannels`.
* Added `#include UE_INLINE_GENERATED_CPP_BY_NAME` to every possible .cpp. It's a new thing in UE 5.1 and it's meant to "improve compile times because less header parsing is required."
* Wrapped editor-only includes with the `WITH_EDITOR`.
* Removed some of the redundant includes indicated by Resharper.
* Moved a few includes to .cpp files. Added forward declaration where needed.
* Non-unity mode build fixes. (submitted by InfiniteAutomaton)

Misc
* Removed `AFlowWorldSettings::IsValidInstance()` needed in pre-UE5 era.