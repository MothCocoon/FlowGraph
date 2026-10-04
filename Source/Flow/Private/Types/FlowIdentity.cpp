// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Types/FlowIdentity.h"

#include "FlowComponent.h"

#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowIdentity)

bool FFlowIdentity::IsValid() const
{
	return !IdentityTags.IsEmpty();
}

bool FFlowIdentity::IsExactMatch() const
{
	return IdentityMatchType == EFlowTagContainerMatchType::HasAnyExact || IdentityMatchType == EFlowTagContainerMatchType::HasAllExact;
}

EGameplayContainerMatchType FFlowIdentity::GetContainerMatchType() const
{
	if (IdentityMatchType == EFlowTagContainerMatchType::HasAny || IdentityMatchType == EFlowTagContainerMatchType::HasAnyExact)
	{
		return EGameplayContainerMatchType::Any;
	}
	else
	{
		return EGameplayContainerMatchType::All;
	}
}

bool FFlowIdentity::Matches(const FGameplayTagContainer& Tags) const
{
	return IsValid() && FlowTypes::HasMatchingTags(Tags, IdentityTags, IdentityMatchType);
}

bool FFlowIdentity::Matches(const UFlowComponent* Component) const
{
	if (IsValid() && Component)
	{
		return MatchesFilters(Component) && FlowTypes::HasMatchingTags(Component->IdentityTags, IdentityTags, IdentityMatchType);
	}

	return false;
}

bool FFlowIdentity::Matches(const AActor* Actor) const
{
	if (IsValid() && Actor)
	{
		TInlineComponentArray<UFlowComponent*> Components(Actor);
		for (const UFlowComponent* Component : Components)
		{
			if (MatchesFilters(Component) && FlowTypes::HasMatchingTags(Component->IdentityTags, IdentityTags, IdentityMatchType))
			{
				return true;
			}
		}
	}
	
	return false;
}

bool FFlowIdentity::MatchesFilters(const UFlowComponent* Component) const
{
	const AActor* Actor = Component ? Component->GetOwner() : nullptr;
	if (Actor)
	{
		// Filter class not loaded means no instance of it exists, so it can't match
		const UClass* ActorClass = ActorFilter.Get();
		const UClass* ComponentClass = ComponentFilter.Get();

		return (ActorFilter.IsNull() || (ActorClass && Actor->IsA(ActorClass)))
			&& (ComponentFilter.IsNull() || (ComponentClass && Component->IsA(ComponentClass)));
	}

	return false;
}

FString FFlowIdentity::ToString(const bool bShortNames, const bool bIncludeMatchType, const bool bIncludeClassFilters, const FString& Separator) const
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

	if (bIncludeClassFilters)
	{
		if (!ComponentFilter.IsNull())
		{
			Result += Separator + ComponentFilter.GetAssetName();
		}

		if (!ActorFilter.IsNull())
		{
			Result += Separator + ActorFilter.GetAssetName();
		}
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
