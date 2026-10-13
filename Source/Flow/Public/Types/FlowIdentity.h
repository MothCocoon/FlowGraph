// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "FlowTypes.h"
#include "FlowIdentity.generated.h"

USTRUCT(BlueprintType)
struct FLOW_API FFlowIdentity
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FGameplayTagContainer IdentityTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	EFlowTagContainerMatchType IdentityMatchType;

	/* Restricts matching to Flow Components of this class.
	 * Class is never loaded by the filter itself. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Identity")
	TSoftClassPtr<class UFlowComponent> ComponentFilter;

	/* Restricts matching to actors of this class.
	 * Class is never loaded by the filter itself. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Identity")
	TSoftClassPtr<class AActor> ActorFilter;

	FFlowIdentity()
		: IdentityMatchType(EFlowTagContainerMatchType::HasAnyExact)
	{
	}

	bool IsValid() const;
	bool IsExactMatch() const;
	EGameplayContainerMatchType GetContainerMatchType() const;

	bool Matches(const FGameplayTagContainer& Tags) const;
	bool Matches(const UFlowComponent* Component) const;
	bool Matches(const AActor* Actor) const;

	bool MatchesFilters(const UFlowComponent* Component) const;

	FString ToString(const bool bShortNames = false, const bool bIncludeMatchType = false, const bool bIncludeClassFilters = false, const FString& Separator = TEXT("\n")) const;
	bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot);
};

template <>
struct TStructOpsTypeTraits<FFlowIdentity> : public TStructOpsTypeTraitsBase2<FFlowIdentity>
{
	enum
	{
		WithStructuredSerializeFromMismatchedTag = true,
	};
};
