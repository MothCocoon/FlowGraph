---
title: Flow 1.1
---

January 23, 2022.

This release addresses the first wave of feedback I'm receiving from early adopters. It includes pull requests from the community: BorMor (Sergey Vikhirev), IceRiverr, Mahoukyou, Mephiztopheles (Markus Ahrweiler), moadib (Marat Yakupov), ryanjon2040, Solessfir (Constantine Romakhov), sturcotte06.

This is the first release for UE 4.27 and UE5 Early Access.

This is also the last release for UE 4.22, 4.23, 4.24. Already a few improvements in v1.1 couldn't be backported.

* [Flow 5.0 EA](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.1-5.0EA)
* [Flow 4.27](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.1-4.27)
* [Flow 4.26](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.1-4.26)
* [Flow 4.25](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.1-4.25)
* [Flow 4.24](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.1-4.24)
* [Flow 4.23](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.1-4.23)
* [Flow 4.22](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.1-4.22)

## New: SaveGame support
* Added initial support for loading/saving instanced Flow Graphs.

## Specific Flow Nodes
* New node: `MultiGate` node which replicates the blueprint node of the same. (submitted by BorMor)
* New nodes: `OnActorRegistered` and `OnActorUnregistered` waiting until actor would appear in the world, or disappear.
* Nodes based on `UFlowNode_ComponentObserver` class can be set to work indefinitely, i.e. On Trigger Enter node would continue to work after the player entered trigger once. The node would trigger the `Success` output pin every time player entered the trigger. Obviously, a node can be stopped anytime by calling the `Stop` input pin. If you have any custom Component Observer nodes, you just need to change a single function call in order to use new options. Replace `TriggerOutput(TEXT("Success"), true)` with `OnEventReceived()`.
* A few nodes now use a Gameplay Tag Container instead of a single Gameplay Tag. So now we have an `IdentityTags` container instead of a single `IdentityTag`. And `NotifyTags` container instead of a single `NotifyTag`. Changed nodes are: `UFlowNode_ComponentObserver`, `UFlowNode_NotifyActor`, `UFlowNode_NotifyFromActor`. Your code might need to be updated if you have a class inheriting after any of these node classes. (partially implemented by moadib)
* Added `IdentityMatchType` to `UFlowNode_ComponentObserver`. Now you have better control over which actors should be accepted on the specific nodes, i.e. you can specify to accept actor that has all Identity Tags listed on the node.
* `CustomEvent` node has been renamed to the `CustomInput`. It should fix the confusion caused by the previous name. Now we have pair of nodes working with custom pins on the `SubGraph` node: `CustomInput` and `CustomOutput`.
* Exposed `FMovieSceneSequencePlaybackSettings` on `UFlowNode_PlayLevelSequence` node.
* (4.24+) Exposed `FLevelSequenceCameraSettings` on `UFlowNode_PlayLevelSequence` node.
* Exposed log verbosity on the `UFlowNode_Log`. (submitted by ryanjon2040)

## Blueprint-specific Flow Node changes
* Exposed `CanUserAddInput` and `CanUserAddOutput` to the blueprint Flow Node. If a method is returning True, the user will be able to add numbered pins on a node in the graph. See Sequence node for reference.
* Flow Asset editor now reads `Blueprint Display Name` property. If set, this overrides asset name as... you guessed... displayed name of the Flow Node in the Flow Graph.
* Flow Asset editor now displays the description for Flow Node blueprints. Simply type text into the `Blueprint Description` property in Class Details.
* API change. `Blueprint Category` property of Flow Node blueprints now overrides the `Category` property from the C++ Flow Node class. This is the way of setting categories for blueprint nodes from now on.
* Exposed methods to get active Flows and Flow Nodes. It allows for creating a custom debug UI.
* Fixed: changing inputs/outputs of the Flow Node blueprint isn't reflected in opened Flow Graphs.
* Fixed: optimized obtaining Flow Node blueprints by editor's Palette.
* Fixed: Details panel of Flow Asset editor doesn't display the non-public properties of blueprint Flow Nodes anymore.

