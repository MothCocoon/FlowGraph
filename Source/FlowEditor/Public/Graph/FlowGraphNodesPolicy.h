// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Types/FlowEnumUtils.h"

#include "FlowGraphNodesPolicy.generated.h"

class UFlowNodeBase;

UENUM()
enum class EFlowGraphPolicyResult : int8
{
	// Forbidden by the policy unless a more specific rule applies
	TentativeForbidden,

	// Allowed by the policy unless a more specific rule applies
	TentativeAllowed,

	// Strictly forbidden by the policy
	Forbidden,

	// Strictly allowed by the policy
	Allowed,

	Max UMETA(Hidden),
	Invalid = -1 UMETA(Hidden),
	Min = 0 UMETA(Hidden),

	// Subrange for strict results
	StrictFirst = Forbidden UMETA(Hidden),
	StrictLast = Allowed UMETA(Hidden),

	// Subrange for tentative results
	TentativeFirst = TentativeForbidden UMETA(Hidden),
	TentativeLast = TentativeAllowed UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowGraphPolicyResult);

namespace EFlowGraphPolicyResult_Classifiers
{
	FORCEINLINE bool IsStrictPolicyResult(const EFlowGraphPolicyResult Result) { return FLOW_IS_ENUM_IN_SUBRANGE(Result, EFlowGraphPolicyResult::Strict); }
	FORCEINLINE bool IsTentativePolicyResult(const EFlowGraphPolicyResult Result) { return FLOW_IS_ENUM_IN_SUBRANGE(Result, EFlowGraphPolicyResult::Tentative); }
	FORCEINLINE bool IsAnyAllowedPolicyResult(const EFlowGraphPolicyResult Result) { return Result == EFlowGraphPolicyResult::Allowed || Result == EFlowGraphPolicyResult::TentativeAllowed; }
	FORCEINLINE bool IsAnyForbiddenPolicyResult(const EFlowGraphPolicyResult Result) { return Result == EFlowGraphPolicyResult::Forbidden || Result == EFlowGraphPolicyResult::TentativeForbidden; }

	FORCEINLINE EFlowGraphPolicyResult MergePolicyResult(const EFlowGraphPolicyResult Result0, const EFlowGraphPolicyResult Result1)
	{ 
		checkf(!(IsStrictPolicyResult(Result0) && IsStrictPolicyResult(Result1)), TEXT("Should not be deciding between two strict results"));

		// Numerically prefer: Allowed or Forbidden > TentativeAllowed > TentativeForbidden > Invalid
		return static_cast<EFlowGraphPolicyResult>(FMath::Max(FlowEnum::ToInt(Result0), FlowEnum::ToInt(Result1)));
	}
}

USTRUCT()
struct FFlowGraphNodesPolicy
{
	GENERATED_BODY();

public:
#if WITH_EDITORONLY_DATA
	UPROPERTY(Config, EditAnywhere, Category = "Nodes")
	TArray<FString> AllowedCategories;

	UPROPERTY(Config, EditAnywhere, Category = "Nodes")
	TArray<FString> DisallowedCategories;
#endif

#if WITH_EDITOR
public:
	EFlowGraphPolicyResult IsNodeAllowedByPolicy(const UFlowNodeBase* FlowNodeBase) const;

protected:
	static bool IsAnySubcategory(const FString& CheckCategory, const TArray<FString>& Categories);
#endif
};

