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

	FFlowIdentity()
		: IdentityMatchType(EFlowTagContainerMatchType::HasAnyExact)
	{
	}

	FString ToString(bool bShortNames = false, bool bIncludeMatchType = false, bool bIncludeClassFilters = false, FString Separator = TEXT("\n")) const;
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
