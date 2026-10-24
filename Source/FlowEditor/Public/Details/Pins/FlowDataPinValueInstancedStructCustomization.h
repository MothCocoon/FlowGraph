// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "IDetailCustomNodeBuilder.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyEditorModule.h"
#include "PropertyHandle.h"

class IPropertyUtilities;
class UScriptStruct;

/**
 * Matches FInstancedStruct properties whose BaseStruct is FFlowDataPinValue or its subclass.
 */
class FFlowDataPinValueInstancedStructIdentifier : public IPropertyTypeIdentifier
{
public:
	virtual bool IsPropertyTypeCustomized(const IPropertyHandle& PropertyHandle) const override;
};

/**
 * Replaces the engine's FInstancedStruct customization for TInstancedStruct<FFlowDataPinValue>.
 * Type picker and child row show the short pin type name, e.g. "Int" instead of "Int - Flow DataPin Value".
 */
class FFlowDataPinValueInstancedStructCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	// IPropertyTypeCustomization
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	// --

	/* FlowPinType metadata of the struct, or its DisplayName if the metadata is missing. */
	static FText GetShortDisplayName(const UScriptStruct& Struct);

private:
	TSharedPtr<IPropertyHandle> StructPropertyHandle;
	TSharedPtr<IPropertyUtilities> PropertyUtilities;

	const UScriptStruct* GetBaseStruct() const;
	FPropertyAccess::Result GetCommonStruct(const UScriptStruct*& OutCommonStruct) const;

	FText GetSelectedStructText() const;
	FText GetSelectedStructTooltip() const;
	const FSlateBrush* GetSelectedStructIcon() const;

	TSharedRef<SWidget> GenerateStructMenu();
	void OnStructPicked(const UScriptStruct* Struct);
};

/**
 * Child rows of the Flow Data Pin Value held by FInstancedStruct, labeled with the short pin type name.
 */
class FFlowDataPinValueInstancedStructDataDetails : public IDetailCustomNodeBuilder
{
public:
	explicit FFlowDataPinValueInstancedStructDataDetails(const TSharedRef<IPropertyHandle>& InStructPropertyHandle);

	// IDetailCustomNodeBuilder
	virtual void SetOnRebuildChildren(FSimpleDelegate InOnRegenerateChildren) override;
	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override {}
	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override;
	virtual void Tick(float DeltaTime) override;
	virtual bool RequiresTick() const override { return true; }
	virtual bool InitiallyCollapsed() const override { return false; }
	virtual FName GetName() const override;
	// --

private:
	TSharedRef<IPropertyHandle> StructPropertyHandle;
	FSimpleDelegate OnRegenerateChildren;

	/* Struct type of each edited instance at the time child rows were generated. */
	TArray<TWeakObjectPtr<const UScriptStruct>> CachedInstanceTypes;

	TArray<TWeakObjectPtr<const UScriptStruct>> GetInstanceTypes() const;
};
