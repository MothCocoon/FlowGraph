---
title: Search
---

There are two ways to search Flow Graphs:
* **Search in the Flow Asset editor** - built into the plugin. Searches nodes of the current asset or all Flow Assets, without indexing.
* **Asset Search** - the engine tool searching all asset types, including Flow Assets. Requires indexing the project first.

## Search in the Flow Asset editor

Open it with the **Search** button on the Flow Asset editor toolbar or with `Ctrl+Shift+F`.

* Type one or more words. A node matches if it contains all of them, case-insensitive.
* **Scope** decides which assets are searched:
  * **This Asset** - the asset opened in the editor.
  * **All Flow Assets of This Type** - all assets of the same Flow Asset class, including subclasses.
  * **All Flow Assets** - every Flow Asset in the project.
* **Filters** decide which parts of a node are searched: titles, tooltips, classes, comments, descriptions, config text, property names, property values, AddOns, subgraphs and pin names.
* With the **Subgraphs** filter enabled, search continues into assets instanced by SubGraph nodes. Results found there are listed under the SubGraph node.
* **Pin filter** narrows pin name matches to input or output pins, connected or unconnected ones.
* **Max depth** limits how deep the search goes into objects and structs nested in node properties.

Click a result to open its asset and jump to the node. Double-click a result found inside a subgraph to open the asset instanced by the SubGraph node and jump to the node there. The tooltip of a result shows which filters matched and the matched property.

The editor remembers the filters and pin filter you used last. The default max depth can be set in Editor Preferences, in the **Flow Graph** section.

## Asset Search

Engine plugin added in UE 4.26, still marked as beta, probably because of the search performance. The Flow plugin enables it and registers an indexer for Flow Assets. The indexer covers properties of Flow Assets, node titles and comments, properties of nodes, and default values of unconnected input pins.

### Using the feature
- Open the `Tools/Search` tab.
- If you never used this Search yet, check the status of "missing indexed assets" in the bottom right corner. Click it to index all assets supported by the Asset Search. It can take many minutes, depending on your project size.
- Wait until indexing finishes and the status in the bottom left corner is `Ready` again.

### Limiting Search by asset type
If you merge the provided engine modification, you can exclude specific asset types from Asset Search indexing, on the project level (via Project Settings) or the user level (Editor Preferences).
* [Added Search Roles to Asset Search settings as a huge time-saver](https://github.com/EpicGames/UnrealEngine/pull/9332)

Here's the list of asset types supported out of the box, hardcoded in `FAssetSearchManager::Start()` (as of UE 5.7).
```cpp
RegisterAssetIndexer(UDataAsset::StaticClass(), MakeUnique<FGenericObjectIndexer>("DataAsset"));
RegisterAssetIndexer(UDataTable::StaticClass(), MakeUnique<FDataTableIndexer>());
RegisterAssetIndexer(UCurveTable::StaticClass(), MakeUnique<FCurveTableIndexer>());
RegisterAssetIndexer(UBlueprint::StaticClass(), MakeUnique<FBlueprintIndexer>());
RegisterAssetIndexer(UWidgetBlueprint::StaticClass(), MakeUnique<FWidgetBlueprintIndexer>());
RegisterAssetIndexer(UDialogueWave::StaticClass(), MakeUnique<FDialogueWaveIndexer>());
RegisterAssetIndexer(UWorld::StaticClass(), MakeUnique<FLevelIndexer>());
RegisterAssetIndexer(AActor::StaticClass(), MakeUnique<FActorIndexer>());
RegisterAssetIndexer(USoundCue::StaticClass(), MakeUnique<FSoundCueIndexer>());
RegisterAssetIndexer(UMaterial::StaticClass(), MakeUnique<FMaterialExpressionIndexer>("Material"));
RegisterAssetIndexer(UMaterialFunction::StaticClass(), MakeUnique<FMaterialExpressionIndexer>("MaterialFunction"));
RegisterAssetIndexer(UMaterialParameterCollection::StaticClass(), MakeUnique<FGenericObjectIndexer>("MaterialParameterCollection"));
RegisterAssetIndexer(UMaterialInstance::StaticClass(), MakeUnique<FGenericObjectIndexer>("MaterialInstance"));
```

### Useful engine modifications
None of these modifications is merged into the engine, as of UE 5.7.

* [Jump from Asset Search result to the node in any graph editor!](https://github.com/EpicGames/UnrealEngine/pull/9882) - set `ENABLE_JUMP_TO_INNER_OBJECT` to 1 in `FlowEditorDefines.h` after integrating this change.
* [Asset Search: added option to run search on single asset, with Search Browser opened as asset editor tab](https://github.com/EpicGames/UnrealEngine/pull/9943)
