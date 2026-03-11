---
title: Flow 1.0
---

March 8, 2021. The first publicly released version of the plugin!

* [Flow 4.26 + example content](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.0-example)
* [Flow 4.26](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.0-4.26)
* [Flow 4.25](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.0-4.25)
* [Flow 4.24](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.0-4.24)
* [Flow 4.23](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.0-4.23)
* [Flow 4.22](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.0-4.22)

## Flow Graph
* `Flow Graph` is a custom visual scripting system in which a single node (`FlowNode`) represents UObject, while a blueprint node (K2Node) represents UFunction.
* Unlike blueprints, Flow Node is async/latent by design. Active nodes usually subscribe to delegates, so they can react to events by triggering output pins (or whatever you choose to).
* Its concept is described in this [24-minute in-depth presentation](https://www.youtube.com/watch?v=BAqhccgKx_k).

## Flow Subsystem
* `Flow Subsystem` is a runtime [programming subsystem](https://dev.epicgames.com/documentation/en-us/unreal-engine/programming-subsystems-in-unreal-engine).
* It manages the lifetime of Flow Assets that contain Flow Graph (and its Flow Nodes). In other words, the Flow Subsystem creates instances of Flow Assets.
* `Flow World Settings` provides a method to automatically instantiate Flow Asset assigned to the persistent level. The new toolbar in the main editor window is used to assign this asset to the Flow World Settings. You have to set Flow World Settings as your World Settings class in Project Settings.
* Flow Subsystem allows instantiating Flow Assets from any script in the game by calling StartRootFlow method. Conceptually, it's like you could instantiate your own Level Blueprint from anywhere - a graph controlling events in the level, but not part of any actor or component.

## Flow Component Registry
* Instantiated Flow Asset isn't part of the world, and it doesn't allow to reference level actors like Level Blueprints. How do we access actors then? Every actor you want to access in Flow Graph needs to have a simple `Flow Component` (part of the plugin). Every Flow Component in the world is registered on `BeginPlay` to the Flow Subsystem. 
* This what I call the `Flow Component Registry`. This registry is a map of all Flow Components existing in loaded levels and spawned actors.
* It exposes methods to quickly obtain references to Flow Components existing in the world, identified by `Identity Tags` - a Gameplay Tag container included in Flow Component. Since you can get components, you can easily get an actor that owns a given component.
* It's possible to access Flow Component Registry in Flow Subsystem from any script. It's a cheap way of getting actor references because it filters out optimized TMap instead of iterating all actors in the world every time (like GetAllActorsOfClass method).

## Things not included
There are few things I described in that 24-minute presentation, but they're not part of the plugin.
* Variables support. Adding variables to assets and passing values to nodes. This is a core blueprint feature that I'd like to implement in some way in Flow Asset. It's not a trivial thing to do. I don't know when this would be done.
* You won't find any game-specific systems in the Flow plugin, like quest system or dialogue system. The fundamental idea of this plugin is to provide you an excellent framework for building your project-specific system. I might only provide some examples in the future. This would be added to the separate [FlowGame](https://github.com/MothCocoon/FlowGame) repository.
* FactsDB isn't implemented, it might be in the future. 