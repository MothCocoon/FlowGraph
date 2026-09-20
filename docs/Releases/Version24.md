---
title: Flow 2.4 (in works)
---

This is the upcoming release. This page is updated regularly after changes are pushed to the repository.

This release includes pull requests from the community: Bargestt (Vasilii Bulgakov), dyanikoglu (Doğa Can Yanıkoğlu), , LindyHopperGT (Riot Games), northstarswap, omarchuk-gsc.


This is the first release for UE 5.9.

## Update Notes
### Critical warning for Data Pins users
If you were using Data Pins in your assets prior to Flow 2.2, do not upgrade directly from your current Flow Graph version to the version newer than 2.3.
Version 2.2 came with a huge Data Pins refactor and it requires data migration occuring while loading assets.
* Update first to the Flow Graph 2.2 or 2.3.
* Resave all Flow Graph assets.
* Continue with updating to newer Flow Graph version.

### Changed Flow Node constructor to GENERATED_BODY
This is BREAKING CHANGE if you have any custom UFlowGraphNode class. Updating the code is trivial though.

## Flow Node
* Call `GetNodeTitle()` and `GetNodeDescription()` on node instances only when displaying actual node instances. This allows us to preserve the default title and description for the archetype (e.g. in the node selection list). (contributed by LindyHopperGT)
* Exposed `GetInputPins()` and `GetOutputPins()` to blueprints for automatic tests. (contributed by omarchuk-gsc)
* Editor-only properties of `FFlowPin` wrapped with `WITH_EDITORONLY_DATA`: display name and tooltip. (contributed by Bargestt)

## Specific Nodes
* `UFlowNode_SubGraph` 
    * Added support for passing data from the finished SubGraph instance to the owning SubGraph node! (contributed by LindyHopperGT)
    * Moved `TrySupplyDataPin` override in `UFlowNode_SubGraph` outside of WITH_EDITOR directive. This fixes Flow Asset Params in packaged game. (contributed by dyanikoglu)
* `PlayLevelSequence` node now supports multiplayer via replicated binding on `AFlowLevelSequenceActor`. (contributed by LindyHopperGT)

## Misc
* Fixed `-Wunreachable-code-loop-increment` errors on Clang. (based on changelist contributed by northstarswap)
