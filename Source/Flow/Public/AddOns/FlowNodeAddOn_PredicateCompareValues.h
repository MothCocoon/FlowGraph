// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include <functional>

#include "AddOns/FlowNodeAddOn.h"
#include "Interfaces/FlowPredicateInterface.h"
#include "Types/FlowBranchEnums.h"
#include "Types/FlowNamedDataPinProperty.h"

#include "FlowNodeAddOn_PredicateCompareValues.generated.h"

struct FFlowPinConnectionPolicy;

UCLASS(MinimalApi, NotBlueprintable, meta = (DisplayName = "Compare Values"))
class UFlowNodeAddOn_PredicateCompareValues
	: public UFlowNodeAddOn
	, public IFlowPredicateInterface
{
	GENERATED_BODY()

public:
	UFlowNodeAddOn_PredicateCompareValues();

	// IFlowPredicateInterface
	virtual bool EvaluatePredicate_Implementation() const override;
	// --

#if WITH_EDITOR
	// UObject
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	// --

	// UFlowNodeBase
	virtual EDataValidationResult ValidateNode() override;
	virtual FText K2_GetNodeTitle_Implementation() const override;
	// --

	/* Utility function for subclasses, if they want to force a named property to be Input or Output.
	* Unused in this class. */
	void OnPostEditEnsureAllNamedPropertiesPinDirection(const FProperty& Property, bool bIsInput);
#endif

protected:
	UPROPERTY(EditAnywhere, Category = Configuration, DisplayName = "Operator")
	EFlowPredicateCompareOperatorType OperatorType = EFlowPredicateCompareOperatorType::Equal;

	UPROPERTY(EditAnywhere, Category = Configuration)
	FFlowNamedDataPinProperty LeftValue;

	UPROPERTY(EditAnywhere, Category = Configuration)
	FFlowNamedDataPinProperty RightValue;

	UPROPERTY(EditAnywhere, Category = Configuration)
	EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue;

protected:
	// IFlowDataPinValueOwnerInterface
	virtual bool TryFindPropertyByPinName(
		const FName& PinName,
		const FProperty*& OutFoundProperty,
		TInstancedStruct<FFlowDataPinValue>& OutFoundInstancedStruct) const override;
	// --

	// Operator classifiers
	bool IsEqualityOp() const;
	bool IsArithmeticOp() const;

	/* Compatibility check by pin type names. */
	static bool AreComparablePinTypes(
		const FFlowPinConnectionPolicy& PinConnectionPolicy,
		const FName& LeftPinTypeName,
		const FName& RightPinTypeName);

	// Domain classifiers
	static bool IsNumericTypeName(const FFlowPinConnectionPolicy& PinConnectionPolicy, const FName& TypeName);
	static bool IsFloatingPointType(const FFlowPinConnectionPolicy& PinConnectionPolicy, const FName& TypeName);
	static bool IsIntegerType(const FFlowPinConnectionPolicy& PinConnectionPolicy, const FName& TypeName);
	static bool IsAnyStringLikeTypeName(const FFlowPinConnectionPolicy& PinConnectionPolicy, const FName& TypeName);
	static bool IsGameplayTagLikeTypeName(const FFlowPinConnectionPolicy& PinConnectionPolicy, const FName& TypeName);

	static bool IsTextType(const FName& TypeName);
	static bool IsStringType(const FName& TypeName);
	static bool IsNameLikeType(const FName& TypeName);
	static bool IsBoolTypeName(const FName& TypeName);
	static bool IsVectorTypeName(const FName& TypeName);
	static bool IsRotatorTypeName(const FName& TypeName);
	static bool IsTransformTypeName(const FName& TypeName);
	static bool IsObjectTypeName(const FName& TypeName);
	static bool IsClassTypeName(const FName& TypeName);
	static bool IsInstancedStructTypeName(const FName& TypeName);

	// -----------------------------------------------------------------------
	// Domain equality comparisons
	// (these return true if they successfully compared; equality result is via out param)
	// -----------------------------------------------------------------------

	/* Generic equality check: resolve both sides as TFlowPinType, compare with Comparator.
	 * Works for any pin type whose ValueType is supported by the comparator.
	 * ErrorLabel is used in LogError messages (e.g. "Bool", "Vector", "Object").
	 * ComparatorFn defaults to std::equal_to<> (transparent), which uses operator==. */
	template <typename TFlowPinType, typename ComparatorFn = std::equal_to<>>
	bool TryCheckResolvedValuesEqual(bool& bOutIsEqual, const TCHAR* ErrorLabel, ComparatorFn Comparator = {}) const;

	// Domain comparisons that need special handling beyond simple resolve-and-compare
	bool TryCheckGameplayTagsEqual(bool& bOutIsEqual) const;

	/* Fallback: both sides convert to string via TryConvertValuesToString.
	 * This supports user-added pin types from other plugins, so long as they implement TryConvertValuesToString. */
	bool TryCheckFallbackStringEqual(bool& bOutIsEqual) const;

	// Numeric comparisons support full operator set
	bool TryCompareAsDouble() const;
	bool TryCompareAsInt64() const;

	// Comparison helpers
	bool CompareDoubleUsingOperator(double LeftValueAsDouble, double RightValueAsDouble) const;
	bool CompareInt64UsingOperator(int64 LeftValueAsInt64, int64 RightValueAsInt64) const;

	/* Helper for equality-only type blocks in EvaluatePredicate.
	* Guards against arithmetic operators, calls CompareFunc, applies Equal/NotEqual flip.
	* Returns the final predicate result, or false on error. */
	bool EvaluateEqualityBlock(const TCHAR* TypeLabel, const TFunctionRef<bool(bool& /*bOutIsEqual*/)> CompareFunc) const;

	// These are the DataPinNamedProperty property names 
	// (ie, the name of the property itself, eg "LeftValue")
	const FName& GetLeftValuePropertyName() const;
	const FName& GetRightValuePropertyName() const;

	/* This is the value as-authored by the node author in their graph. */
	FORCEINLINE const FName& GetAuthoredValueName(const FFlowNamedDataPinProperty& NamedDataPinProperty) const { return NamedDataPinProperty.Name; }

	/* This is the authored value after being disambiguated (for duplicates).
	* Example: how it is presented and indexed on the owning Flow Node. */
	FORCEINLINE const FName& GetDisambiguatedValueName(const FFlowNamedDataPinProperty& NamedDataPinProperty) const { return NamedDataPinProperty.DataPinValue.Get().PropertyPinName; }

private:

	/* Cached type names for the current evaluation, to avoid repeated TInstancedStruct::Get() calls.
	* Only valid during a single call to EvaluatePredicate_Implementation or ValidateNode. */
	struct FCachedTypeNames
	{
		FName LeftTypeName;
		FName RightTypeName;
		bool bIsValid = false;

		void Reset() { bIsValid = false; }
	};

	/* Populate cached type names from the current LeftValue/RightValue.
	* Returns false (and logs error) if either value is not configured. */
	bool CacheTypeNames(FCachedTypeNames& OutCache) const;
};

