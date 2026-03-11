---
title: Flow 1.3
---

November 15, 2022.

This release includes pull requests from the community: ArseniyZvezda (Arseniy Zvezda), Bargestt, Cchnn, dnault1, DoubleDeez (Dylan Dumesnil), iknowDavenMC, jhartikainen (Jani Hartikainen), Mephiztopheles (Markus Ahrweiler), ryanjon2040, seimei0083, sturcotte06.

This is the first release for UE 5.1, and the last one for UE 4.26, 4.27.

* [Flow 5.1](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.3-5.1)
* [Flow 5.0](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.3-5.0)
* [Flow 4.27](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.3-4.27)
* [Flow 4.26](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.3-4.26)

Flow Solo (sample project) releases
* [Flow Solo 5.1](https://github.com/MothCocoon/FlowSolo/releases/tag/v1.3-5.1)
* [Flow Solo 5.0](https://github.com/MothCocoon/FlowSolo/releases/tag/v1.3-5.0)

## SaveGame support
* [Signal Modes](../Features/SignalModes.html) features provides a solution for modifying Flow Graphs post-launch, once players already have SaveGames with serialized graph state.

## New: Force Pin Activation
* It's a debugging feature available from Pin's context menu during PIE.
* Allows pushing the graph execution in case of blockers, i.e. specific node doesn't work for whatever reason and we want to continue playtesting.
* It works both on Input pins and Output pins. You can even trigger unconnected Input pins this way.

## New: Asset Search
* (UE 5.1+) Implemented Asset Search for Flow Assets. It's now possible to use properties of Flow Assets and Flow Nodes. See this [Asset Search](../Features/AssetSearch.html) documentation.

## New: Visual Diff
* (UE 5.1+) Implemented Visual Diff for Flow Assets.

## Flow Node
* Added `AllowedAssetClasses` & `DeniedAssetClasses` to the Flow Node class. Flow Node class now can be defined in which Flow Asset class can be placed. So it's another way of defining it - as the Flow Asset class has had a similar list of Flow Node classes for a long time.
* Added `OnActivate` event, called while activating the node (executing any input for the first time). (contributed by Cchnn).
* Refactored numbered pins support, so it would fully safe to add regular pins.
* Added `CanFinishGraph` method, so the logic of finishing the graph from the node isn't hardcoded to the `Finish` node anymore.

## Flow Node blueprint
* Added blueprint `TriggerOutputPin` method with a convenient dropdown for Pin Name selection.

## Specific Flow Nodes
* Now it's possible to disable infinite execution of the OR node. Now OR node executes output only once, by default. Added logic to enable/disable the `Logical OR` node. This can be a breaking change if you somewhere relied on an infinitely working OR node. You can fix this by changing the `Execution Limit` value to 0. (based on Mephiztopheles's pull request)
* Added option to replicate Level Sequence over a network via `PlayLevelSequence` node. (contributed by ArseniyZvezda)
* Added out-of-box support for applying Transform Origin via `PlayLevelSequence` node. It can be enabled by `bUseGraphOwnerAsTransformOrigin` flag on the node. Read official docs on [Transform Origin feature](https://dev.epicgames.com/documentation/en-us/unreal-engine/creating-level-sequences-with-dynamic-transforms-in-unreal-engine). (code changes based seimei0083's pull request)
* Fixed one place in `UFlowNode_ComponentObserver` where `IdentityMatchType` property wasn't respected.
* Fixed a rare case in `UFlowNode_ComponentObserver` where the loop in `StartObserving` method would keep on iterating after the node finished its work.
* Removed limitation of executing only the first found `CustomInput` node with the given Event Name. It was actually more confusing than useful.
* Fixed highlighting of activate wire coming out of `CustomInput` node.

## Known Issues
* In UE 5.1 version some playback settings from the `PlayLevelSequence` node aren't passed to the Level Sequence because of the engine change. This [community commit fixes the issue](https://github.com/MothCocoon/FlowGraph/commit/3c46b7ddaf2aafb9c3e9a0f7c293a701cae05189). The next release will include this fix.

## Flow Subsystem
* Compilation-breaking change for C++ users. `UFlowSubsystem::GetActors` methods now return only actors of a given type. The previous implementation moved to `UFlowSubsystem::GetActorsAndComponents`.
* Added `bExactMatch` bool parameter to all blueprints and C++ methods returning actors/components registered to the Flow Component Registry. If this new option is set to False, the search will accept Flow Components with Identity Tags only partially matching provided tag. Be careful, this new option might be very costly as all registered tags needs are iterated in the loop. Const of using the original option (`bExactMatch` set to True) is constant. (implementation inspired by iknowDavenMC's pull request)
* Implemented support for launching multiple DIFFERENT flows for a single owner, i.e. World Settings, Player Controller, etc. Now when starting/finishing/obtaining a Root Flow instance, you need to provide a Template Asset as the parameter.
* Fixed Flow Asset instance names to be truly unique. (contributed by sturcotte06)

## Graph
* Added `PinFriendlyName` to the `Flow Pin` definition. You can use it to override PinName without the need to update graph connections.
* Added `GetStatusBackgroundColor` method to Flow Node, available in blueprint nodes.
* Added `NodeDoubleClickTarget` setting to Flow Graph editor settings. Now users can choose if double-clicking the node opens "Node Definition" (node's blueprint editor or C++ class in IDE) or the "Primary Asset" (i.e. Dialogue asset for PlayDialogue node). (inspired by ryanjon2040's pull request)
* Added `AssetGuid` property to `UFlowAsset`. It might be helpful in advanced cases of save systems, where the game might need to save per-asset progress, i.e. used dialogue choices. A programmer might save the asset name to SaveGame, but... renaming/moving an asset post-launch would break saves.
* Added `bStartNodePlacedAsGhostNode` flag to `UFlowAsset` class. If set to True, the `Start` node in the newly created asset will be displayed as a ghost node. (based on ryanjon2040's pull request)
* Support keyword search for node search in the graph. (contributed by ryanjon2040)
* Improve drawing for Reroute nodes that go backward. (contributed by jhartikainen)
* Fixed issue where clicking the `Add Pin` button on the node would add the Input pin instead of the Output pin. It was happening if adding both Input and Output pins have been enabled on the given node class.
* Prevented crash if `SubGraph` node would try to instantiate the same Flow Asset as the asset containing this `SubGraph` node. Such an option is still disabled by default, so we could avoid triggering an accidental infinite loop. Users can remove this limitation by checking the `bCanInstanceIdenticalAsset` flag on the specific `SubGraph` nodes.

## SaveGame support
* Fixed case where restoring Root Flow from the save might not work, if we were loading the game without changing or reloading the world.
* Added throwing a permanent onscreen error if Flow Component starting Root Flow has no Identity Tag assigned. This is important, as it would break loading SaveGame for this component.
* Exposed `bWorldBound` bool to eliminate the need to subclass UFlowAsset for simple world-independent Root Flows.

## Asset creation
* Added logic allowing to add `FlowAsset` and `FlowNode` blueprints to already existing asset categories, i.e. "Gameplay". (contributed by Bargestt)
* From now if you'd clear the `Default Flow Asset Class` value when creating a new Flow Asset you'd get a "Pick Flow Asset Class" dialog. This might come in handy if have multiple Flow-based systems in the project. (contributed by Bargestt)
* Added `ConfigRestartRequired` to a few Flow Graph Settings properties. (contributed by Bargestt)

# Editor
* `Go To Master` button renamed to `Go To Parent`. The button is now only visible when it can be used, previously it was only disabled.
* Renamed uses of `FEditorStyle` to `FAppStyle` as it's been deprecated in 5.1. (contributed by DoubleDeez)
