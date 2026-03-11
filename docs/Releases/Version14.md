---
title: Flow 1.4
---

May 21, 2023.

This release includes pull requests from the community: ArseniyZvezda (Arseniy Zvezda), bohdon (Bohdon Sayre), LindyHopperGT (Greg Taylor), lfowles (Landon Fowles), Mephiztopheles (Markus Ahrweiler), michalmocarskiintermarum (Michał Mocarski), ryanjon2040 (Satheesh), twevs, Vi-So (Alex van Mansom).

This is the first release for UE 5.2.

* [Flow 5.2](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.4-5.2)
* [Flow 5.1](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.4-5.1)
* [Flow 5.0](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.4-5.0)

Flow Solo (sample project) releases
* [Flow Solo 5.2](https://github.com/MothCocoon/FlowSolo/releases/tag/v1.4-5.2)
* [Flow Solo 5.1](https://github.com/MothCocoon/FlowSolo/releases/tag/v1.4-5.1)
* [Flow Solo 5.0](https://github.com/MothCocoon/FlowSolo/releases/tag/v1.4-5.0)

## Upgrade notes
Do not update directly from version 1.0 to this version. Important fixups of deprecated properties have been removed in this version.

## New: Message Log support
* Added `Validate` toolbar action calling data validation on the entire asset, checking all Flow Nodes. If any issues are found, these are reported in the new `Validation Log` window.
* Node validation uses the new `UFlowNode::ValidateNode` method which is available in C++ only.
* Added `Runtime Log` windows displaying runtime message set by Flow Nodes, using methods `LogError`, `LogWarning`, and `LogNote`.
* Editor throws a notification on the PIE end if there are any warnings or errors reported.
* Refactored plain debugger struct into Flow Debugger Subsystem which gathers Runtime Logs.
* Fixed: `UFlowNode::LogError` doesn't execute its logic in Shipping builds anymore. Also, prevented calling runtime log methods on templates of the Flow Asset.

![2UFTrHOoGP](https://user-images.githubusercontent.com/5065057/226177111-a987759b-43b9-45bc-befc-88069367aae5.png)

## Editor misc
* Compilation-breaking change for C++. Context Pins now are constructed directly as `FFlowPin` instead of `FName`. It allows setting display names and tooltips on such pins.
* `Refresh` command now also recreates `UFlowGraphNode` instances if a given `UFlowNode` now has a different `UFlowGraphNode` assigned. Only applicable to C++ classes, as blueprint nodes cannot have custom `UFlowGraphNode`. (reworked pull request by ArseniyZvezda)
* Improved filtering of given Flow Node subclasses in `Flow Graph Schema`.
* Extending the `GetAssignedGraphNodeClass()` function to support returning the correct `EdGraphNode` for grandchild classes of `UFlowNode`. (contributed by Vi-So)
* The FlowGraphSchema will now create default nodes for any CustomInputs that exist when the asset is first created. (contributed by LindyHopperGT)
* Show a pretty readable pin name even if a friendly name is not provided. This is optional behavior that might be enabled by the `bEnforceFriendlyPinNames` flag in editor settings. (based on ryanjon2040 contribution)
* Extracted graph-related code from the `FFlowAssetEditor` to the new `SFlowGraphEditor` class.
* Added `SFlowGraphEditor::IsTabFocused` method to prevent delete/paste/copy nodes if the graph tab isn't focused.
* Exposed all remaining classes out of the module, allowing you to extend whatever you need.
* Moved `MovieScene` headers to the Public folder. Moved the `SLevelEditorFlow` class to the Utils folder.
* Moved all Details Customizations classes to a single `DetailsCustomizations` folder.

## Asset Search
* Jump from the Asset Search result to the Flow Node in any graph editor. To unlock this feature
  * Integrate this pull request to your engine: [Jump from Asset Search result to the node in any graph editor!](https://github.com/EpicGames/UnrealEngine/pull/9882). 
  * Set the `ENABLE_JUMP_TO_INNER_OBJECT` value to 1.
* Added option to run a search on a single asset, with Search Browser opened as an asset editor tab. To unlock this feature
  * Integrate this pull request to your engine: [Asset Search: added option to run a search on a single asset, with Search Browser opened as asset editor tab](https://github.com/EpicGames/UnrealEngine/pull/9943) 
  * Set the `ENABLE_SEARCH_IN_ASSET_EDITOR` value to 1.

## Flow Node
* Added `Node Color` property directly to the Flow Node class. (contributed by ryanjon2040)
* Added log message while node executes in `Disabled` or `PassThrough` mode.
* Added options to disable printing Signal Mode messages to the Output Log. (contributed by ryanjon2040)
* Added editor user setting `bShowNodeDescriptionWhilePlaying` allowing to hide of static node descriptions while PIE/SIE is active. (contributed by ryanjon2040)
* Moved the `TryGetRootFlowActorOwner` method from the `PlayLevelSequence` node to the base class. Also provided a component version of the same code. (contributed by LindyHopperGT)
* Exposed direct access to `Flow Node` connections in C++.
* Fixed: correctly updating user-add pins on nodes like `Sequence` after removing one of pins.
* Fixed: rare crash if node would trigger output on exiting game.
* Compilation-breaking change: Removed template method `LoadAsset` as it was redundant.
* Optimized building debug-only status strings, when using methods like `GetProgressAsString`.

## Specific Flow Nodes
* Introduced a superclass to `UFlowNode_CustomInput` and `UFlowNode_CustomOutput` so they can share more code. (contributed by LindyHopperGT)
* Exposed CustomInput Add/Remove functions on UFlowNode to allow subclasses to modify the `CustomInputs` array. (contributed by LindyHopperGT)
* Added a UseAdaptiveNodeTitles option to optionally make CustomInputs integrate their EventName into the title for the node. This defaults to false (to preserve previous behavior by default). (contributed by LindyHopperGT)
* Added the `exact match` option to the `Notify Actor` node. (contributed by bohdon)
* Timer node complete in next tick if a value is closer to 0. (contributed by ryanjon2040)
* Added search keywords to nodes: `AND`, `Checkpoint`, `Log`, `Multi Gate`, `NotifyActor`, `OnActorRegistered`, `OnActorUnregistered`, `OR`, `Timer`. (contributed by ryanjon2040)
* (UE 5.1+) Fixed applying `FMovieSceneSequencePlaybackSettings` while creating `UFlowLevelSequencePlayer`. (contributed by michalmocarskiintermarum)
* Fixed: "Pause at End" does nothing in the `Play Level Sequence` node. (contributed by twevs)
* Fixed: Component Observer may continue triggering outputs if the last component triggered a finish during `UFlowNode_ComponentObserver::StartObserving`. (fix proposed by lfowles)

## Flow Component
* Fixed bug in `UFlowComponent::GetRootInstances()` where the Owner parameter wasn't being used. (contributed by LindyHopperGT)

## SaveGame support
* Exposed methods to blueprints: `LoadRootFlow`, `LoadSubFlow`. (contributed by Mephiztopheles)
* Fixed rare issue with loading saves: prevent triggering input on a not-yet-loaded node. Now when restoring the graph state, we iterate the graph "from the end", backward to the execution order. This prevents an issue when the preceding node would instantly fire output to a not-yet-loaded node.

## Runtime misc
* Flow Asset no longers requires `Start Node` to be always a default entry node. It's still a default behavior, but you can change it by overriding `UFlowAsset::GetDefaultEntryNode()`. 
* Corrected templated version of `UFlowAsset::GetOwner()`. (contributed by ryanjon2040)
* Added null `UFlowAsset` check when calling `UFlowSubsystem::StartRootFlow`. (contributed by bohdon)
* Changed some functions to return const &'s rather than doing a full deep copy of a member container. (contributed by LindyHopperGT)
* Added includes and forward declaration fixing some compiler errors. (contributed by LindyHopperGT)

## New: Import from blueprint graph
* Added `UFlowImportUtils` blueprint library to create Flow Graph asset in the same folder as the selected blueprint.
* Includes simple utility method that recreates Flow Graph with nodes matching blueprint function nodes (`UK2Node_CallFunctions`). 
* Utility automatically transfers blueprint input pin values to Flow Node properties if the pin name matches the Flow Node property name. It's also possible to map mismatched names (blueprint pin to Flow Node property) as the utility input parameter.
* Added `UFlowNode::PostImport()` allowing to update a newly created Flow Node just after it got created.

NOTE. It's NOT meant to be the universal, out-of-box solution as the complexity of blueprint graphs conflicts with the simplicity of the Flow Graph.
However, it might be useful to provide this basic utility to anyone who would like to batch-convert their custom blueprint-based event system to the Flow Graph.
