// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"

#include "Types/FlowEnumUtils.h"
#include "Types/FlowArray.h"

// NOTE (gtaylor) The choice of which EFlowGameplayTagMapExpandPolicy to use will be informed by the map's tolerance
//  for memory vs. lookup performance.  If speed is not a concern, then fully expanding with AllSubtags can
//  make for a single-tech lookup.  If memory is more of a concern, then NoExpand will store the minimal information
//  in the map keys (potentially requiring multiple parent searches in TryLookupGameplayTagKey).  If only the leaf tags
//  will be used for lookup, then LeafSubtags expansion policy is a good option.

UENUM()
enum class EFlowGameplayTagMapExpandPolicy : int8
{
	AllSubtags,			// Apply the payload to all of the tag's child tags
	LeafSubtags,		// Apply the payload to the tag's leaf child tags
	RemoveSubtags,		// Remove all of the keys in the result map of the tag's child tags
	NoExpand,			// Only apply the payload patch to the tag, make no changes to it's child tags

	Max UMETA(Hidden),
	Invalid = -1 UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowGameplayTagMapExpandPolicy);

namespace FlowMap
{
	// Utility functions for utilizing FGameplayTags as a key in a TMap.
	//  Expected to be wrapped by the client code to hide some of the details in these function signatures.

	template <EFlowGameplayTagMapExpandPolicy ExpandPolicy, typename TPayload, int32 ExpectedSourceKeyCountMax = 32>
	void PatchGameplayTagMap(
		const TMap<FGameplayTag, TPayload>& PatchSourceMap,
		TMap<FGameplayTag, TPayload>& InOutPatchedMap)
	{
		checkf(
			&PatchSourceMap != &InOutPatchedMap,
			TEXT("We could make this case work, but it would require a temp caching anyway, so letting the caller make the copy if they want to expand in-line"));

		FlowArray::TInlineArray<FGameplayTag, ExpectedSourceKeyCountMax> PatchMapKeys;
		PatchSourceMap.GenerateKeyArray(PatchMapKeys);

		FLOW_ASSERT_ENUM_MAX(EFlowGameplayTagMapExpandPolicy, 4);

		constexpr bool bProcessSubtags = (ExpandPolicy != EFlowGameplayTagMapExpandPolicy::NoExpand);

		// Only sort the keys if we will be processing the subtags

		if constexpr (bProcessSubtags)
		{
			PatchMapKeys.StableSort([](const FGameplayTag& Tag0, const FGameplayTag& Tag1)
			{
				// Sort the keys to apply in order from least specific to most specific

				return Tag0.GetGameplayTagParents().Num() < Tag1.GetGameplayTagParents().Num();
			});
		}

		for (const FGameplayTag& PatchKeyTag : PatchMapKeys)
		{
			const TPayload& PatchPayload = PatchSourceMap.FindChecked(PatchKeyTag);

			// First, patch the payload in the target map

			InOutPatchedMap.Add(PatchKeyTag, PatchPayload);

			// Now apply the payload to child tag keys in the map

			if constexpr (bProcessSubtags)
			{
				const UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();
				const FGameplayTagContainer TagAndChildrenContainer = TagsManager.RequestGameplayTagChildren(PatchKeyTag);

				for (auto ChildTagIt = TagAndChildrenContainer.CreateConstIterator(); ChildTagIt; ++ChildTagIt)
				{
					const FGameplayTag& ChildTag = *ChildTagIt;

					if constexpr (ExpandPolicy == EFlowGameplayTagMapExpandPolicy::AllSubtags)
					{
						// Replace all child tag entries (if any) with the patch source tag's payload
						InOutPatchedMap.Add(ChildTag, PatchPayload);
					}

					if constexpr (ExpandPolicy == EFlowGameplayTagMapExpandPolicy::LeafSubtags)
					{
						// BB (gtaylor) Is there a lighter-weight way to ask if a tag is a leaf tag?

						const FGameplayTagContainer ChildChildTags = TagsManager.RequestGameplayTagChildren(PatchKeyTag);
						const bool bIsChildALeafTag = (ChildChildTags.Num() == 0);

						if (bIsChildALeafTag)
						{
							// Replace only leaf child tag entries (if any) with the patch source tag's payload
							InOutPatchedMap.Add(ChildTag, PatchPayload);
						}
					}

					if constexpr (ExpandPolicy == EFlowGameplayTagMapExpandPolicy::RemoveSubtags)
					{
						// Remove all subtag mappings in the map
						InOutPatchedMap.Remove(ChildTag);
					}
				}
			}
		}
	}

