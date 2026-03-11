---
title: Flow 1.2
---

April 5, 2022.

This release includes pull requests from the community: Drakynfly (Guy Lundvall), kathelon (Erdem Acar), LunaRyuko (Luna Ryuko Zaremba), Mephiztopheles (Markus Ahrweiler), ryanjon2040, seimei0083 (Joseph).

This is the first release for UE 5.0, and the last one for UE 4.25.

* [Flow 5.0](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.2-5.0)
* [Flow 4.27](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.2-4.27)
* [Flow 4.26](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.2-4.26)
* [Flow 4.25](https://github.com/MothCocoon/FlowGraph/releases/tag/v1.2-4.25)

Flow Solo (sample project) releases
* [Flow Solo 5.0](https://github.com/MothCocoon/FlowSolo/releases/tag/v1.2-5.0)
* [Flow Solo 4.27](https://github.com/MothCocoon/FlowSolo/releases/tag/v1.2-4.27)
* [Flow Solo 4.26](https://github.com/MothCocoon/FlowSolo/releases/tag/v1.2-4.26)

## Flow Node
* It's now possible to deprecate Flow Nodes. Simply set the `bNodeDeprecated` flag to True. You can provide a replacement class by choosing another Flow Node as `ReplacedBy` class. Nodes deprecated this way continue to work normally.
* It's now possible to mark a Flow Node class as `bCanDelete` and `bCanDuplicate` without creating the `UFlowGraphNode` class just for that. (submitted by ryanjon2040)
* Added DeniedClasses as option to blacklist FlowNodes. (submitted by Mephiztopheles)
* `UFlowNodeBlueprint` isn't marked as final anymore, it's now possible to extend it (submitted by LunaRyuko)
* Added `IsInputConnected` method.
* Fixed typo in `JumpToNodeDefinition` label (submitted by Drakynfly)

## Specific Flow Nodes
* Added Restart input pin to the `UFlowNode_Timer` node. (submitted by kathelon)
* `UFlowNode_PlayLevelSequence` now takes into account `CustomTimeDilation` of actor owning Root Flow (actor with a Flow Component creating that Flow Graph instance). (submitted by seimei0083)

## Fixes
* Clearing `UFlowSaveGame` in `UFlowSubsystem::OnGameSaved` method, before serializing the current data. This prevents the critical issue if game-specific code passes reused SaveGame object here. We only remove data from the current world and global Flow Graph instances (not bound to any world). We keep data from all other worlds.
* Loading SaveGame no longer calls `Start` node on all loaded Sub Graphs.
* Fixed some localization key collisions (submitted by Drakynfly).