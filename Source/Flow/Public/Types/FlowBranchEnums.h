// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Types/FlowEnumUtils.h"

UENUM(BlueprintType)
enum class EFlowPredicateCombinationRule : uint8
{
	AND UMETA(ToolTip = "Passes if ALL child predicates pass"),
	OR UMETA(ToolTip = "Passes if ANY (at least one) child predicates pass"),

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowPredicateCombinationRule);

/* Operator for compare operation. */
UENUM(BlueprintType)
enum class EFlowPredicateCompareOperatorType : uint8
{
	// Supported by all DataPin types (& UBlackboardKeyItem subclasses in the AIFlowGraph plugin)

	Equal			UMETA(DisplayName = "Is Equal To"),
	NotEqual		UMETA(DisplayName = "Is Not Equal To"),

	// Supported by numeric (eg, _Int, _Float and _Enum) subclasses only

	Less			UMETA(DisplayName = "Is Less Than"),
	LessOrEqual		UMETA(DisplayName = "Is Less Than Or Equal To"),
	Greater			UMETA(DisplayName = "Is Greater Than"),
	GreaterOrEqual	UMETA(DisplayName = "Is Greater Than Or Equal To"),

	Max				UMETA(Hidden),
	Invalid			UMETA(Hidden),
	Min = 0			UMETA(Hidden),

	// Subrange for equality operations
	EqualityFirst = Equal UMETA(Hidden),
	EqualityLast = NotEqual UMETA(Hidden),

	// Subrange for Arithmetic-only operations
	ArithmeticFirst = Less UMETA(Hidden),
	ArithmeticLast = GreaterOrEqual UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowPredicateCompareOperatorType)

namespace EFlowPredicateCompareOperatorType_Classifiers
{
	FORCEINLINE bool IsEqualityOperation(EFlowPredicateCompareOperatorType Operation) { return FLOW_IS_ENUM_IN_SUBRANGE(Operation, EFlowPredicateCompareOperatorType::Equality); }
	FORCEINLINE bool IsArithmeticOperation(EFlowPredicateCompareOperatorType Operation) { return FLOW_IS_ENUM_IN_SUBRANGE(Operation, EFlowPredicateCompareOperatorType::Arithmetic); }

	FORCEINLINE_DEBUGGABLE FString GetOperatorSymbolString(const EFlowPredicateCompareOperatorType OperatorType)
	{
		static_assert(static_cast<int32>(EFlowPredicateCompareOperatorType::Max) == 6, TEXT("This should be kept up to date with the enum"));
		switch(OperatorType)
		{
		case EFlowPredicateCompareOperatorType::Equal:
			return TEXT("==");
		case EFlowPredicateCompareOperatorType::NotEqual:
			return TEXT("!=");
		case EFlowPredicateCompareOperatorType::Less:
			return TEXT("<");
		case EFlowPredicateCompareOperatorType::LessOrEqual:
			return TEXT("<=");
		case EFlowPredicateCompareOperatorType::Greater:
			return TEXT(">");
		case EFlowPredicateCompareOperatorType::GreaterOrEqual:
			return TEXT(">=");
		default:
			return TEXT("[Invalid Operator]");
		}
	}
}