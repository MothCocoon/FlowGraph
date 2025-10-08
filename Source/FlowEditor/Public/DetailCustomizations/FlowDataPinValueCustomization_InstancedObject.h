// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "DetailCustomizations/FlowDataPinValueCustomization_ObjectBase.h"
#include "ClassViewerFilter.h"
#include "UObject/WeakObjectPtr.h"

/*
* InstancedObject customization:
*  - ClassFilter row: allows abstract classes, but requires EditInlineNew and interface (MustImplement) if specified.
*  - Instantiation picker (when value is null): requires non-abstract + EditInlineNew + interface + subclass of EffectiveFilterClass.
*  - If no instantiable subclasses exist, shows a message instead of picker.
*  - Lock only affects ClassFilter editing (not instantiation).
*  - Direct pointer assignment for instanced objects.
*/
class FLOWEDITOR_API FFlowDataPinValueCustomization_InstancedObject : public FFlowDataPinValueCustomization_ObjectBase
{
	using Super = FFlowDataPinValueCustomization_ObjectBase;

public:
	FFlowDataPinValueCustomization_InstancedObject() = default;

	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FFlowDataPinValueCustomization_InstancedObject());
	}

	// Non-copyable / non-movable
	FFlowDataPinValueCustomization_InstancedObject(const FFlowDataPinValueCustomization_InstancedObject&) = delete;
	FFlowDataPinValueCustomization_InstancedObject& operator=(const FFlowDataPinValueCustomization_InstancedObject&) = delete;
	FFlowDataPinValueCustomization_InstancedObject(FFlowDataPinValueCustomization_InstancedObject&&) = delete;
	FFlowDataPinValueCustomization_InstancedObject& operator=(FFlowDataPinValueCustomization_InstancedObject&&) = delete;

protected:
	virtual bool SupportsInlineCreation() const override { return true; }
	virtual void BuildClassFilterRow(IDetailChildrenBuilder& StructBuilder) override;
	virtual TSharedRef<SWidget> BuildObjectValueWidgetForElement(TSharedPtr<IPropertyHandle> ElementHandle) override;

private:
	class FClassFilterRowFilter : public IClassViewerFilter
	{
	public:
		TWeakObjectPtr<UClass> Base;
		TWeakObjectPtr<UClass> RequiredInterface;

		virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions,
			const UClass* InClass,
			TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override;

		virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions&,
			const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData,
			TSharedRef<FClassViewerFilterFuncs>) override;
	};

	class FInstantiationFilter : public IClassViewerFilter
	{
	public:
		TWeakObjectPtr<UClass> Base;
		TWeakObjectPtr<UClass> RequiredInterface;

		virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions,
			const UClass* InClass,
			TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override;

		virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions&,
			const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData,
			TSharedRef<FClassViewerFilterFuncs>) override;
	};

	// Metadata (MustImplement interface)
	UClass* RequiredInterface = nullptr;

	// Cached scan result for instantiable subclasses
	mutable bool bInstantiableScanDone = false;
	mutable bool bHasAnyInstantiable = false;

	// Metadata / filtering
	void ExtractInterfaceMetadata();
	void BuildClassFilterFilters(TArray<TSharedRef<IClassViewerFilter>>& Out) const;
	void BuildInstantiationFilters(TArray<TSharedRef<IClassViewerFilter>>& Out) const;

	// Instantiation scanning
	bool ScanForAnyInstantiable() const;
	bool CanInstantiateClass(const UClass* Candidate) const;

	// Object creation / assignment
	void InstantiateForHandle(TSharedPtr<IPropertyHandle> ElementHandle, const UClass* ChosenClass);
	UObject* ResolveOuterForNewObject() const;
	bool SetInstancedObjectHandleDirect(TSharedPtr<IPropertyHandle> ElementHandle, UObject* NewValue);

	// Per-element change tracking
	void BindPerElementValueChange(TSharedPtr<IPropertyHandle> ElementHandle);
	void OnElementValueChanged(TSharedPtr<IPropertyHandle> ElementHandle);
};