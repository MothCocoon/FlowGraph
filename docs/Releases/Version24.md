---
title: Flow 2.4 (work in progress)
---

This is the upcoming release. This page is updated regularly after changes are pushed to the repository.

This release includes pull requests from the community: AMBXJianPing (Fong Jian Ping), Bargestt (Vasilii Bulgakov), CyaDaPaKnat, dskliarov-gsc, dyanikoglu (Doğa Can Yanıkoğlu), fcarreiro (Facundo), fede-Raider (Federico Ciardi), LindyHopperGT (Riot Games), Maksym Kapelianovych, northstarswap, omarchuk-gsc.

This is the first release for UE 5.9.

## Update Notes
### Critical warning for Data Pins users
If you were using Data Pins in your assets prior to Flow 2.2, do not upgrade directly from your current Flow Graph version to a version newer than 2.3.
Version 2.2 came with a huge Data Pins refactor, and it requires data migration occurring while loading assets.
* First update to Flow Graph 2.2 or 2.3.
* Resave all Flow Graph assets.
* Then update to the newer Flow Graph version.

### Changes potentially breaking C++ compilation
* The `ActivationState` property of Flow Asset is now private. Access it through the `GetActivationState()` getter.
* Changed the Flow Graph Node constructor to GENERATED_BODY.
    * This is a breaking change if you have custom UFlowGraphNode classes. Updating the code is trivial though.
* Detail Customizations are now organized into subfolders.
    * This is a breaking change if you made changes to these classes.

### By default, Data Validation runs on saving a Flow Asset
This is the result of hooking up Flow Asset validation to the engine's built-in validation. Thanks to this, many issues can be detected early on.
However, this might be annoying to users.
* You can disable this behavior in the project settings by unchecking the `bValidateAssetOnSave` flag.
* You can disable this in a specific `UFlowAsset` subclass by overriding the `ShouldValidateOnSave()` method.

## Flow Asset
* Saving graph state to SaveGame now iterates nodes connected to Custom Inputs. Previously, the code only iterated over nodes connected to the Start node. (contributed by dskliarov-gsc)
* New feature: Create a SubGraph asset from selected nodes! (contributed by LindyHopperGT)
    * You can find this new option in the node context menu, in the `Organization` section.
    * Creating a new asset was tested under Perforce and Git. It should work with other VCS but hasn't been tested yet.
* Flow Asset validation improvements.
    * Validation logic is now called by the engine's Data Validation pipeline. (contributed by fede-Raider)
    * Added ways to disable Data Validation on saving a Flow Asset. (based on a changelist by dskliarov-gsc)
    * Added a check whether a pin connects to zombie nodes. (contributed by LindyHopperGT)
    * Improved AddOn validation, i.e. checking whether it's allowed to place multiple AddOns of the same class on the node. (contributed by LindyHopperGT)
* Added an option to reject the entire Flow Asset instance when attempting to create it.
    * The new `UFlowAsset::CanBeInstanced()` method checks if all node templates are valid: a node won't be in the packaged game if its class is marked as Deprecated. Previously, such a missing node caused a silent failure in the packaged game. (contributed by LindyHopperGT)
    * The runtime check is a failsafe, as such issues should be detected by Data Validation thanks to the changes to `UFlowAsset::IsNodeOrAddOnClassAllowed`. (contributed by LindyHopperGT)
    * By default, this new behavior is disabled. This is the only way to ensure that released games and player save games are not affected.
    * The new behavior can be enabled by checking the `bRejectFlowInstancesWithInvalidNodes` flag.
* Diff improvements. (contributed by MaksymKapelianovych)
    * Reintroduced graph orientation changing.
    * Added highlighting of all changed properties in the Details view (as well as the focused one).
    * Added linked scrolling between Details views.
    * Fixed populating the diff entry tree. The AddOn hierarchy is now preserved when an AddOn's direct parent has no changes but its grandparent does.
    * Fixed focusing a node in the graph when the user clicks a diff element belonging to an AddOn.
    * Made Details views read-only.
* The debugger toolbar has been revamped. It now supports more ways of breakpoint-based debugging. (contributed by LindyHopperGT)
* Fixed a bug where options in the Debug menu were greyed out and unusable.
* Flow Asset Params improvements.
    * Child parameter assets are now synchronized when their parent Start-node parameter names change. (contributed by LindyHopperGT)
    * Fixed: Params assets were marked as dirty after any change in a related Flow Asset.
    * QoL fix: added editor customizations to avoid displaying the " - Flow DataPin Value" suffix on every Data Pin type in the Flow Asset editor.
* Refactored Flow Search. (contributed by LindyHopperGT)
    * Moved shared search semantics from the `FindInFlow` class into the `FFlowSearch` service. The extracted code can be called from external tools.
    * Added Pin Name search controls and progress feedback to the FindInFlow UI.
    * Fix: keep newly added filters enabled by default.

## Flow Node
* New: Added an initial implementation of network replication directly in Flow Nodes.
* Pin-related improvements.
    * Added tooltip comparison when checking if Pins match. This fixes the automatic refresh of displayed tooltips after a tooltip has been modified. (contributed by CyaDaPaKnat)
    * Exposed `GetInputPins()` and `GetOutputPins()` to blueprints for automated tests. (contributed by omarchuk-gsc)
    * Editor-only properties of `FFlowPin` are now wrapped with `WITH_EDITORONLY_DATA`: display name and tooltip. (contributed by Bargestt)
    * Fixed a slow `FPinRecord` memory leak in non-Shipping builds. Added `FPinRecordBuffer` that now limits the number of records to 64, instead of expanding infinitely. (contributed by AMBXJianPing)
