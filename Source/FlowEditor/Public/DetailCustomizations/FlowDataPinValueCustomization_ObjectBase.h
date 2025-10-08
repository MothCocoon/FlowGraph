// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "DetailCustomizations/FlowDataPinValueCustomization.h"
#include "DetailCustomizations/FlowValueSourcePolicy.h"

/*
* Base for object-like pin value customizations (Object, InstancedObject).
* - Computes & applies source policy (lock only affects ClassFilter)
* - Adds (optionally) a ClassFilter property row
* - Validates object reference values against ClassFilter
* - Offers single / array layout; element value widget supplied by derived class
*/
class FLOWEDITOR_API FFlowDataPinValueCustomization_ObjectBase : public FFlowDataPinValueCustomization
{
	using Super = FFlowDataPinValueCustomization;

public:
	FFlowDataPinValueCustomization_ObjectBase() = default;
	virtual ~FFlowDataPinValueCustomization_ObjectBase() = default;

	// Non-copyable / non-movable
	FFlowDataPinValueCustomization_ObjectBase(const FFlowDataPinValueCustomization_ObjectBase&) = delete;
	FFlowDataPinValueCustomization_ObjectBase& operator=(const FFlowDataPinValueCustomization_ObjectBase&) = delete;
	FFlowDataPinValueCustomization_ObjectBase(FFlowDataPinValueCustomization_ObjectBase&&) = delete;
	FFlowDataPinValueCustomization_ObjectBase& operator=(FFlowDataPinValueCustomization_ObjectBase&&) = delete;

protected:
	virtual void BuildValueRows(TSharedRef<IPropertyHandle> InStructPropertyHandle,
		IDetailChildrenBuilder& StructBuilder,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	virtual void OnSourceLockToggled() override;
	virtual const FFlowValueSourcePolicy* GetSourcePolicy() const override { return &SourcePolicy; }

protected:
	virtual bool SupportsInlineCreation() const { return false; }
	virtual TSharedRef<SWidget> BuildObjectValueWidgetForElement(TSharedPtr<IPropertyHandle> ElementHandle);
	virtual void BuildClassFilterRow(IDetailChildrenBuilder& StructBuilder);

protected:
	// Property handles
	TSharedPtr<IPropertyHandle> ClassFilterHandle;

	// Policy
	FFlowValueSourcePolicy SourcePolicy;

	// Metadata / filter state
	bool bMetaClassForced = false;
	TWeakObjectPtr<UClass> EffectiveFilterClass;

	// Layout helpers
	void BuildSingleBranch(IDetailChildrenBuilder& StructBuilder);
	void BuildArrayBranch(IDetailChildrenBuilder& StructBuilder);
	void GenerateArrayElementRow(TSharedRef<IPropertyHandle> ElementHandle,
		int32 Index,
		IDetailChildrenBuilder& ChildBuilder,
		const TAttribute<EVisibility>& RowVisibility);

	// Policy computation
	void ComputePolicy();

	// Meta-class handling
	void TryApplyMetaClass();
	void ResolveEffectiveFilter();

	// Delegates & validation
	void BindDelegates();
	void OnClassFilterChanged();
	void OnValuesChanged();
	void ValidateAll();
	void ValidateElement(TSharedPtr<IPropertyHandle> ElementHandle, UClass* Filter);

	// Value access
	UObject* GetObjectValue(TSharedPtr<IPropertyHandle> ElementHandle) const;
	void SetObjectValue(TSharedPtr<IPropertyHandle> ElementHandle, UObject* NewObj);
	UClass* GetCurrentFilterClassProperty() const;
};