	// (const) Lookup function, which works on a gameplaytag-keyed map
	// it can crawl up the tag ancestry chain to allow general keys to apply to sub-tags
	template <typename TPayload>
	const TPayload* TryLookupGameplayTagKey(
		const FGameplayTag& KeyTag,
		const TMap<FGameplayTag, TPayload>& GameplayTagToPayloadMap,
		const FGameplayTag& KeyTagBase = FGameplayTag::EmptyTag,
		int32 ParentTagSearchDepthMax = 0)
	{
		check(ParentTagSearchDepthMax >= 0);
		check(
			KeyTagBase == FGameplayTag::EmptyTag ||
			KeyTag == KeyTagBase ||
			KeyTag.MatchesTag(KeyTagBase));

		const TPayload* FoundPayload = GameplayTagToPayloadMap.Find(KeyTag);

		if (!FoundPayload &&
			ParentTagSearchDepthMax > 0 &&		
			KeyTag != KeyTagBase)
		{
			// Recurse to direct parent tag, decrementing the allowed search depth

			const FGameplayTag DirectParentTag = KeyTag.RequestDirectParent();
			const int32 NewParentTagSearchDepthMax = ParentTagSearchDepthMax - 1;

			return TryLookupGameplayTagKey<TPayload>(DirectParentTag, GameplayTagToPayloadMap, KeyTagBase, NewParentTagSearchDepthMax);
		}

		return FoundPayload;
	}

	// (mutable) Lookup function, which works on a gameplaytag-keyed map
	// it can crawl up the tag ancestry chain to allow general keys to apply to sub-tags
	template <typename TPayload>
	TPayload* TryLookupGameplayTagKey(
		const FGameplayTag& KeyTag,
		TMap<FGameplayTag, TPayload>& GameplayTagToPayloadMap,
		const FGameplayTag& KeyTagBase = FGameplayTag::EmptyTag,
		int32 ParentTagSearchDepthMax = 0)
	{
		// Non-const map signature uses the same lookup code as the const version

		return
			const_cast<TPayload*>(
				TryLookupGameplayTagKey<TPayload, ParentTagSearchDepthMax>(
					KeyTag,
					*const_cast<const TMap<FGameplayTag, TPayload>*>(&GameplayTagToPayloadMap),
					KeyTagBase,
					ParentTagSearchDepthMax));
	}

	// Extracts the key/value pairs from a gameplaytag-keyed map into a sorted array
	template <typename TPayload, int32 ExpectedSourceKeyCountMax = 32>
	TArray<TPair<FGameplayTag, TPayload>> BuildSortedGameplayTagMapPairs(const TMap<FGameplayTag, TPayload>& GameplayTagToPayloadMap)
	{
		FlowArray::TInlineArray<FGameplayTag, ExpectedSourceKeyCountMax> MapKeys;
		GameplayTagToPayloadMap.GenerateKeyArray(MapKeys);

		MapKeys.StableSort([](const FGameplayTag& Tag0, const FGameplayTag& Tag1)
		{
			return Tag0.GetGameplayTagParents().Num() < Tag1.GetGameplayTagParents().Num();
		});

		TArray<TPair<FGameplayTag, TPayload>> Pairs;
		Pairs.Reserve(MapKeys.Num());

		for (const FGameplayTag& KeyTag : MapKeys)
		{
			const TPayload& Payload = GameplayTagToPayloadMap.FindChecked(KeyTag);

			Pairs.Emplace(KeyTag, Payload);
		}

		return Pairs;
	}

}
