// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Types/FlowIdentity.h"

#include "FlowComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowIdentity)

FString FFlowIdentity::ToString(bool bShortNames, bool bIncludeMatchType, bool bIncludeClassFilters, FString Separator) const
{
	if (IdentityTags.IsEmpty())
	{
		return TEXT("None");
	}

	FString Result = FString::JoinBy(IdentityTags, *Separator, [bShortNames](const FGameplayTag& Tag)
	{
		FString TagName = bShortNames ? Tag.GetTagLeafName().ToString() : Tag.ToString();
		return TagName;
	});

	if (bIncludeMatchType)
	{
		FString MatchName = UEnum::GetValueAsString(IdentityMatchType);
		MatchName.Split(TEXT("::"), nullptr, &MatchName);
		Result += Separator + MatchName;
	}

	return Result;
}

bool FFlowIdentity::SerializeFromMismatchedTag(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot)
{
	if (Tag.GetType().IsStruct(FGameplayTagContainer::StaticStruct()->GetFName()))
	{
		IdentityTags.Serialize(Slot);
		return true;
	}
	if (Tag.GetType().IsStruct(FGameplayTag::StaticStruct()->GetFName()))
	{
		FGameplayTag GameplayTag;
		FGameplayTag::StaticStruct()->SerializeItem(Slot, &GameplayTag, nullptr);
		IdentityTags.AddTag(GameplayTag);
		return true;
	}
	return false;
}
