// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "DetailCustomizations/FlowDataPinValueCustomization.h"

class UEnum;

/*
* Enum customization:
*  - Conditionally shows EnumClass / EnumName (OwnerInterface->ShowFlowDataPinValueClassFilter).
*  - Enabled if OwnerInterface->CanEditFlowDataPinValueClassFilter (MetaClass concept not applied here).
*  - Enumerator selection via combo boxes (single / array).
*  - Validates stored names.
*  - Uses base single/array visibility helpers.
*/
class FLOWEDITOR_API FFlowDataPinValueCustomization_Enum : public FFlowDataPinValueCustomization
{
	using Super = FFlowDataPinValueCustomization;

public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FFlowDataPinValueCustomization_Enum());
	}

protected:
	virtual void BuildValueRows(TSharedRef<IPropertyHandle> InStructPropertyHandle,
	                            IDetailChildrenBuilder& StructBuilder,
	                            IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	// Source handles
	TSharedPtr<IPropertyHandle> EnumClassHandle;
	TSharedPtr<IPropertyHandle> EnumNameHandle;

	// Enumerator state
	TArray<TSharedPtr<FName>> EnumeratorOptions;
	bool bEnumResolved = false;
	bool bMultiTypeDelegateBound = false;

	// Build helpers
	void BuildSingle(IDetailChildrenBuilder& StructBuilder);
	void BuildArray(IDetailChildrenBuilder& StructBuilder);

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

	// Widgets
	TSharedRef<SWidget> GenerateEnumeratorWidget(TSharedPtr<FName> Item) const;
	static FText GetEnumeratorDisplayText(const FName& Value);
	FText GetEnumSourceTooltip() const;

	// Selection handlers
	static void OnSingleValueChanged(TSharedPtr<FName> NewSelection, ESelectInfo::Type SelectInfo, TSharedPtr<IPropertyHandle> ElementHandle);
	static void OnArrayElementChanged(TSharedPtr<FName> NewSelection, ESelectInfo::Type SelectInfo, TSharedPtr<IPropertyHandle> ElementHandle);

	// Convenience
	struct FFlowDataPinValue_Enum* GetEnumValueStruct() const;
	bool HasEnumeratorOptions() const { return bEnumResolved && EnumeratorOptions.Num() > 0; }
	bool IsValueEditingEnabled() const { return HasEnumeratorOptions(); }

	// Permissions (inline owner queries)
	bool ShouldShowSourceRow() const;
	bool IsSourceEditable() const;
	bool AreValuesEditable() const { return true; }
};
