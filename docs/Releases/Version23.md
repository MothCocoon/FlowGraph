---
title: Flow 2.3 (in works)
---

This is the upcoming release. This page is updated regularly after changes are pushed to the repository.

This release includes pull requests from the community: Chen-Gary (Gary Chen), dskliarov-gsc, EvanC4, LindyHopperGT (Riot Games).

This is the first release for UE 5.8, and the last for UE 5.6.

## Update Notes
### Critical warning for Data Pins users
If you were using Data Pins in your assets prior to Flow 2.2, do not upgrade directly from your current Flow Graph version to the version newer than 2.2.
Version 2.2 came with a huge Data Pins refactor and it requires data migration occuring while loading assets.
* Update first to the Flow Graph 2.2.
* Resave all Flow Graph assets.
* Continue with updating to newer Flow Graph version.

### Changed Flow Node constructor to GENERATED_BODY
This is BREAKING CHANGE. It requires updating constructors for C++ Flow Nodes. It's simple, but it will take a few minutes for large projects.

## Flow Node
* Added Paste option to the right-click menu for Flow Nodes. Formerly could only paste onto a Flow Node using Ctrl + V. (contributed by LindyHopperGT)
* Added Attach AddOn drop-down to Flow Node (and AddOn) details. Adds a more convenient method for attaching addons that is fewer-clicks per operation and a bit less hidden. This is in addition to the right-click menu on these nodes. (contributed by LindyHopperGT)
* Setting `UFlowNode` pointer on AddOns more reliably in editor. Updated the node pointer in editor for AddOns so that it is usable any time while in editor, can updated when addons are moved/rebuilt. (contributed by LindyHopperGT)
* Updated Flow Palette filters. (contributed by LindyHopperGT)
    *  Improved the Flow Palette filtering by category to also check superclasses of the `UFlowAsset` subclass being edited.
    * The palette filtering can apply a strict or tentative result, as it crawls up the superclass lineage.
* Refactored the IsInput/OutputConnected interface to be more useful & pin connection change event improvements.
    * These new function signatures provide multiple connections when present, and also have `FConnectedPin` structs as their container
    Updated existing calls to these functions to use the new signatures.
    * Augmented the event for `OnConnectionsChanged` to have an array of changed connections instead of the old connections array.
    * This new version `OnEditorPinConnectionsChanged` is called on all of the addons on a Flow Node as well, and has a blueprint signature.
    * This change allows nodes to be more reactive to pin connections changing (like changing their Config Text), which was possible before, but not smooth.
    * Updated some Flow Nodes (Log, FormatText) to update their config text on pin connection changes.

## Specific Flow Nodes
* `UFlowNode_Reroute` (contributed by LindyHopperGT)
    * Reroute nodes can now retype themselves if connected to a new type (and in doing so, break incompatible connections).
    * Copy/paste for data pin reroutes preserves the type of the reroute (was being lost).
* `UFlowNode_FormatText`
    * Fixed crash: prevent Getting the value of an invalid FFlowDataPinValue property. (contributed LindyHopperGT)
    * Fixed node not supporting the Format Text input connection. (contributed by dskliarov-gsc)
* Added `PredicateRequireGameplayTags` AddOn: a data pin version. Blackboard version lives in AIFlowGraph plugin. (contributed by LindyHopperGT)
* Added `CompareValues` predicate (for data pins) and auto-generate data pins refactor. (contributed by LindyHopperGT)
    *  Refactored the auto-generate data pins code so that the CompareValues predicate can get its pins generated, duplicates disambiguated and the results queried.
    * Created CompareValues predicate, which is analogous to the Compare Blackboard Values predicate, but for data pins.

## SaveGame support
* Refactored SaveGame integration to allow for arbitrary save data container objects. (inspired by gregorhcs)
    * Added variants of `OnGameSaved` and `OnGameLoaded`: accepting `TArray<FFlowComponentSaveData>` and `TArray<FFlowAssetSaveData>` as input parameters.
    * Added creating a transient `UFlowSaveGame` object in the new `OnGameLoaded` variant. This way, the plugin can keep operating on `UFlowSaveGame` as a container for these 2 arrays, but projects can store these arrays whenever they want.

## Misc
* In `UFlowGraphSettings`, all occurences of hard refences `TSubclassOf` have been changed to `TSoftClassPtr`. In general, TSubclassOf should be avoided to prevent automatic loading of unnecessary assets. In case of plugins, using hard reference was using issue with loading some assets defined in other plugins. Since `UFlowGraphSettings` could load assets before other plugin is loaded. (contributed by Chen-Gary)
* Fix crash when `OnMapOpened` delegate fires after `SLevelEditorFlow` destruction. (contributed by EvanC4)
