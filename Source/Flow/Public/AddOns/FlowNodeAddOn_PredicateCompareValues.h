// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "AddOns/FlowNodeAddOn.h"
#include "Interfaces/FlowPredicateInterface.h"
#include "Types/FlowBranchEnums.h"
#include "Types/FlowNamedDataPinProperty.h"

#include "FlowNodeAddOn_PredicateCompareValues.generated.h"

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

	/* Compatibility check by standard pin type names. */
	static bool AreComparableStandardPinTypes(const FName& LeftPinTypeName, const FName& RightPinTypeName);

	// Domain classifiers
	static bool IsNumericTypeName(const FName& TypeName);
	static bool IsFloatingPointType(const FName& TypeName);
	static bool IsIntegerType(const FName& TypeName);

	static bool IsTextType(const FName& TypeName);
	static bool IsStringType(const FName& TypeName);
	static bool IsNameLikeType(const FName& TypeName);
	static bool IsEnumTypeName(const FName& TypeName);

	static bool IsAnyStringLikeTypeName(const FName& TypeName);
	static bool IsGameplayTagLikeTypeName(const FName& TypeName);

	// Domain comparisons (these return true if they successfully compared; result is via out param)
	bool TryCheckGameplayTagsEqual(bool& bOutIsEqual) const;
	bool TryCheckTextEqual(bool& bOutIsEqual) const;
	bool TryCheckStringEqual(bool& bOutIsEqual) const;
	bool TryCheckNameEqual(bool& bOutIsEqual) const;

	// Numeric comparisons support full operator set
	bool TryCompareAsDouble() const;
	bool TryCompareAsInt64() const;

	// Comparison helpers
	bool CompareDoubleUsingOperator(double LeftValueAsDouble, double RightValueAsDouble) const;
	bool CompareInt64UsingOperator(int64 LeftValueAsInt64, int64 RightValueAsInt64) const;

	// These are the DataPinNamedProperty property names 
	// (ie, the name of the property itself, eg "LeftValue")
	const FName& GetLeftValuePropertyName() const;
	const FName& GetRightValuePropertyName() const;

	/* This is the value as-authored by the node author in their graph. */
	FORCEINLINE const FName& GetAuthoredValueName(const FFlowNamedDataPinProperty& NamedDataPinProperty) const { return NamedDataPinProperty.Name; }

	/* This is the authored value after being disambiguated (for duplicates).
	 * Example: how it is presented and indexed on the owning Flow Node. */
	FORCEINLINE const FName& GetDisambiguatedValueName(const FFlowNamedDataPinProperty& NamedDataPinProperty) const { return NamedDataPinProperty.DataPinValue.Get().PropertyPinName; }
};