## Flow Node
* API change. Refactored Flow Node pins, now defined as `FFlowPin` struct. Pins now can have tooltips. Blueprint nodes and Flow Assets are automagically updated. C++ nodes need a manual update as defining pins has changed a bit. C++ pins are now defined via methods like `AddInputPins({TEXT("Spawn"), TEXT("Despawn")});` or simply constructing struct with tooltip like `InputPins.Add(FFlowPin(TEXT("Reset"), TEXT("MyTooltip")));`
* Flow Pin constructor accepts many types as PinName: FString, FName, FText, TChar*, int32, uint8.
* Added a few C++ flavors of `TriggerOutput()` methods, accepting FString, FText, TCHAR*.
* API change. Renamed methods for adding numbered pins to `SetNumberedInputPins()` and `SetNumberedOutputPins()`.
* Added `IsOutputConnected` function to Flow Node.
* Added `InitializeInstance` function to Flow Node. It's called just after creating the node instance while initializing the Flow Asset instance. This happens before executing the graph, only called during gameplay. (proposed by sturcotte06)
* Implemented `IsDataValid()` method in the UFlowAsset, so it would return the validation result of Flow Nodes. (submitted by sturcotte06)
* Added `NodeSpecificColors` to `UFlowGraphSettings` (Flow Graph in Project Settings), so it's possible to override the color of any node. (original code submitted by Mephiztopheles)
* Added `GetDynamicTitleColor()` method to the `UFlowNode`, so every node instance can have a different color based on the runtime logic. Node color could even change in runtime!
* Added ability to hide specific nodes from the Flow Palette without changing the source code. Check `NodesHiddenFromPalette` in `UFlowGraphSettings`.
* `LogError` method is now marked as `const`.
* (4.26+) Utilized Viewport Stats Subsystem to display error messages (added by `UFlowNode::LogError`) permanently on the screen. If you'd like to use revert to the old behavior, call `LogError` with parameter `OnScreenMessageType` equal to `Temporary`.

## Flow Asset
* Added `AllowedNodeClasses` list to the `UFlowAsset`. This lets you limit what nodes can be added to a given Flow Asset type.
    * By default, all Flow Node classes are accepted.
    * You can simply change to accept only custom nodes (i.e. dialogue nodes) by adding setting your base Dialogue Node class as the only element of  `AllowedNodeClasses` array.
    * You can add multiple node classes, assign a few specific nodes to the given asset type.

## Flow Component
* Flow Components marked as blueprintable, so people could extend it freely.
* Added methods for adding/removing `IdentityTags` (in Flow Component) during gameplay. These methods update the Flow Component Registry in Flow Subsystem.
* Exposed set of Flow Subsystem functions and delegates to blueprints. This exposes obtaining references to actors in the world containing Flow Component with valid Identity Tag.
* `GetComponents()` method in Flow Subsystem now returns TSet instead of TArray. Actually, there was no advantage of using TArray here, since we can't rely on the order of elements returned from the component registry.
* Two delegates in Flow Subystem became dynamic (exposed to blueprint): `OnComponentRegistered()` and `OnComponentUnregistered()`. You might need to add UFUNCTION() above your methods bound to these delegates in C++.

## Generic multiplayer support - entirely new!
This set of changes attempts to provide generic support for networked projects, in order to minimize the need to modify the plugin's code.
* By default, Flow Subsystem is created on clients. This allows to access the `Flow Component Registry` - an optimized way to find "event actors" in the world. It's also possible to instantiate client-only Flow Graphs. You can disable Flow Subsystem on clients by changing `bCreateFlowSubsystemOnClients` flag in Flow Settings (Flow section in Project Settings).
* By default, the Flow asset assigned to the world (via Flow Toolbar above the main viewport) isn't instantiated on clients. This kind of Flow usually contains gameplay-critical logic (i.e. quests), which should exist only on the server. If you need to call some code there on the client, I would recommend handling it inside a specific Flow Node. However, you can change it by selecting another `RootFlowMode" on the Flow Component in World Settings.
* Added network replication of `IdentityTags` in Flow Component.
* Added network replication of `NotifyTags` in Flow Component. This way designers can script custom multiplayer logic in their actors.

## Root Flow
Root Flow is a Flow Asset instanced by an object from outside of the Flow System, i.e. World Settings or Player Controller. This Root Flow can create child Flow Assets in runtime by using the `SubGraph` node. This release adds more flexibility in managing Root Flows.
* Added `StartRootFlow()` and `FinishRootFlow()` methods to the Flow Component, so it's possible to reduce boilerplate code in actor classes.
* You can now specify on Flow Component, should it start RootFlow on `BeginPlay`? If not, you can freely script when it would be started.
* You can now specify on Flow Component, under which Net Mode would be allowed to start RootFlow? Single-player only (Standalone), Server-only, Authority (both single-player and server), Client-only, Any Mode. Change it by selecting the appropriate `RootFlowMode`.
* Flow Toolbar (available above the main editor viewport) isn't tied to AFlowWorldSettings class anymore. It works with the Flow Component which needs to be a part of your `AWorldSettings` class.
* Added comments in code to clarify what is the `Owner` of the Root Flow instance. It's any object passed as parameter of `UFlowSubsystem::StartRootFlow()`. Typically it would be an object that called this method, i.e. Flow Component.
* `Owner` of the Root Flow instance is now passed to all dependent Flow Asset instances, for easier access. (submitted by sturcotte06)
* Flow Asset is now a blueprint type, so it's possible to create blueprint variables of this type.
* Removed `FlowAsset` input argument from `UFlowSubsystem::FinishRootFlow()` method, it was redundant.
* It's now possible to set Allowed Class for Flow Asset set in Level Editor Toolbar. It's exposed in Flow Graph Settings (project settings).
* Fixed issue with starting Root Flow assigned on different `AFlowWorldSettings` than currently chosen in Project Settings. It might happen if you subclassed `AFlowWorldSettings`. Unfortunately, the engine would instantiate a few World Settings instances, both for your class and parent `AFlowWorldSettings` class.

