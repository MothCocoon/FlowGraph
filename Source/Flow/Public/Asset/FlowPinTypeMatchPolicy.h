// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowPinTypeMatchPolicy.generated.h"

UENUM(meta = (BitFlags))
enum class EFlowPinTypeMatchRules : uint32
{
	None = 0,

	RequirePinCategoryMatch					= 1 << 0,
	RequirePinCategoryMemberReferenceMatch	= 1 << 1,
	RequireContainerTypeMatch				= 1 << 2,
	RequirePinSubCategoryObjectMatch		= 1 << 3,
	AllowSubCategoryObjectSubclasses		= 1 << 4,
	AllowSubCategoryObjectSameLayout		= 1 << 5,
	SameLayoutMustMatchPropertyNames		= 1 << 6,

	// Masks for convenience
	StandardPinTypeMatchRulesMask = 
		RequirePinCategoryMatch |
		RequirePinCategoryMemberReferenceMatch |
		AllowSubCategoryObjectSubclasses |
		AllowSubCategoryObjectSameLayout UMETA(Hidden),

	SubCategoryObjectPinTypeMatchRulesMask =
		StandardPinTypeMatchRulesMask |
		RequirePinSubCategoryObjectMatch UMETA(Hidden),
};

USTRUCT()
struct FFlowPinTypeMatchPolicy
{
	GENERATED_BODY()

	UPROPERTY()
	EFlowPinTypeMatchRules PinTypeMatchRules = EFlowPinTypeMatchRules::StandardPinTypeMatchRulesMask;

	// Pin categories to allow beyond an exact match
	UPROPERTY()
	TSet<FName> PinCategories;
};