* Backward connections between nodes that overlap horizontally are no longer routed around them, and nodes can override their title padding with `GetTitleBorderMargin()`. (contributed by LindyHopperGT)
* Added the `Flow Identity` struct with a UI customization that filters actors in the world by actor and component class. (contributed by Bargestt)
    * This is used to provide a convenient actor picker on Flow Nodes.
    * The struct needs to be manually added to a Flow Node class and read by the node's logic.
* Added a PIE world context guard during `ForcePinActivation`. This fixes issues with nodes spawning actors in the wrong (editor) world. (contributed by CyaDaPaKnat)
* `GetNodeTitle()` and `GetNodeDescription()` are now called on node instances only when displaying actual node instances. This allows us to preserve the default title and description for the archetype (e.g. in the node selection list). (contributed by LindyHopperGT)
* Node descriptions have been revamped and greatly improved. (contributed by LindyHopperGT)
* Added documentation methods for LLM agents. All nodes now implement `GetAgentDoc()`. (contributed by LindyHopperGT)

## Flow Node AddOns
* Moved the `NodeGuid` property to `UFlowNodeBase`, so AddOns now use it for identification. (contributed by LindyHopperGT)
    * Added code to update legacy graphs to set `NodeGuid` on AddOns.
* Improved copy-pasting of AddOns.
    * Reworked pasting so multiple nodes with AddOns can be copy-pasted. Previously, the editor could crash in some cases.
    * Added validation of AddOn attachment during copy-paste. (contributed by LindyHopperGT)
    * The graph update lock is now released when nothing is imported. (contributed by LindyHopperGT)
* Visual improvements. (contributed by LindyHopperGT)
    * AddOn content now uses the available width of its containing node.
    * Nested AddOns now have a visible selection outline when selected through the graph details UI.
* Added an optional mode to show all AddOn details in their Flow Node's details. (contributed by LindyHopperGT)
    * You need to enable `bShowAddOnsInNodeDetails` in Editor Preferences to activate this feature.
* Fixed a case where changing the `UFlowGraphNode` class caused the loss of all AddOns.

## Specific Nodes
* `UFlowNode_SubGraph`
    * Added support for passing data from the finished SubGraph instance to the owning SubGraph node! (contributed by LindyHopperGT)
    * Moved the `TrySupplyDataPin` override in `UFlowNode_SubGraph` outside of the `WITH_EDITOR` directive. This fixes Flow Asset Params in packaged builds. (contributed by dyanikoglu)
    * Added an option to defer deinitialization of the Flow Asset instance (owned by the SubGraph node). The default behavior remains unchanged: the asset instance is fully removed when the SubGraph finishes its work. The new option keeps this instance until the owner of the SubGraph is destroyed. The new behavior can be activated globally by changing `SubGraphFinishPolicy` in the project settings, or by overriding `GetSubGraphFinishPolicy()` in a specific subclass of `UFlowAsset`. (based on a changelist by LindyHopperGT)
    * Added the `Reuse` option to `SubGraphFinishPolicy`. A finished SubGraph instance is kept and restarted when the SubGraph node runs again, instead of creating a new instance. (contributed by LindyHopperGT)
    * Components injected into actors can now be shared between a SubGraph and its parent graphs. Such a component is removed only after its last user releases it. (contributed by LindyHopperGT)
* The `PlayLevelSequence` node now supports multiplayer via a replicated binding on `AFlowLevelSequenceActor`. (contributed by LindyHopperGT)
* `OnNotifyFromActor` now has a `bExactMatch` flag, allowing users to choose how the `NotifyTag` should be matched. It's similar to the `NotifyActor` node. (contributed by fede-Raider)
* Placing a Comment node immediately starts editing its text. This saves users the extra action of selecting the Comment node.

## Flow Component
* Added the `bMarkFlowCategoryImportant` option to force the "Flow" category to the top of the component's Details panel. Enabled by default. (contributed by Bargestt)
* Added options to set default Gameplay Tag categories for the `IdentityTags` property. (contributed by Bargestt)
    * `DefaultIdentityTagCategories` sets the global default and is also used as the default for the `Flow Identity` filter.
    * `ComponentIdentityTagCategories` allows setting different defaults for different Flow Component classes.
* Added convenient access to Flow Component tags from the selected actor's Details panel. (contributed by Bargestt)
    * This also makes it possible to edit tags when multiple actors are selected.
    * Controlled by the `bShowFlowTagsInActorDetails` flag, enabled by default.

## Misc
* The plugin code went through a series of code audits.
    * Anyone can run the same audits on their own code. The agent skills are available in the [CodeAudits](https://github.com/MothCocoon/CodeAudits) repository.
* Weak pointer audit across the plugin.
    * Each weak pointer is now resolved once, instead of `IsValid()` followed by `Get()`, `->` or `Pin()`. This removes the redundant cost of dereferencing multiple times.
    * Added null checks where a weak pointer or a nullable accessor was dereferenced without one. A missing object is now skipped instead of crashing.
* `UFlowAsset::ExpectedOwnerClass` and `UFlowNodeBase::ReplacedBy` are now `TSoftClassPtr`, so cooked assets no longer load the referenced classes.
    * `GetExpectedOwnerClass()` and `GetReplacedByClass()` now return the soft pointer. Code calling them needs to load or resolve the class.
    * As a general rule, runtime properties shouldn't use TSubclassOf, which is a hard reference.
* Fixed the `GetObjectsWithOuter` call in `RemoveOrphanedNodes` for UE 5.8. (contributed by fcarreiro)
* Fixed `-Wunreachable-code-loop-increment` errors on Clang. (based on a changelist by northstarswap)
