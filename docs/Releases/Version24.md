---
title: Flow 2.4 (work in progress)
---

This is the upcoming release. This page is updated regularly after changes are pushed to the repository.

This release includes pull requests from the community: Bargestt (Vasilii Bulgakov), CyaDaPaKnat, dyanikoglu (Doğa Can Yanıkoğlu), fcarreiro (Facundo), fede-Raider (Federico Ciardi), LindyHopperGT (Riot Games), Maksym Kapelianovych, northstarswap, omarchuk-gsc.

This is the first release for UE 5.9.

## Update Notes
### Critical warning for Data Pins users
If you were using Data Pins in your assets prior to Flow 2.2, do not upgrade directly from your current Flow Graph version to a version newer than 2.3.
Version 2.2 came with a huge Data Pins refactor, and it requires data migration occurring while loading assets.
* First update to Flow Graph 2.2 or 2.3.
* Resave all Flow Graph assets.
* Then update to the newer Flow Graph version.

### Changed Flow Graph Node constructor to GENERATED_BODY
This is a BREAKING CHANGE if you have any custom UFlowGraphNode class. Updating the code is trivial though.

## Flow Node
* Added the `Flow Identity` struct with UI customization which filters actors in the world by actor and component class. (contributed by Bargestt)
    * This is used to provide a convenient actor picker on Flow Nodes.
    * The struct needs to be manually added to a Flow Node class and read by the node's logic.
* Added PIE world context guard during `ForcePinActivation`. This fixes issues with nodes spawning actors in the wrong (editor) world. (contributed by CyaDaPaKnat)
* `GetNodeTitle()` and `GetNodeDescription()` are now called on node instances only when displaying actual node instances. This allows us to preserve the default title and description for the archetype (e.g. in the node selection list). (contributed by LindyHopperGT)
* Exposed `GetInputPins()` and `GetOutputPins()` to blueprints for automated tests. (contributed by omarchuk-gsc)
* Editor-only properties of `FFlowPin` are now wrapped with `WITH_EDITORONLY_DATA`: display name and tooltip. (contributed by Bargestt)

## Specific Nodes
* `UFlowNode_SubGraph`
    * Added support for passing data from the finished SubGraph instance to the owning SubGraph node! (contributed by LindyHopperGT)
    * Moved `TrySupplyDataPin` override in `UFlowNode_SubGraph` outside of the `WITH_EDITOR` directive. This fixes Flow Asset Params in packaged builds. (contributed by dyanikoglu)
    * Added an option to defer deinitialization of the Flow Asset instance (owned by the SubGraph node). The default behavior remains unchanged: the asset instance is fully removed when the SubGraph finishes its work. The new option keeps this instance until the owner of the SubGraph is destroyed. The new behavior can be activated globally by changing `SubGraphFinishPolicy` in the project settings, or by overriding `GetSubGraphFinishPolicy()` in a specific subclass of `UFlowAsset`. (based on a changelist by LindyHopperGT)
* `PlayLevelSequence` node now supports multiplayer via a replicated binding on `AFlowLevelSequenceActor`. (contributed by LindyHopperGT)
* `OnNotifyFromActor` now has `bExactMatch` flag allowing users to choose how the `NotifyTag` should be matched. It's similar to the `NotifyActor` node. (contributed by fede-Raider)

## Flow Node AddOns
* Moved `NodeGuid` property to `UFlowNodeBase`, so now AddOns uses this for identification. (contributed by LindyHopperGT)
    * Added code to update legacy graphs to set `NodeGuid` on AddOns.

## Flow Asset
* Diff improvements. (contributed by MaksymKapelianovych)
    * Reintroduced graph orientation changing.
    * Added highlighting of all changed properties in the Details view (as well as the focused one).
    * Added linked scrolling between Details views.
    * Fixed populating the diff entry tree. The add-on hierarchy is now preserved when an add-on's direct parent has no changes but its grandparent does.
    * Fixed focusing a node in the graph when the user clicks a diff element belonging to an add-on.
    * Made Details views read-only.
* Flow Asset validation is now called by the engine's Data Validation pipeline. (contributed by fede-Raider)

## Flow Component
* Added `bMarkFlowCategoryImportant` option to force the "Flow" category to the top of the component's Details panel. Enabled by default. (contributed by Bargestt)
* Added options to set default Gameplay Tag categories for the `IdentityTags` property. (contributed by Bargestt)
    * `DefaultIdentityTagCategories` sets the global default and is also used as the default for the `Flow Identity` filter.
    * `ComponentIdentityTagCategories` allows setting different defaults for different Flow Component classes.
* Added convenient access to Flow Component tags from the selected actor's Details panel. (contributed by Bargestt)
    * This also makes it possible to edit tags when multiple actors are selected.
    * Controlled by the `bShowFlowTagsInActorDetails` flag, enabled by default.

## Misc
* Fixed the `GetObjectsWithOuter` call in `RemoveOrphanedNodes` for UE 5.8. (contributed by fcarreiro)
* Fixed `-Wunreachable-code-loop-increment` errors on Clang. (based on a changelist by northstarswap)
