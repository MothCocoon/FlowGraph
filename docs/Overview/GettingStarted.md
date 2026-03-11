---
title: Getting Started
---

## Sample projects
There's a separate repository including [a sample project called FlowGame](https://github.com/MothCocoon/FlowSolo), so you can easily check how this plug-in works. It includes
* Flow plugin
* Additional C++ Flow nodes, as `FlowQuest` plugin.
* Simple map with a few Flow Graphs.

## Adding Flow Graph to your project
1. Unpack the plugin to the Plugins folder in your project folder. If you don't have such a folder yet, simply create it.
2. First of all, open Project Settings in the editor. Change `World Settings` to the `Flow World Settings` class. Restart the editor. That class starts the Flow assigned to the map, just when starting the game. So it serves as a replacement for BeginPlay in the level blueprint.
3. You can assign a Flow Asset to the map via the Flow toolbar above the main viewport. Assigning it directly via the World Settings editor window also works.

It's crucial to perform Step 3 after Step 2. Otherwise, a reference to the Flow Asset might be improperly saved in the map and wouldn't work. If that happens, just try to revert your map. And assign the Flow Asset again.

## Including GitHub repository in your repository
You can include this plugin repository as a dependency. It can be done by using Git submodules. 
```
git submodule add -b 5.0 https://github.com/MothCocoon/FlowGraph.git Plugins/Flow
```

## Useful tools
* [Subsystem Browser plugin](https://github.com/aquanox/SubsystemBrowserPlugin) works really well with Flow Graph, as it displays runtime data of subsystems like Flow Subsystem.