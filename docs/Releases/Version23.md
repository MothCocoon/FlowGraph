---
title: Flow 2.3 (in works)
---

This is the upcoming release. This page is updated regularly after changes are pushed to the repository.

This release includes pull requests from the community: LindyHopperGT.

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
* Added Paste option to the right-click menu for Flow Nodes. Formerly could only paste onto a Flow Node using Ctrl + V. (contributed by Riot Games: LindyHopperGT)
* Added Attach AddOn drop-down to Flow Node (and AddOn) details. Adds a more convenient method for attaching addons that is fewer-clicks per operation and a bit less hidden. This is in addition to the right-click menu on these nodes. (contributed by Riot Games: LindyHopperGT)
* Setting `UFlowNode` pointer on AddOns more reliably in editor. Updated the node pointer in editor for AddOns so that it is usable any time while in editor, can updated when addons are moved/rebuilt. (contributed by Riot Games: LindyHopperGT)
* Updated Flow Palette filters. (contributed by Riot Games: LindyHopperGT)
    *  Improved the Flow Palette filtering by category to also check superclasses of the `UFlowAsset` subclass being edited.
    * The palette filtering can apply a strict or tentative result, as it crawls up the superclass lineage.
* Refactored the IsInput/OutputConnected interface to be more useful & pin connection change event improvements.
    * These new function signatures provide multiple connections when present, and also have `FConnectedPin` structs as their container
    Updated existing calls to these functions to use the new signatures.
    * Augmented the event for `OnConnectionsChanged` to have an array of changed connections instead of the old connections array.
    * This new version `OnEditorPinConnectionsChanged` is called on all of the addons on a Flow Node as well, and has a blueprint signature.
    * This change allows nodes to be more reactive to pin connections changing (like changing their Config Text), which was possible before, but not smooth.
    * Updated some Flow Nodes (Log, FormatText) to update their config text on pin connection changes.

# Specific Flow Nodes
* Reroute updates/fixes. (contributed by Riot Games: LindyHopperGT)
    * Reroute nodes can now retype themselves if connected to a new type (and in doing so, break incompatible connections).
    * Copy/paste for data pin reroutes preserves the type of the reroute (was being lost).
* Added `PredicateRequireGameplayTags` AddOn: a data pin version. Blackboard version lives in AIFlowGraph plugin. (contributed by Riot Games: LindyHopperGT)
* Added `CompareValues` predicate (for data pins) and auto-generate data pins refactor. (contributed by Riot Games: LindyHopperGT)
    *  Refactored the auto-generate data pins code so that the CompareValues predicate can get its pins generated, duplicates disambiguated and the results queried.
    * Created CompareValues predicate, which is analogous to the Compare Blackboard Values predicate, but for data pins.
