---
title: Asset Search
---

Feature based on the engine plugin added in UE 4.26. The plugin is marked as beta, probably because of the search performance.

## Using the feature
- Make sure the `Asset Search` plugin is enabled in your project.
- Open the `Tools/Search` tab.
- If you never used this Search yet, check the status of "missing indexed assets" in the bottom right corner. Click it, and it gonna index all assets supported by the Asset Search. It can take many minutes, depending on your project size.
- Wait until indexing finishes and the status in the bottom left corner is `Ready` again.

## Limiting Search by asset type
If you'd merge the provided engine modification, you can exclude specific asset types from Asset Search indexing. On the project level (via Project Settings) or user level (Editor Preferences).
* [Added Search Roles to Asset Search settings as a huge time-saver](https://github.com/EpicGames/UnrealEngine/pull/9332)

Here's the list of asset types supported out of the box, hardcoded in the engine's code. (as of UE 5.0)
![image](https://user-images.githubusercontent.com/5065057/175774889-bf4b3ed4-ba6b-47e2-af17-10320e3da8c1.png)

## Useful engine modifications
* [Jump from Asset Search result to the node in any graph editor!](https://github.com/EpicGames/UnrealEngine/pull/9882) - set the `ENABLE_JUMP_TO_INNER_OBJECT` value to 1 after integrating this change.
* [Asset Search: added option to run search on single asset, with Search Browser opened as asset editor tab](https://github.com/EpicGames/UnrealEngine/pull/9943)