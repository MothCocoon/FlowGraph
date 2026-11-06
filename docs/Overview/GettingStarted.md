---
title: Getting Started
---

## Sample project
There's a separate repository including [a sample project called FlowGame](https://github.com/MothCocoon/FlowGame), so you can easily check how this plug-in works. It includes
* Flow plugin
* Plugins extending Flow (AI Flow, Flow Actors and others), included as Git submodules.
* Game-specific C++ Flow nodes, i.e. spawning actors, trigger and interaction events.
* Simple map with a few Flow Graphs.

## Adding Flow Graph to your project
1. Unpack the plugin to the Plugins folder in your project folder. If you don't have such a folder yet, simply create it.
2. Open Project Settings in the editor. In the `Engine - General Settings` section, change `World Settings Class` to `Flow World Settings`. Restart the editor. That class starts the Flow assigned to the map, just when starting the game. So it serves as a replacement for BeginPlay in the level blueprint.
3. You can assign a Flow Asset to the map via the Flow toolbar above the main viewport. Assigning it directly via the World Settings editor window also works.

It's crucial to perform Step 3 after Step 2. Otherwise, a reference to the Flow Asset might be improperly saved in the map and wouldn't work. If that happens, just try to revert your map. And assign the Flow Asset again.

## Including GitHub repository in your repository
You can include this plugin repository as a dependency. It can be done by using Git submodules. 
```
git submodule add -b 5.x https://github.com/MothCocoon/FlowGraph.git Plugins/FlowGraph
```