// -----------------------------------------------------------------------
// Template implementations
// -----------------------------------------------------------------------

template <typename TFlowPinType, typename ComparatorFn>
bool UFlowNodeAddOn_PredicateCompareValues::TryCheckResolvedValuesEqual(bool& bOutIsEqual, const TCHAR* ErrorLabel, ComparatorFn Comparator) const
{
	typename TFlowPinType::ValueType LeftResolved{};
	{
		const EFlowDataPinResolveResult ResolveResult =
			TryResolveDataPinValue<TFlowPinType>(GetDisambiguatedValueName(LeftValue), LeftResolved, SingleFromArray);

		if (!FlowPinType::IsSuccess(ResolveResult))
		{
			LogError(FString::Printf(TEXT("Failed to resolve LeftValue as %s."), ErrorLabel));
			return false;
		}
	}

	typename TFlowPinType::ValueType RightResolved{};
	{
		const EFlowDataPinResolveResult ResolveResult =
			TryResolveDataPinValue<TFlowPinType>(GetDisambiguatedValueName(RightValue), RightResolved, SingleFromArray);

		if (!FlowPinType::IsSuccess(ResolveResult))
		{
			LogError(FString::Printf(TEXT("Failed to resolve RightValue as %s."), ErrorLabel));
			return false;
		}
	}

	bOutIsEqual = Comparator(LeftResolved, RightResolved);
	return true;
}
