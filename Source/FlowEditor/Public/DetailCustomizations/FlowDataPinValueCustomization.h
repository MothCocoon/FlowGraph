// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Types/FlowPinType.h"
#include "Types/FlowDataPinValue.h"
#include "Types/FlowPinEnums.h"
#include "UnrealExtensions/IFlowExtendedPropertyTypeCustomization.h"
#include "IPropertyTypeCustomization.h"
#include "Widgets/Input/SComboBox.h"
#include "Templates/Function.h"

class IFlowDataPinValueOwnerInterface;

/*
* Flow Data Pin Value Customization
*
* Responsibilities:
*  - Header with MultiType selector + Input Pin checkbox.
*  - Child rows for single vs array modes (built conditionally now).
*  - Base for specialized enum/class/object customizations.
*  - Exposes array validation helpers.
*
* Dynamic Mode Switching:
*  - After MultiType changes, invokes OwnerInterface->RequestFlowDataPinValuesDetailsRebuild()
*    (implemented via owner-level detail customization) to force a full rebuild.
*/
class FLOWEDITOR_API FFlowDataPinValueCustomization : public IFlowExtendedPropertyTypeCustomization
{
	using Super = IFlowExtendedPropertyTypeCustomization;

protected:
	// Property handles
	TSharedPtr<IPropertyHandle> MultiTypeHandle;
	TSharedPtr<IPropertyHandle> ValuesHandle;
	TSharedPtr<IPropertyHandle> IsInputPinHandle;
	TSharedPtr<IPropertyUtilities> PropertyUtilities;

	// Cached context
	const FFlowPinType* DataPinType = nullptr;
	IPropertyTypeCustomizationUtils* CustomizationUtils = nullptr;
	IFlowDataPinValueOwnerInterface* OwnerInterface = nullptr;

	// MultiType UI state (enum values)
	TArray<TSharedPtr<int32>>                MultiTypeOptions;
	TSharedPtr<int32>                        SelectedMultiType;
	TSharedPtr<SComboBox<TSharedPtr<int32>>> MultiTypeComboBox;

	// Cached flag whether this pin type supports Array mode
	bool bArraySupported = true;

public:
	FFlowDataPinValueCustomization() = default;
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	// Non-copyable / non-movable
	FFlowDataPinValueCustomization(const FFlowDataPinValueCustomization&) = delete;
	FFlowDataPinValueCustomization& operator=(const FFlowDataPinValueCustomization&) = delete;
	FFlowDataPinValueCustomization(FFlowDataPinValueCustomization&&) = delete;
	FFlowDataPinValueCustomization& operator=(FFlowDataPinValueCustomization&&) = delete;

	// IPropertyTypeCustomization Interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle,
		FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle,
		IDetailChildrenBuilder& StructBuilder,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

protected:
	// Build flow
	virtual void BuildValueRows(TSharedRef<IPropertyHandle> InStructPropertyHandle,
		IDetailChildrenBuilder& StructBuilder,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils);

	virtual void BuildSingleBranch(IDetailChildrenBuilder& StructBuilder);
	virtual void BuildArrayBranch(IDetailChildrenBuilder& StructBuilder); // Skips if !bArraySupported

	void EnsureSingleElementExists();
	void RequestRefresh();

	// Mode / State
	EFlowDataMultiType GetCurrentMultiType() const;
	EVisibility GetSingleModeVisibility() const;
	EVisibility GetArrayModeVisibility() const;
	void TrimArrayToSingle();

	// Appearance
	FFlowDataPinValue* GetFlowDataPinValueBeingEdited() const
	{
		return IFlowExtendedPropertyTypeCustomization::TryGetTypedStructValue<FFlowDataPinValue>(StructPropertyHandle);
	}

	// Input Pin
	ECheckBoxState GetCurrentIsInputPin() const;
	EVisibility GetInputPinCheckboxVisibility() const;
	bool GetInputPinCheckboxEnabled() const;

	// MultiType UI Helpers
	TSharedRef<SWidget> GenerateMultiTypeWidget(TSharedPtr<int32> Item) const;
	FText GetSelectedMultiTypeText() const;

	// Change Handlers
	void OnMultiTypeChanged(TSharedPtr<int32> NewSelection, ESelectInfo::Type SelectInfo);
	void OnInputPinChanged(ECheckBoxState NewState);

	// Caching
	void CacheHandles(const TSharedRef<IPropertyHandle>& PropertyHandle,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils);
	void CacheOwnerInterface();
	void CacheArraySupported();

	// Shared Helpers ------------------------------------------------------
	void BuildVisibilityAwareArray(IDetailChildrenBuilder& StructBuilder,
		TSharedPtr<IPropertyHandle> ArrayHandle,
		TFunction<void(TSharedRef<IPropertyHandle>, int32, IDetailChildrenBuilder&, const TAttribute<EVisibility>&)> Generator,
		TAttribute<EVisibility> VisibilityAttribute);

	void ValidateArrayElements(TSharedPtr<IPropertyHandle> ArrayHandle,
		TFunction<bool(TSharedPtr<IPropertyHandle>)> IsValidPredicate,
		TFunction<void(TSharedPtr<IPropertyHandle>)> InvalidateAction);

	// Tooltips (centralized)
	static FText GetMultiTypeTooltip();
	static FText GetInputPinTooltip();
};

// Template customization for simple scalar value structs
template <class T>
class TFlowDataPinValueCustomization : public FFlowDataPinValueCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new TFlowDataPinValueCustomization<T>());
	}
};