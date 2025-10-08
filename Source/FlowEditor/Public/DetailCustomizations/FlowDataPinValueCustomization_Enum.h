// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "DetailCustomizations/FlowDataPinValueCustomization.h"
#include "DetailCustomizations/FlowValueSourcePolicy.h"
#include "UnrealExtensions/VisibilityArrayBuilder.h"

class UEnum;
struct FFlowDataPinValue_Enum;

/*
* Enum customization:
*  - Lock affects only EnumClass / EnumName (source).
*  - Enumerator value selection remains editable (independent of lock).
*  - Uses policy field bFinalEditableSource for source rows.
*/
class FLOWEDITOR_API FFlowDataPinValueCustomization_Enum : public FFlowDataPinValueCustomization
{
	using Super = FFlowDataPinValueCustomization;

public:
	FFlowDataPinValueCustomization_Enum() = default;

	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FFlowDataPinValueCustomization_Enum());
	}

	// Non-copyable / non-movable
	FFlowDataPinValueCustomization_Enum(const FFlowDataPinValueCustomization_Enum&) = delete;
	FFlowDataPinValueCustomization_Enum& operator=(const FFlowDataPinValueCustomization_Enum&) = delete;
	FFlowDataPinValueCustomization_Enum(FFlowDataPinValueCustomization_Enum&&) = delete;
	FFlowDataPinValueCustomization_Enum& operator=(FFlowDataPinValueCustomization_Enum&&) = delete;

protected:
	virtual void BuildValueRows(TSharedRef<IPropertyHandle> InStructPropertyHandle,
		IDetailChildrenBuilder& StructBuilder,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	virtual void OnSourceLockToggled() override;
	virtual const FFlowValueSourcePolicy* GetSourcePolicy() const override { return &SourcePolicy; }

private:
	// Source handles
	TSharedPtr<IPropertyHandle> EnumClassHandle;
	TSharedPtr<IPropertyHandle> EnumNameHandle;

	// Policy
	FFlowValueSourcePolicy SourcePolicy;

	// Lock / value state
	TSharedPtr<IPropertyHandle> LockEnumHandle;
	TArray<TSharedPtr<FName>> EnumeratorOptions;
	bool bEnumResolved = false;
	bool bMultiTypeDelegateBound = false;

	// Builders
	void BuildSingle(IDetailChildrenBuilder& StructBuilder);
	void BuildArray(IDetailChildrenBuilder& StructBuilder);

	// Visibility helpers
	EVisibility GetSingleVisibility() const;
	EVisibility GetArrayVisibility() const;

	// Enum resolution
	void CacheEnumHandles(const TSharedRef<IPropertyHandle>& StructHandle);
	void OnEnumSourceChanged();
	void RebuildEnumData();
	UEnum* ResolveEnum() const;
	void CollectEnumerators(UEnum& EnumObj);

	// Validation
	void ValidateStoredValues();
	bool IsValueValid(const FName& Candidate) const;
	TSharedPtr<FName> FindEnumeratorMatch(const FName& Current) const;

	// Multi-type reaction
	void OnMultiTypeChanged();

	// Array element generation
	void GenerateArrayElementVisible(TSharedRef<IPropertyHandle> ElementHandle,
		int32 Index,
		IDetailChildrenBuilder& ChildBuilder,
		const TAttribute<EVisibility>& RowVisibility);

	// Widgets
	TSharedRef<SWidget> GenerateEnumeratorWidget(TSharedPtr<FName> Item) const;
	FText GetEnumeratorDisplayText(const FName& Value) const;
	FText GetEnumSourceTooltip() const;

	// Selection handlers
	void OnSingleValueChanged(TSharedPtr<FName> NewSelection,
		ESelectInfo::Type SelectInfo,
		TSharedPtr<IPropertyHandle> ElementHandle);
	void OnArrayElementChanged(TSharedPtr<FName> NewSelection,
		ESelectInfo::Type SelectInfo,
		TSharedPtr<IPropertyHandle> ElementHandle);

	// Policy compute
	void ComputePolicy();

	// Convenience
	FFlowDataPinValue_Enum* GetEnumValueStruct() const;
	bool HasEnumeratorOptions() const { return bEnumResolved && EnumeratorOptions.Num() > 0; }
	bool IsValueEditingEnabled() const { return HasEnumeratorOptions(); }
};