# Editor
* You can now create Reroute node by double-clicking on the wire. (contributed by ryanjon2040)
* Added circuit connection drawing. (contributed by ryanjon2040)
* Added "graph tooltip", a preview of the entire nested graph while hovering over SubGraph node. (contributed by ryanjon2040)
* Added optional displaying asset path on the Flow Node in the graph (contributed by ryanjon2040)
* Added option to hide Asset Toolbar above Level Editor. Some projects might not use it or replace it with a custom toolbar.
* Added option hide base `UFlowAsset` class from Flow asset category in "Create Asset" menu, by changing `bExposeFlowAssetCreation` flag in `UFlowGraphEditorSettings`. It's useful if you only wanna use your custom Flow Asset classes in the project.
* Added option hide base `UFlowNode` class from Flow asset category in "Create Asset" menu, by changing `bExposeFlowNodeCreation` flag in `UFlowGraphEditorSettings`. It's useful if you only wanna use your custom Flow Node classes in the project.
* Exposed `FlowAssetCategoryName` to `UFlowGraphEditorSettings`, so you can override the default name of the Flow asset category.
* Extracted some plugin-specific Project Settings to a new plugin-specific Editor Preferences (a new class `UFlowGraphEditorSettings`).
* Created `SFlowGraphNode_SubGraph` widget to allow customizations specific to the SubGraph node.
* Marked many methods in the FlowAssetEditor as virtual, for your convenience.
* Added option to override Flow Asset category name, so you group more assets together.

## Fixes
* Changed LoadingPhase of runtime Flow module to `PreDefault`. This fixes rare issues with the plugin not being loaded on time while blueprint actors already try to load the Flow Component.
* Fixed a few editor issues with blueprint Flow Nodes. Editor code was loading blueprint data too aggressively, causing performance issues.
* Added missing editor logic: auto-wiring a newly placed node. (submitted by IceRiverr)
* Fixed node not being registered with the asset after undoing its deletion. (submitted by Mahoukyou)
* Clear previously selected nodes before new node creation. (submitted by BorMor)
* Fixed crash on saving Flow Asset after force-deleting blueprint node used in that asset.
* Fixed case where Root Flow wouldn't be properly finished and cleaned up. (reported and fixed by BorMor)
* Fixed Clang compilation. Fixed compilation with unity disabled. (submitted by sturcotte06)
* Fixed a few compilation issues. (submitted by moadib)
* Solved filename conflicts with Dungeon Architect plugin which caused compilation errors. Make sure you updated the Dungeon Architect plugin to the latest version.
* Added permanent invite link to the Flow discord server in plugin's SupportURL and ReadMe.
* (UE5) Fixed Engine parsing Plugin version error while cooking (submitted by Solessfir)

## API changes
* If you exposed any Flow Node properties (to be edited by users in the Flow Asset) by `EditDefaultsOnly`, you need to change to correct `EditAnywhere` specifier. Otherwise, the property won't be visible in the Details panel of the Flow Asset.
* If you changed anything in the Flow Editor settings, you need to perform a manual fixup. This class has been renamed to the FlowGraphSettings (as part of fixing filename conflict with Dungeon Architect). You need to edit your DefaultEditor.ini and change the old section name `[/Script/FlowEditor.FlowEditorSettings]` to `[/Script/FlowEditor.FlowGraphSettings]`.
* `TemplateAsset` property in FlowAsset is now private. Use `GetTemplateAsset()` method, if you need to access it.
* (4.25+) Merged code from the `FlowDebuggerToolbar` class/file into `FlowAssetToolbar`. Simplified code around creating it, should be less confusing to extend it. Although you might need to update your code if you've done any changes around the toolbar.
