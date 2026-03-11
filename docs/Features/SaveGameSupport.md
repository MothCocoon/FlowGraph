---
title: Save Game Support
layout: default
---

Flow Graph plugs into Unreal's `SaveGame` system. If you haven't used it yet, read [Saving and Loading Your Game](https://dev.epicgames.com/documentation/en-us/unreal-engine/saving-and-loading-your-game-in-unreal-engine).

You control which properties are included in SaveGame by marking C++ properties with the `SaveGame` specifier. Or by ticking the `SaveGame` checkbox in the blueprint editor.

## What to look for?
* `FlowSave.h`. Active graphs are serialized to the `UFlowSaveGame` object, which extends the engine's `USaveGame`. That allows you to integrate Flow Graph into your SaveGame setup.
* `UFlowSubsystem` keeps a registry of all active Flow Graphs at the given moment. That's why it also contains methods providing SaveGame support. To support Flow Graph in your save system, you need to call `OnGameSaved` and `OnGameLoaded` methods. These are accessible from blueprints.
* `UFlowNode` class provides overridable events `OnSave` and `OnLoad`, so you can add custom SaveGame logic to any node, like restoring Timer with "RemainingTime" value read from SaveGame. Check the `UFlowNode_Timer` class for reference.
* `UFlowNode_Checkpoint` node is a built-in example of implementing autosave called from quests.
* `UFlowAsset` and `UFlowComponent` expose similar `OnSave` and `OnLoad` events, so you should be able to customize SaveGame logic in every plugin's class that's involved in SaveGame operations.

[Signal Modes](SignalModes.html) features provide a solution for modifying Flow Graphs post-launch, once players already have SaveGames with serialized graph state.

## Quick sample
You can find a quick example of integrating Flow into your SaveGame setup in the `FlowGame` project. Here are [simple C++ classes related to this](https://github.com/MothCocoon/FlowGame/blob/5.7/Source/FlowGame/).

Note: You might need to call `UFlowSubsystem::LoadRootFlow` manually on your Root Flow owners if you're loading a game while the world is already active. Flow Component automatically calls `LoadRootFlow` only on BeginPlay! Supporting in-game loading is up to you.

## Support for graphs not instantiated from the Flow Component
It's possible to create Root Flow for any UObject owner, like Player Controller or a subsystem. If these objects don't include the Flow Component, supporting Save/Load logic requires a bit more work.
* You need to call `UFlowSubsystem::LoadRootFlow` on this custom owner after deserializing the SaveGame with `UFlowSubsystem::OnGameLoaded`. Look at the sample code linked above. You need to iterate on owners if they don't include the Flow Component's logic.
* If your Root Flow is created on an UObject owner that doesn't belong to the world (Game Instance or its subsystem), you need to set the `bWorldBound` property on your Flow Asset to False.