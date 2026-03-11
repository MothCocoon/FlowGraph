---
title: Flow 2.0
---

December 15, 2024.

This release number is bumped to 2.0 thanks to 2 huge features introduced by Riot Games. They did awesome work this year and chose to contribute to our humble open-source project.

This release includes pull requests from the community: Benxidosz, Bigotry0, DoubleDeez (Dylan Dumesnil), dyanikoglu (Doğa Can Yanıkoğlu), dzxmxd (Wang Xudong), Joschuka, LindyHopperGT, MaksymKapelianovych, NachoAbril, slobodin (Nikolay Slobodin), Soraphis, SPontadit, Unbansheee.

This is the first release for UE 5.5, and the last for UE 5.3.

* [Flow 5.5](https://github.com/MothCocoon/FlowGraph/releases/tag/v2.0-5.5)
* [Flow 5.4](https://github.com/MothCocoon/FlowGraph/releases/tag/v2.0-5.4)
* [Flow 5.3](https://github.com/MothCocoon/FlowGraph/releases/tag/v2.0-5.3)

Flow Solo (sample project) releases
* [Flow Solo 5.5](https://github.com/MothCocoon/FlowSolo/releases/tag/v2.0-5.5)
* [Flow Solo 5.4](https://github.com/MothCocoon/FlowSolo/releases/tag/v2.0-5.4)
* [Flow Solo 5.3](https://github.com/MothCocoon/FlowSolo/releases/tag/v2.0-5.3)

## Flow AddOns
AddOns lets us create modular Flow Nodes. This solves the problem of big monolithic Flow Nodes with overly complex code trying to support all use cases of given mechanics in the game.
(contributed by LindyHopperGT, Riot Games)

Related work
* Restored SFlowGraphNode pin alignment to pre-FlowAddon settings. (contributed by Unbansheee)
* This feature replaces the need to use `Call Blueprint Owner Function`. Please remove any usage of that old feature, it will be removed in the next plugin release.

## Data Pins
This is initial support for passing property values between Flow Nodes! This implementation is based on Struct Utils introduced with UE5. It doesn't utilize blueprint-specific code.
(contributed by LindyHopperGT, Riot Games)

Related work
* Initialize Data Pin Struct Values in order to remove errors. (contributed by Benxidosz)

## Flow Asset
* POTENTIALLY BREAKING CHANGE: Replaced usage of old `FAssetTypeActions` with new `UAssetDefinition`. Added `UAssetDefinition_FlowAsset` implementing a new way of defining editor-only asset properties like asset category. This is a direct replacement for `FAssetTypeActions_FlowAsset`. If you had a custom asset class extending `FAssetTypeActions_FlowAsset`, you must convert it to a class inheriting after `UAssetDefinition_FlowAsset`. 
    * This is quite a straightforward process. You don't need to register new class in the editor module class.
    * Here's a [commit](https://github.com/MothCocoon/FlowGraph/commit/5ef45f328809213024c99e9aba883ace7a104c58) introducing that change.
* Added a search functionality that does not rely on engine modifications. (contributed by dzxmxd)
* Added the `AdditionalNodeIndexing` method to Flow Asset Indexer. This allows developers to add custom logic to Asset Search indexing. (contributed by SPontadit)
* Improvements to Flow Diff. (contributed by Riot Games)
    * Fix FlowGraph details diffing by assigning Splitter to the `FDiffControl.Widget` in `SFlowDiff::GenerateDetailsPanel()`
    * Add nesting to tree entry display to more easily read the diffs.
    * Add individual property diffing with highlights.
    * Nest Add-On nodes inside their parent flow nodes.
    * Added FFlowObjectDiff to track all the data needed to display an individual tree entry's diff.
* Fixed Diff menu not using the FlowAsset of the current editor when several flow asset editors are opened. (contributed by SPontadit)
* Added `OnDetailsRefreshRequested` delegate. It allows developers to refresh the Asset or Node details panel without adding a boilerplate to projects.
* Made it possible to pass InstanceName to CreateRootFlow. (contributed by dyanikoglu)
* Finish root flow instances without calling finish root flows externally. (contributed by Joschuka)
* Call NotifyGraphChanged when validating an asset to refresh nodes without making the asset dirty. (contributed by SPontadit)

## Flow Node
* The display style of nodes (the body color) is now controlled by the gameplay tag from the `Flow.NodeStyle` category instead of the `ENodeStyle` enum. (refractor contributed by LindyHopperGT)
   * All existing enum options have been recreated as native tags declared in the `FlowNodeStyle` namespace. Pre-existing settings of nodes and color schemes are automatically converted to the new format.
   * It also means, you only need a minor code change to use new tags directly. Simply change your C++ code from `EFlowNodeStyle::InOut` to `FlowNodeStyle::InOut.
* Improvements to generating Node Title. (contributed by MaksymKapelianovych)
   * Editing `UFlowGraphEditorSettings::bShowNodeClass` now immediately updates node titles in the graph.
   * Editing `UFlowSettings::bUseAdaptiveNodeTitles` now immediately updates node titles in the graph.
   * `UFlowGraphNode::GetTooltipText()` now calls `UFlowNode::GetNodeToolTip()`.
   * Refresh the graph only once, when the node asset is renamed.
   * User can now specify node prefixes that will be automatically removed, instead of manually writing custom `meta = (DisplayName = ...)`. Now `UFlowGraphSettings` holds the `NodePrefixesToRemove` array, where user can add their custom node prefixes. By default, it contains two elements: "FN" and "FlowNode". Any duplicating elements will be instantly removed with an error notification.
   * To optimize the process of prefix removal, nodes' names without prefixes are generated and stored as custom `GeneratedDisplayName` metadata every time the array is changed. `UFlowNode::GetNodeTitle()` method has been modified to return `GeneratedDisplayName` in case the node class does not have `BlueprintDisplayName` and `DisplayName` metadata, and `UFlowNode::bDisplayNodeTitleWithoutPrefix` == true. Similar changes have been made in `UFlowNode::GetNodeToolTip()`, so now it also returns `GeneratedDisplayName` if possible.
* Added a bunch of const keywords to allow usage of LogError/LogWarning/LogNote in const functions without const_cast. (contributed by MaksymKapelianovych)
* Added check to prevent a crash when deleting two or more node assets (if some of them are in undo history). (contributed by MaksymKapelianovych)

## Specfic Flow Nodes
* Added `UFlowNode_ExecuteComponent` which executes a UActorComponent on the owning actor as if it was a SubGraph. (contributed by LindyHopperGT)
* `UFlowNode_PlayLevelSequence`: prevent multiple output pins with the same name (contributed by Soraphis)

## Flow Component
* Added support for replicating variables using the Push Model. (contributed by NachoAbril)
* Moved the "Start or Load of the RootFlow" to a virtual function. (contributed by Soraphis)
* Added `virtual` keyword to `StartRootFlow` and `FinishRootFlow` methods. (contributed by dyanikoglu)

## Misc
* Refactored plugin's code to utilize TObjectPtr for raw UPROPERTY pointers. Some projects might have TObjectPtr enforcement enabled starting from UE 5.5, and this prevented the plugin from compiling. (contributed by dyanikoglu)
* Change the FlowEditor loading phase to `PreDefault` to fix corrupting FlowAssets when DefaultPawnClass for GameMode is set in C++ (as in Epic's templates). (contributed by MaksymKapelianovych)
* Replaced the monolithic header include (PropertyEditing.h) with the corresponding includes. (contributed by SPontadit)
* Fixed compilation for iOS. (contributed by slobodin)
* Fixed compile failure due to default C++ standard below 20. (contributed by Bigotry0)
* Fixed short type name warnings. (contributed by DoubleDeez)
