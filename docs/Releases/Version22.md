---
title: Flow 2.2
---

February 28, 2026.

This release includes pull requests from the community: ameaninglessname, bohdon (Bohdon Sayre), Danamarik, DemonViglu, GreggoryAddison-AntiHeroGameStudio (Greggory Addison), gregorhcs (Gregor Sönnichsen), j0tt (Jeff Ott), LindyHopperGT, Maksym Kapelianovych, Numblaze, Rhillion, SilentGodot (Mor Ohana).

This is the first release for UE 5.7, and the last for UE 5.5.

* [Flow 5.7](https://github.com/MothCocoon/FlowGraph/releases/tag/v2.2-5.7)
* [Flow 5.6](https://github.com/MothCocoon/FlowGraph/releases/tag/v2.2-5.6)
* [Flow 5.5](https://github.com/MothCocoon/FlowGraph/releases/tag/v2.2-5.5)

Flow Game (sample project) releases
* [Flow Game 5.7](https://github.com/MothCocoon/FlowGame/releases/tag/v2.2-5.7)
* [Flow Game 5.6](https://github.com/MothCocoon/FlowGame/releases/tag/v2.2-5.6)
* [Flow Game 5.5](https://github.com/MothCocoon/FlowGame/releases/tag/v2.2-5.5)

## Post-update actions
### Flow plugin is now DISABLED by default
Change suggested Riot Games, but the idea isn't new. This plugin tends to be part of the codebase shared between projects in studios. In that case, it is desired to have this plugin as part of a shared codebase or repository, but it's not desired to have the plugin enabled for every project.

The easiest way to re-enable the plugin in your project is to edit your .uproject file and add this section to the Plugins list.
``` 
{
    "Name": "Flow",
    "Enabled": true
},
```

### Resave all Flow Assets
This version automatically updates data related to Data Pins. Many of your graphs will be marked as dirty, even if you don't use this feature. It's best to resave all graphs at once.

If you're using Data Pins, please:
* Read about related changes below.
* Smoke test if all data has been properly updated.

## Flow Asset
* Added `UFlowAsset::GatherNodesConnectedToAllInputs` helper function. (contributed by Riot Games)
* Fixed the issue with the unwanted multiplication of the Comment node while copy-pasting it.
* Made `UFlowAsset::IsBoundToWorld()` const. (contributed by gregorhcs)
* Removed the old way of injecting own Main Menu items into the Flow Asset editor, by using `FExtensibilityManager`. The deprecation message tells the programmer how exactly the code should be updated.

## Flow Subsystem
* Exposed `UFlowSubsystem` runtime state fields to subclasses. (contributed by gregorhcs)

## Flow Node
* Trigger Outputs deferred while processing an Input Trigger (contributed by Riot Games: LindyHopperGT)
    * This is a change to the core Flow input triggering logic to fix a category of sequencing bugs from the previous behavior. It would immediately fully process a triggered input and so on down the chain of Flow Nodes, without allowing the current Flow Node to finish executing, this caused a whole category of problems where the node wasn't able to finish its execution before being interrupted by a retirgger (from downstream) and AddOns wouldn't execute at the same time as their owning flow node reliably.
    * Now, Flow Asset will queue any triggers generated while processing a trigger, and flush them when ending the processing of that trigger.
    * Also integrated the debugger queued trigger caching mechanism to use the same system.
    * Subclasses of `UFlowAsset` that do their own deferred asset triggering can disable this feature, except for the debugger portion, which is still processed using the `UFlowAsset` queue.
* Improvements to node validation. (contributed by Numblaze)
    * Added support for node validation via Blueprints using a new `K2_ValidateNode()` function.
    * Blueprints can now log validation messages (errors, warnings, notes) using new `LogValidation` functions exposed on `UFlowNodeBase`.
    * Flow Asset Validation Logs All Severities. Before the fix, validation logged only warnings and notes if at least one error was present. After fix: UFlowAsset validation logic ensures that all node validation messages are logged, regardless of their severity
    * The node validation code has been moved from UFlowNode to UFlowNodeBase to allow validation of AddOns.
* AddOns
    * AddOns can now include data pins, which show up as pins on their owning node. (contributed by Riot Games: LindyHopperGT)
    * Updated the ForEachAddOn templates to support a parameter to control how the function should recurse into child addons (or not). (contributed by Riot Games: LindyHopperGT)
    * Extracted `UFlowNodeAddOn::FindOwningFlowNode()` functionality into a function. (contributed by Riot Games: LindyHopperGT)
    * Added AddOn descriptions to node descriptions, with an editor settings option to disable. (contributed by Rhillion)
* Exposed methods to `public` to enable Flow Node status display in runtime debuggers. (contributed by gregorhcs)
* Added option to tone down on-screen error messages. (contributed by gregorhcs)
* Added support for custom overlay icons for Flow Nodes. (contributed by Riot Games: LindyHopperGT)
    * `FlowNodeBase::GetCornerIcon()` allows you to easily define an icon to draw in the top-right of the Node and handles positioning for you.
    * `FlowNodeBase::GetOverlayIcons()` allows you to define any number of overlay icons and custom positioning.
    * Added code in SFlowGraphNode to query and draw the custom overlay icons.
* Added a virtual method to allow saving non-executing flow nodes. (contributed by Riot Games: LindyHopperGT)
* Added transaction for changing the signal mode. (contributed by Maksym Kapelianovych)
* Added an `IsFinishedState()` classifier function for `EFlowNodeState`, to error-proof checking node state for "finished" states. (contributed by Riot Games: LindyHopperGT)
* Made `CanFinishGraph` a BlueprintImplementableEvent. (contributed by Riot Games: EvanC4)
* Added `GetRandomSeed()`. The default version uses the hash from the node's GUID. This can be overridden in subclasses (which we do) to any implementation that suits the client code. (contributed by Riot Games: LindyHopperGT)
* `OnNodeDoubleClicked` logic moved to the Graph Node itself, allowing for overriding the default logic.
* Added `FLOW_API` to `GetFlowPinType()` functions to allow them to be called from extension plugins. (contributed by Riot Games: LindyHopperGT)
* Added enum-based constructors for `FFlowDataPinResult` structs. (contributed by Riot Games: LindyHopperGT)
* Renamed `FFlowNamedDataPinOutputPropertyCustomization`, which is now also used for input pins. (contributed by Riot Games: LindyHopperGT)
* Fixed some Datapin result logic. (contributed by DemonViglu)
* Fixed rare crash in LogError(..) caused by invalid flow node self or owner. (contributed by gregorhcs)

# Specific Flow Nodes
* Added `bUseAsyncSave` option to `UFlowNode_Checkpoint`. This option can be changed by adding an entry to the `DefaultGame.ini`. (based on changelist submitted by j0tt)
* Fixed `UFlowNode_OnNotifyFromActor` node not using identity tag match type. (contributed by SilentGodot)
* `UFlowNode_Reroute` now supports Data Pins. Previously, only Exec pins were supported. (contributed by Riot Games: LindyHopperGT)
* Added `UFlowNode_FormatText` that formats text using input pins and the FText formatting engine. (contributed by Riot Games: LindyHopperGT)
* Changed the `UFlowNode_Log` to format text (a la `UFlowNode_FormatText`) to generate its logged output string. (contributed by Riot Games: LindyHopperGT)
* Changed `UFlowNode_Log` to inherit from `UFlowNode_DefineProperties`, so that it can have input properties added on the instance. (contributed by Riot Games: LindyHopperGT)
* Renamed `UFlowNode_DefineProperties::OutputProperties` to `NamedProperties`, so that it can be used as the super class for `UFlowNode_FormatText`. (contributed by Riot Games: LindyHopperGT)
* Fixed `UFlowNode_ExecuteComponent` to handle injected components correctly in validation. (contributed by Riot Games: LindyHopperGT)
* Fixed `UFlowNode_ExecuteComponent` to conform to the new style of pin generation, now using ContextPins (the old method didn't work after a refactor with flow graph node reconstruction). (contributed by Riot Games: LindyHopperGT)
* Updated `UFlowNode_ExecuteComponent` to allow the component to supply data pin output values. (contributed by Riot Games: LindyHopperGT)
* Updated FlowNode_Branch: AND/OR selectability and BranchCase support. (contributed by Riot Games: LindyHopperGT)
    * Branch node can now be configured with AND/OR for the top-level combination rule (default is still AND).
    * Branch node can now have BranchCase AddOns at its root addon level, these will be evaluated and can trigger prior to evaluating the root predicates (which serve as the "else" case for this sort of configuration).
    * BranchCase adds a switch/case-like capability where each BranchCase can have its own output exec pin, if that case's predicates pass
* Switch Case AddOns' names are authorable. (contributed by Riot Games: LindyHopperGT)
    * If you set the name, it will match the pin 1:1 (unless duplicates).
    * Default case name (and pin name) is "Case".
    * Duplicates are disambiguated with numbers appended.
    * Sets the title to the pin name, so it's easier to match the pin to the case

## Data Pins refactor and improvements
Benefits
* Arrays are now supported for data pins.
* Cross-conversion between like-types is supported (eg, int<->float, tag<->tag container, etc.) when connecting pins, including all standard data pin types convertible to strings (primarily for logging and dev).
* User-addable type framework. It's definitely an advanced feature, but it is possible to extend Flow via your plugin to add new data pin types.
* Less boilerplate for nodes, etc. that interact with data pins.
* "Mostly" backward compatible with the old `FFlowDataPinProperty` classes and old TryResolveDataPin functions. The data side should auto-convert your flow graph pins without hand-fixes. It "should" work with the older classes and API in blueprint and Flow Graph data-space, but you will want to convert your pin properties to the new wrapper classes `FFlowDataPinValue` and the new, unified results signature.  There's a suite of new blueprint functions to make using data pins in blueprint Flow Nodes, etc. easier. The backward compatibility support is designed to preserve authored data through the transition process, and it worked for internal Riot Games use cases. Riot intends to preserve the legacy support code for one UE minor release (~3 months) to allow projects to update.

Detailed changelog
* Incremented `UFlowGraph::GraphVersion` to 2, created a data migration function (UpgradeAllFlowNodePins).
* `FlowPinType` namespace templates for the bulk of the "Supply/Resolve" pipeline support for data pins.
* Updated standard `FFlowPinType` and `FFlowDataPinValue` subclasses to use them with the new resolve pipeline.
* Reworked FlowSchema's pin compatibility checks to be more orderly, simpler, and data-driven connectivity.
* Created policies for schema connectivity rules for the standard types.
* Updated `FFlowNamedDataPinProperty` to use `FFlowDataPinValue` as its property payload (including migrate functions from the old data).
* Updated` FlowDataPinBlueprintLibrary` with new auto-converts and functions to support data pin manipulation in blueprint.
* Updated `FlowNodeBase` with the new Resolve pathway entry points & related refactors.
* Updated `FlowNode` with the new Supply pathway entry points & related refactors.
* Updated `FlowPin` to deprecate Enum PinType and add ContainerType (for Array data pins) and PinTypeName (the replacement for PinType Enum).
* Removed overrides `TrySupplyDataPinAs...` (now replaced by general version).
* Removed `TrySupplyDataPinAs...` variants from the IFlowDataPinValueSupplierInterface (leaving only the general replacement).
* Ported `TryResolveDataPinAs...` specialized versions to use the general version internally.
* Adapted uses of `TryResolveDataPinAs...` to the general version TryResolveDataPin.
* FlowNode (and AddOn) details customizations now inherit from `TFlowDataPinValueOwnerCustomization`, which adds a RequestRebuild() for rebuilding flow node details in a way that correctly rebuilds the `FFlowDataPinValue` customizations.
* Refactored FlowAsset's automatic pin generation mechanism to be cleaner, simpler, and work with the new system.
* Details customizations for `FFlowDataPinValue` specific subclasses.
* Details customization for `IFlowDataPinValueOwnerInterface` implementers (via template).
* Added some details customization rebuild hooks into `IFlowDataPinValueOwnerInterface` to support `FFlowDataPinValue` details rebuilding.
* Updated some flow nodes to use new Resolve functions (eg, Log, DefineProperties, Start, FormatText, etc.).
* Slighly reworked `FFlowPinSubsystem` API.
* Created new test classes and assets in `FlowGraph_DataPinsTest`.
* Converted FlowDataPinValueSupplierInterface to C++ only. The blueprint functionality for this has been removed.
* Added FlowNode_BlueprintDataPinSupplierBase. This is a thin C++ class that exposes TrySupplyDataPin override capability for blueprint if they need it.
* Flow Asset Params improvements.
    * Can now statically assign AssetParams to use on a Subgraph node for the associated Flow Asset.
    * Can also dynamically source the AssetParams to use via an input data pin for the Subgraph node.
    * If the Subgraph node pins are connected, the connected supplier is preferred over the `FlowAssetParams`. Otherwise, the FlowAssetParams object (if chosen) will supply the value (this is the new part).
    * If no assigned FlowAssetParams, the Start node in the subgraph's default values are used (this was pre-existing behavior).
    * Fixed some code in and around sourcing subgraph Start node pin values to allow this sourcing to work properly.
    * Fixed some issues preventing the tooltip inspection for data pins in subgraphs from correctly finding the correct source value.
* For a reroute node, connecting to a new type with a data pin will change its type and break any incompatible connections to the reroute. Also fixed copy/paste reroute nodes losing their type.

(Contributed by Riot Games: LindyHopperGT)

## Breakpoint debugger
* Functional improvements. (based on the changelist submitted by Maksym Kapelianovych)
    * Open the Flow Asset editor on breakpoint hit instead of just freezing.
    * Added Debug Menu allowing to enable/disable/remove all breakpoints in the graph.
    * Added initial support for filtering out inspected instances by world. This is primarily for multiplayer to separate server instances from client ones.
    * Select the correct flow asset instance, instead of the first one spawned with the same name in multiplayer.
* Functional improvements. (Contributed by Riot Games: LindyHopperGT)
    * Debugger will stop at breakpoints instead of continuing to trigger.
    * Data Pin values are visible in the debugger as tooltips.
* Visual improvements. (submitted by Maksym Kapelianovych)
    * In the top right corner of the graph, the PIE status will be displayed, based on the selected world/instance.
    * Visual changes for breadcrumbs (added trailing delimiter, background color, max width).
    * Clicking on a breadcrumb now will focus the `SubGraph` node that created the inspected instance.
    * The button `Go to Parent` was removed from the toolbar (the same can be achieved with breadcrumbs), but the command was kept to retain the ability to use a shortcut for this action.

## Flow Search
* Updated Search UX, displays the source(s) where the search terms were found.
* Multi-asset search capability (same time or all flow assets).
* Increased number of search sources (Eg, tooltips or property values).
* Filters to control which search sources to include.
* Improved inline object, struct and subgraph search capabilities.
* Caching search metadata for each Flow Asset, for faster re-searches, updated when the asset changes.
(Contributed by Riot Games: LindyHopperGT)

## Misc
* Made `UFlowComponent::NotifyFromGraph` a BlueprintCallable. (contributed by Riot Games: EvanC4)
* Deprecated redundant `UFlowSettings::Get()`, it's cleaner to call `GetDefault<UFlowSettings>()`.
* Deprecated redundant `UFlowGraphSettings::Get()`, it's cleaner to call `GetDefault<UFlowGraphSettings>()`.
* Deprecated redundant `UFlowGraphEditorSettings::Get()`, it's cleaner to call `GetDefault<UFlowGraphEditorSettings>()`.
* Fixed an issue where breakpoint overlay brushes weren't showing properly due to deprecated code in 5.6. (contributed by Greggory Addison)
* Preventing a crash while using Ctrl + Shift + X shortcut of the `BlueprintAssist` plugin. (contributed by Numblaze)
* Exposed some methods related to custom blueprint nodes. (contributed by Danamarik)
* Non-unity build fixes. (contributed by bohdon)
* Fixed: build error when making an installed build. (contributed by ameaninglessname)
* Removed enforcing cpp20 in the `FlowEditor.Build.cs`. This an engine default since UE 5.3.
* Code style update
    * Using style for multi-line comments for all comments above method and property declarations. Sentences end with a dot.
    * Using only a single asterisk to begin a multi-line comment as this lets us align text better.
    * Update class, structs, enums and interfaces descriptions to use the style of multi-line comment.
    * Removed parentheses in comments as this actually reduces cognitive load (while using multi-line comment style), I guess?
    * Removed empty line between copyright and "#pragma once" to make these lines more compact.
