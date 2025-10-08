// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "DetailCustomizations/FlowDataPinValueCustomization.h"
#include "DetailCustomizations/FlowValueSourcePolicy.h"

class SClassPropertyEntryBox;

/*
* Class value customization using FFlowValueSourcePolicy:
*  - Lock hides/disables only ClassFilter (source) — NOT the class value pickers.
*  - Class value rows remain editable (subject to base owner policy & metadata), even when filter locked.
*/
class FLOWEDITOR_API FFlowDataPinValueCustomization_Class : public FFlowDataPinValueCustomization
{
	using Super = FFlowDataPinValueCustomization;

public:
	FFlowDataPinValueCustomization_Class() = default;

	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FFlowDataPinValueCustomization_Class());
	}

	// Non-copyable / non-movable
	FFlowDataPinValueCustomization_Class(const FFlowDataPinValueCustomization_Class&) = delete;
	FFlowDataPinValueCustomization_Class& operator=(const FFlowDataPinValueCustomization_Class&) = delete;
	FFlowDataPinValueCustomization_Class(FFlowDataPinValueCustomization_Class&&) = delete;
	FFlowDataPinValueCustomization_Class& operator=(FFlowDataPinValueCustomization_Class&&) = delete;

protected:
	virtual void BuildValueRows(TSharedRef<IPropertyHandle> InStructPropertyHandle,
		IDetailChildrenBuilder& StructBuilder,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	virtual void OnSourceLockToggled() override;
	virtual const FFlowValueSourcePolicy* GetSourcePolicy() const override { return &SourcePolicy; }

private:
	// Property handles
	TSharedPtr<IPropertyHandle> ClassFilterHandle;

	// Policy
	FFlowValueSourcePolicy SourcePolicy;

	// Metadata-derived flags
	const UClass* RequiredInterface = nullptr;
	bool bAllowAbstract = true;
	bool bIsBlueprintBaseOnly = false;
	bool bAllowNone = true;
	bool bShowTreeView = false;
	bool bHideViewOptions = false;
	bool bShowDisplayNames = false;
	bool bMetaClassForced = false;

	// Cached effective filter
	TWeakObjectPtr<UClass> CachedEffectiveFilter;

	// Helpers
	void ExtractMetadata();
	void ComputePolicy();
	void BuildClassFilterRow(IDetailChildrenBuilder& StructBuilder);
	void BuildSingleBranch(IDetailChildrenBuilder& StructBuilder);
	void BuildArrayBranch(IDetailChildrenBuilder& StructBuilder);
	void GenerateArrayElementRow(TSharedRef<IPropertyHandle> ElementHandle, int32 Index,
		IDetailChildrenBuilder& ChildBuilder, const TAttribute<EVisibility>& RowVisibility);

	void BindValidationDelegates();
	void OnClassFilterChanged();
	void OnValuesChanged();

	void TrySetClassFilterFromMetaData();
	UClass* DeriveBestClassFilter() const;
	void RefreshEffectiveFilter();

	void ValidateAllElements();
	void ValidateElement(const TSharedPtr<IPropertyHandle>& ElementHandle, UClass* FilterClass);

	const UClass* GetSelectedClassForHandle(TSharedPtr<IPropertyHandle> ElementHandle) const;
	void OnSetClassForHandle(const UClass* NewClass, TSharedPtr<IPropertyHandle> ElementHandle);

	bool GetElementPathString(const TSharedPtr<IPropertyHandle>& ElementHandle, FString& OutPath) const;
	bool IsNoneString(const FString& Str) const;
};