---
title: Flow 2.4 (work in progress)
---

This is the upcoming release. This page is updated regularly after changes are pushed to the repository.

This release includes pull requests from the community: Bargestt (Vasilii Bulgakov), CyaDaPaKnat, dyanikoglu (Doğa Can Yanıkoğlu), fcarreiro (Facundo), LindyHopperGT (Riot Games), Maksym Kapelianovych, northstarswap, omarchuk-gsc.


This is the first release for UE 5.9.

## Update Notes
### Critical warning for Data Pins users
If you were using Data Pins in your assets prior to Flow 2.2, do not upgrade directly from your current Flow Graph version to a version newer than 2.3.
Version 2.2 came with a huge Data Pins refactor and it requires data migration occurring while loading assets.
* First update to Flow Graph 2.2 or 2.3.
* Resave all Flow Graph assets.
* Then update to the newer Flow Graph version.

### Changed Flow Graph Node constructor to GENERATED_BODY
This is a BREAKING CHANGE if you have any custom UFlowGraphNode class. Updating the code is trivial though.

## Flow Node
* Added `Flow Identity` struct with UI customization which filters actors in the world by actor and component class. (contributed by Bargestt)
    * This is used to provide a convenient actor picker on Flow Nodes.
    * The struct needs to be manually added to a Flow Node class and read by the node's logic.
* Added PIE world context guard during `ForcePinActivation`. This fixes issues with nodes spawning actors which happened in the wrong (editor) world. (contributed by CyaDaPaKnat)
* `GetNodeTitle()` and `GetNodeDescription()` are now called on node instances only when displaying actual node instances. This allows us to preserve the default title and description for the archetype (e.g. in the node selection list). (contributed by LindyHopperGT)
* Exposed `GetInputPins()` and `GetOutputPins()` to blueprints for automatic tests. (contributed by omarchuk-gsc)
* Editor-only properties of `FFlowPin` wrapped with `WITH_EDITORONLY_DATA`: display name and tooltip. (contributed by Bargestt)

## Specific Nodes
* `UFlowNode_SubGraph`
    * Added support for passing data from the finished SubGraph instance to the owning SubGraph node! (contributed by LindyHopperGT)
    * Moved `TrySupplyDataPin` override in `UFlowNode_SubGraph` outside of `WITH_EDITOR` directive. This fixes Flow Asset Params in packaged builds. (contributed by dyanikoglu)
* `PlayLevelSequence` node now supports multiplayer via replicated binding on `AFlowLevelSequenceActor`. (contributed by LindyHopperGT)

## Flow Asset
* Diff improvements. (contributed by MaksymKapelianovych)
    * Reintroduced graph orientation changing.
    * Added highlighting of all changed properties in the details view (as well as the focused one).
    * Added linked scrolling between details views.
    * Fixed populating the diff entry tree. The add-on hierarchy is now preserved when an add-on's direct parent has no changes but its grandparent does.
    * Fixed focusing a node in the graph when the user clicks a diff element belonging to an add-on.
    * Made details views read-only.

## Flow Component
* Added `bMarkFlowCategoryImportant` option to forcefully move the "Flow" category to the top of component Details. Enabled by default. (contributed by Bargestt)
* Added options to set default Gameplay Tag categories for `IdentityTags` property. (contributed by Bargestt)
    * `DefaultIdentityTagCategories` sets the global default and is also used as the default for the `Flow Identity` filter.
    * `ComponentIdentityTagCategories` allows setting different defaults for different Flow Component classes.
* Added convenient access to Flow Component tags from selected actor details. (contributed by Bargestt)
    * Also makes it possible to edit tags when multiple actors are selected.
    * Controlled by the `bShowFlowTagsInActorDetails` flag, enabled by default.

## Misc
* Fixed the `GetObjectsWithOuter` call in `RemoveOrphanedNodes` for UE 5.8. (contributed by fcarreiro)
* Fixed `-Wunreachable-code-loop-increment` errors on Clang. (based on changelist contributed by northstarswap)
