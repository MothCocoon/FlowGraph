// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "DetailCustomizations/FlowDataPinValueCustomization.h"

/*
* Object value customization:
*  - Conditionally shows ClassFilter (OwnerInterface->ShowFlowDataPinValueClassFilter).
*  - MetaClass metadata forces filter (row shown but disabled).
*  - Validates object references against effective filter.
*/
class FLOWEDITOR_API FFlowDataPinValueCustomization_Object : public FFlowDataPinValueCustomization
{
	using Super = FFlowDataPinValueCustomization;

public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FFlowDataPinValueCustomization_Object());
	}

protected:
	virtual void BuildValueRows(TSharedRef<IPropertyHandle> InStructPropertyHandle,
		IDetailChildrenBuilder& StructBuilder,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	// Property handles
	TSharedPtr<IPropertyHandle> ClassFilterHandle;

	// MetaClass state
	bool bMetaClassForced = false;
	TWeakObjectPtr<UClass> EffectiveFilterClass;

	// UI building
	void BuildClassFilterRow(IDetailChildrenBuilder& StructBuilder, bool bSourceEditable) const;
	virtual void BuildSingleBranch(IDetailChildrenBuilder& StructBuilder) override;
	virtual void BuildArrayBranch(IDetailChildrenBuilder& StructBuilder) override;

	// Metadata / filter
	void TryApplyMetaClass();
	void ResolveEffectiveFilter();

	// Delegates & validation
	void BindDelegates();
	void OnClassFilterChanged();
	void OnValuesChanged();
	void ValidateAll();
	bool IsElementValid(TSharedPtr<IPropertyHandle> ElementHandle) const;
	static void InvalidateElement(TSharedPtr<IPropertyHandle> ElementHandle);

	// Value access
	static UObject* GetObjectValue(TSharedPtr<IPropertyHandle> ElementHandle);
	static void SetObjectValue(TSharedPtr<IPropertyHandle> ElementHandle, UObject* NewObj);

	// Permissions (inline owner queries)
	bool ShouldShowSourceRow() const;
	bool IsSourceEditable() const;
	static bool AreValuesEditable() { return true; }

	// Value struct accessor
	struct FFlowDataPinValue_Object* GetValueStruct() const;

	// Widget
	TSharedRef<SWidget> BuildObjectValueWidgetForElement(TSharedPtr<IPropertyHandle> ElementHandle) const;
};