// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "GameplayTagContainer.h"

#include "FlowGameplayTagUtils.generated.h"

/** Encapsulate require and ignore tags
 * Adapted from FGameplayTagRequirements, but without the GameplayAbilities module dependency */
USTRUCT(BlueprintType)
struct FFlowGameplayTagRequirements
{
	GENERATED_BODY()

	/** All of these tags must be present */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayTags, meta = (DisplayName = "Must Have Tags"))
	FGameplayTagContainer RequireTags;

	/** None of these tags may be present */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayTags, meta = (DisplayName = "Must Not Have Tags"))
	FGameplayTagContainer IgnoreTags;

	/** Build up a more complex query that can't be expressed with RequireTags/IgnoreTags alone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayTags, meta = (DisplayName = "Query Must Match"))
	FGameplayTagQuery TagQuery;

	/** True if all required tags and no ignore tags found */
	FLOW_API bool RequirementsMet(const FGameplayTagContainer& Container) const;

	/** True if neither RequireTags or IgnoreTags has any tags */
	FLOW_API bool IsEmpty() const;

	/** Return debug string */
	FLOW_API FString ToString() const;

	FLOW_API bool operator==(const FFlowGameplayTagRequirements& Other) const;
	FLOW_API bool operator!=(const FFlowGameplayTagRequirements& Other) const;

	/** Converts the RequireTags and IgnoreTags fields into an equivalent FGameplayTagQuery */
	[[nodiscard]] FLOW_API FGameplayTagQuery ConvertTagFieldsToTagQuery() const;
};
