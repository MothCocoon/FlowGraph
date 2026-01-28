// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "DetailCustomizations/FlowDataPinValueCustomization.h"

class SClassPropertyEntryBox;

/*
* Class value customization:
*  - Conditionally shows ClassFilter (OwnerInterface->ShowFlowDataPinValueClassFilter).
*  - If MetaClass metadata present: show row but disabled.
*  - Enabled state otherwise: OwnerInterface->CanEditFlowDataPinValueClassFilter.
*  - Validates stored FSoftClassPath values against effective filter.
*/
class FLOWEDITOR_API FFlowDataPinValueCustomization_Class : public FFlowDataPinValueCustomization
{
	using Super = FFlowDataPinValueCustomization;

public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FFlowDataPinValueCustomization_Class());
	}

protected:
	virtual void BuildValueRows(TSharedRef<IPropertyHandle> InStructPropertyHandle,
		IDetailChildrenBuilder& StructBuilder,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	// Property handles
	TSharedPtr<IPropertyHandle> ClassFilterHandle;

	// Metadata-derived flags
	const UClass* RequiredInterface = nullptr;
	bool bAllowAbstract = true;
	bool bIsBlueprintBaseOnly = false;
	bool bAllowNone = true;
	bool bShowTreeView = false;
	bool bHideViewOptions = false;
	bool bShowDisplayNames = false;
	bool bHasMetaClass = false;

	// Effective filter
	TWeakObjectPtr<UClass> CachedEffectiveFilter;

	// Helpers
	void ExtractMetadata();
	void TrySetClassFilterFromMetaData() const;
	UClass* DeriveBestClassFilter() const;
	void RefreshEffectiveFilter();

	// UI
	void BuildClassFilterRow(IDetailChildrenBuilder& StructBuilder, bool bSourceEditable) const;
	void BuildSingleBranch(IDetailChildrenBuilder& StructBuilder);
	void BuildArrayBranch(IDetailChildrenBuilder& StructBuilder);

	// Delegates / validation
	void BindDelegates();
	void OnClassFilterChanged();
	void OnValuesChanged();

	void ValidateAllElements();
	bool IsElementValid(TSharedPtr<IPropertyHandle> ElementHandle) const;

	// Access / modification
	static const UClass* GetSelectedClassForHandle(TSharedPtr<IPropertyHandle> ElementHandle);
	void OnSetClassForHandle(const UClass* NewClass, TSharedPtr<IPropertyHandle> ElementHandle) const;

	static bool GetElementPathString(const TSharedPtr<IPropertyHandle>& ElementHandle, FString& OutPath);
	static bool IsNoneString(const FString& Str);

	// Permissions (inline owner queries)
	bool ShouldShowSourceRow() const;
	bool IsSourceEditable() const;
	bool AreValuesEditable() const { return true; }

	// Value struct access
	struct FFlowDataPinValue_Class* GetValueStruct() const;
};