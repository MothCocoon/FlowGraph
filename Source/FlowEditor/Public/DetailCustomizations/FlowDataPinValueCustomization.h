// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Types/FlowDataPinType.h"
#include "Types/FlowDataPinValue.h"
#include "UnrealExtensions/IFlowExtendedPropertyTypeCustomization.h"
#include "IPropertyTypeCustomization.h"
#include "Templates/UnrealTypeTraits.h"
#include "Widgets/Input/SComboBox.h"

struct FFlowValueSourcePolicy;
class IFlowDataPinValueOwnerInterface;

/*
* Flow Data Pin Value Customization
*
* Responsibilities:
*  - Provides shared header (MultiType selector + Input Pin checkbox + optional lock UI).
*  - Builds child rows for single value vs array modes (with auto element creation).
*  - Integrates optional source policy (Class / Enum / Object-like specializations derive and supply it).
*  - Supplies extensibility points (AppendHeaderExtensions, OnSourceLockToggled, GetSourcePolicy).
*/
class FLOWEDITOR_API FFlowDataPinValueCustomization : public IFlowExtendedPropertyTypeCustomization
{
	using Super = IFlowExtendedPropertyTypeCustomization;

protected:
	// Property handles
	TSharedPtr<IPropertyHandle> MultiTypeHandle;
	TSharedPtr<IPropertyHandle> ValuesHandle;
	TSharedPtr<IPropertyHandle> IsInputPinHandle;

	// Cached context
	const FFlowDataPinType*              PinType = nullptr;
	IPropertyTypeCustomizationUtils*     CustomizationUtils = nullptr;
	IFlowDataPinValueOwnerInterface*     OwnerInterface = nullptr;
	// -- 

	// MultiType UI state
	TArray<TSharedPtr<FString>>                  MultiTypeOptions;
	TSharedPtr<FString>                          SelectedMultiType;
	TSharedPtr<SComboBox<TSharedPtr<FString>>>   MultiTypeComboBox;

public:
	// Construction / Lifetime
	FFlowDataPinValueCustomization() = default;
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
	// -- 

	// Non-copyable / non-movable
	FFlowDataPinValueCustomization(const FFlowDataPinValueCustomization&) = delete;
	FFlowDataPinValueCustomization& operator=(const FFlowDataPinValueCustomization&) = delete;
	FFlowDataPinValueCustomization(FFlowDataPinValueCustomization&&) = delete;
	FFlowDataPinValueCustomization& operator=(FFlowDataPinValueCustomization&&) = delete;
	// -- 

	// IPropertyTypeCustomization Interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle,
		FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle,
		IDetailChildrenBuilder& StructBuilder,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	// -- 

protected:
	// High-Level Build Flow
	virtual void BuildValueRows(TSharedRef<IPropertyHandle> InStructPropertyHandle,
		IDetailChildrenBuilder& StructBuilder,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils);

	virtual void BuildSingleBranch(IDetailChildrenBuilder& StructBuilder);
	virtual void BuildArrayBranch(IDetailChildrenBuilder& StructBuilder);

	void EnsureSingleElementExists();
	// -- 

	// Mode / State Queries
	EFlowDataMultiType GetCurrentMultiType() const;
	EVisibility GetSingleModeVisibility() const;
	EVisibility GetArrayModeVisibility() const;
	void TrimArrayToSingle();
	// -- 

	// Visual / Appearance
	FLinearColor GetRowTint() const;
	FFlowDataPinValue* GetFlowDataPinValueBeingEdited() const
	{
		return IFlowExtendedPropertyTypeCustomization::TryGetTypedStructValue<FFlowDataPinValue>(StructPropertyHandle);
	}
	// -- 

	// Header Extensions / Source Lock
	virtual void AppendHeaderExtensions(TSharedRef<SHorizontalBox> HeaderBox);
	virtual void OnSourceLockToggled();
	// -- 

	// Optional Source Policy (Overridden in specialized subclasses)
	virtual const FFlowValueSourcePolicy* GetSourcePolicy() const { return nullptr; }
	// -- 

	// Input Pin Helpers
	ECheckBoxState GetCurrentIsInputPin() const;
	EVisibility GetInputPinCheckboxVisibility() const;
	bool GetInputPinCheckboxEnabled() const;
	// -- 

	// UI Generation Helpers
	TSharedRef<SWidget> GenerateMultiTypeWidget(TSharedPtr<FString> Item) const;
	FText GetSelectedMultiTypeText() const;
	// -- 

	// Change Handlers
	void OnMultiTypeChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnInputPinChanged(ECheckBoxState NewState);
	// -- 

	// Caching
	void CacheHandles(const TSharedRef<IPropertyHandle>& PropertyHandle,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils);
	void CacheOwnerInterface();
	// -- 
};

// ------------------------------------------------------------------------
// Generic Template Customization for Simple Value Types
// ------------------------------------------------------------------------
template <class T>
class TFlowDataPinValueCustomization : public FFlowDataPinValueCustomization
{
public:
	TFlowDataPinValueCustomization() = default;

	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new TFlowDataPinValueCustomization<T>());
	}
};