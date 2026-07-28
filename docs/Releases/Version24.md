---
title: Flow 2.4 (in works)
---

This is the upcoming release. This page is updated regularly after changes are pushed to the repository.

This release includes pull requests from the community: Bargestt (Vasilii Bulgakov), dyanikoglu (Doğa Can Yanıkoğlu), northstarswap.

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
* Wrapped editor only properties in `FFlowPin`: display name and tooltip. (contributed by Bargestt)

## Specific Nodes
* Moved `TrySupplyDataPin` override in `UFlowNode_SubGraph` outside of WITH_EDITOR directive. This fixes Flow Asset Params in packaged game. (contributed by dyanikoglu)

## Misc
* Fixed `-Wunreachable-code-loop-increment` errors on Clang. (based on changelist contributed by northstarswap)
