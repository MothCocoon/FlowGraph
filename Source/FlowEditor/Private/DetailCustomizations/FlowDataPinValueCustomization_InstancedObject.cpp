// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowDataPinValueCustomization_InstancedObject.h"

#include "PropertyHandle.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "IPropertyUtilities.h"
#include "Types/FlowDataPinValuesStandard.h"
#include "Interfaces/FlowDataPinValueOwnerInterface.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
#include "UObject/UObjectIterator.h"
#include "EditorClassUtils.h"
#include "IDetailPropertyRow.h"
#include "IDetailChildrenBuilder.h"

#define LOCTEXT_NAMESPACE "FlowDataPinValueCustomization_InstancedObject"

// ---------------------------
// Metadata Extraction
// ---------------------------
void FFlowDataPinValueCustomization_InstancedObject::ExtractInterfaceMetadata()
{
	RequiredInterface = nullptr;

	if (!StructPropertyHandle.IsValid())
	{
		return;
	}

	const FString& MustImplement = StructPropertyHandle->GetMetaData(TEXT("MustImplement"));

	if (!MustImplement.IsEmpty())
	{
		RequiredInterface = FEditorClassUtils::GetClassFromString(MustImplement);
	}
}

// ---------------------------
// ClassFilter Row
// ---------------------------
void FFlowDataPinValueCustomization_InstancedObject::BuildClassFilterRow(IDetailChildrenBuilder& StructBuilder)
{
	if (!ClassFilterHandle.IsValid())
	{
		return;
	}

	ExtractInterfaceMetadata();

	IDetailPropertyRow& Row = StructBuilder.AddProperty(ClassFilterHandle.ToSharedRef());
	Row.DisplayName(LOCTEXT("InstancedObjClassFilter", "Class Filter"));

	TSharedPtr<IPropertyHandle> LocalHandle = ClassFilterHandle;

	TArray<TSharedRef<IClassViewerFilter>> Filters;
	BuildClassFilterFilters(Filters);

	Row.CustomWidget()
		.NameContent()
		[
			LocalHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(250.f)
		[
			SNew(SClassPropertyEntryBox)
				.MetaClass(UObject::StaticClass())
				.AllowAbstract(true)             // Abstract allowed
				.IsBlueprintBaseOnly(false)
				.AllowNone(true)
				.HideViewOptions(false)
				.ShowDisplayNames(true)
				.ShowTreeView(true)
				.RequiredInterface(nullptr)
				.ClassViewerFilters(Filters)
				.IsEnabled(SourcePolicy.bFinalEditableSource)
				.SelectedClass_Lambda([LocalHandle]() -> const UClass*
					{
						UObject* ObjVal = nullptr;

						if (LocalHandle->GetValue(ObjVal) == FPropertyAccess::Success && ObjVal)
						{
							return Cast<UClass>(ObjVal);
						}

						return nullptr;
					})
				.OnSetClass_Lambda([this, LocalHandle](const UClass* NewClass)
					{
						if (!LocalHandle.IsValid())
						{
							return;
						}

						if (NewClass)
						{
							// MustImplement interface
							if (RequiredInterface && !NewClass->ImplementsInterface(RequiredInterface))
							{
								NewClass = nullptr;
							}

							// Require EditInlineNew for the filter
							if (NewClass && !NewClass->HasAnyClassFlags(CLASS_EditInlineNew))
							{
								NewClass = nullptr;
							}
						}

						LocalHandle->SetValue(const_cast<UClass*>(NewClass));
						OnClassFilterChanged();
					})
		];

	Row.IsEnabled(SourcePolicy.bFinalEditableSource);
}

// ---------------------------
// Value Widget (instantiate if null)
// ---------------------------
TSharedRef<SWidget> FFlowDataPinValueCustomization_InstancedObject::BuildObjectValueWidgetForElement(TSharedPtr<IPropertyHandle> ElementHandle)
{
	if (!ElementHandle.IsValid())
	{
		return SNew(STextBlock).Text(LOCTEXT("InvalidHandle", "<Invalid>"));
	}

	UObject* CurrentObj = nullptr;
	ElementHandle->GetValue(CurrentObj);

	if (CurrentObj)
	{
		BindPerElementValueChange(ElementHandle);
		return ElementHandle->CreatePropertyValueWidget();
	}

	ExtractInterfaceMetadata();

	// Value editing disabled by owner policy (not by lock)
	if (!SourcePolicy.bFinalEditableValues)
	{
		return SNew(STextBlock)
			.Text(LOCTEXT("ValuesDisabled", "(Values Locked by Owner Policy)"))
			.ColorAndOpacity(FLinearColor::Gray);
	}

	// Scan once for instantiables
	if (!bInstantiableScanDone)
	{
		bHasAnyInstantiable = ScanForAnyInstantiable();
		bInstantiableScanDone = true;
	}

	if (!bHasAnyInstantiable)
	{
		return SNew(STextBlock)
			.Text(LOCTEXT("NoInstantiableFound",
				"No instantiable (EditInlineNew, non-abstract) subclass found.\n"
				"Adjust Class Filter or add a concrete EditInlineNew subclass."))
			.AutoWrapText(true);
	}

	TArray<TSharedRef<IClassViewerFilter>> Filters;
	BuildInstantiationFilters(Filters);

	return SNew(SClassPropertyEntryBox)
		.MetaClass(UObject::StaticClass())
		.AllowAbstract(false)
		.IsBlueprintBaseOnly(false)
		.AllowNone(false)
		.HideViewOptions(false)
		.ShowDisplayNames(true)
		.ShowTreeView(true)
		.RequiredInterface(nullptr)
		.ClassViewerFilters(Filters)
		.SelectedClass_Lambda([]() -> const UClass* { return nullptr; })
		.OnSetClass_Lambda([this, ElementHandle](const UClass* ChosenClass)
			{
				if (!CanInstantiateClass(ChosenClass))
				{
					return;
				}

				InstantiateForHandle(ElementHandle, ChosenClass);
			});
}

// ---------------------------
// Filters
// ---------------------------
bool FFlowDataPinValueCustomization_InstancedObject::FClassFilterRowFilter::IsClassAllowed(
	const FClassViewerInitializationOptions&,
	const UClass* InClass,
	TSharedRef<FClassViewerFilterFuncs>)
{
	if (!InClass)
	{
		return false;
	}

	if (Base.IsValid() && !InClass->IsChildOf(Base.Get()))
	{
		return false;
	}

	if (RequiredInterface.IsValid() && !InClass->ImplementsInterface(RequiredInterface.Get()))
	{
		return false;
	}

	if (!InClass->HasAnyClassFlags(CLASS_EditInlineNew))
	{
		return false;
	}

	// Abstract allowed here
	return true;
}

bool FFlowDataPinValueCustomization_InstancedObject::FClassFilterRowFilter::IsUnloadedClassAllowed(
	const FClassViewerInitializationOptions&,
	const TSharedRef<const IUnloadedBlueprintData>,
	TSharedRef<FClassViewerFilterFuncs>)
{
	return false;
}

bool FFlowDataPinValueCustomization_InstancedObject::FInstantiationFilter::IsClassAllowed(
	const FClassViewerInitializationOptions&,
	const UClass* InClass,
	TSharedRef<FClassViewerFilterFuncs>)
{
	if (!InClass)
	{
		return false;
	}

	if (Base.IsValid() && !InClass->IsChildOf(Base.Get()))
	{
		return false;
	}

	if (RequiredInterface.IsValid() && !InClass->ImplementsInterface(RequiredInterface.Get()))
	{
		return false;
	}

	if (InClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return false;
	}

	if (!InClass->HasAnyClassFlags(CLASS_EditInlineNew))
	{
		return false;
	}

	return true;
}

bool FFlowDataPinValueCustomization_InstancedObject::FInstantiationFilter::IsUnloadedClassAllowed(
	const FClassViewerInitializationOptions&,
	const TSharedRef<const IUnloadedBlueprintData>,
	TSharedRef<FClassViewerFilterFuncs>)
{
	return false;
}

void FFlowDataPinValueCustomization_InstancedObject::BuildClassFilterFilters(TArray<TSharedRef<IClassViewerFilter>>& Out) const
{
	TSharedRef<FClassFilterRowFilter> Filter = MakeShared<FClassFilterRowFilter>();
	Filter->Base = nullptr;
	Filter->RequiredInterface = RequiredInterface;
	Out.Add(Filter);
}

void FFlowDataPinValueCustomization_InstancedObject::BuildInstantiationFilters(TArray<TSharedRef<IClassViewerFilter>>& Out) const
{
	TSharedRef<FInstantiationFilter> Filter = MakeShared<FInstantiationFilter>();
	Filter->Base = EffectiveFilterClass;
	Filter->RequiredInterface = RequiredInterface;
	Out.Add(Filter);
}

// ---------------------------
// Enumeration for instantiables
// ---------------------------
bool FFlowDataPinValueCustomization_InstancedObject::ScanForAnyInstantiable() const
{
	UClass* BaseClass = EffectiveFilterClass.Get();

	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* C = *It;

		if (!C)
		{
			continue;
		}

		if (BaseClass && !C->IsChildOf(BaseClass))
		{
			continue;
		}

		if (RequiredInterface && !C->ImplementsInterface(RequiredInterface))
		{
			continue;
		}

		if (C->HasAnyClassFlags(CLASS_Abstract))
		{
			continue;
		}

		if (!C->HasAnyClassFlags(CLASS_EditInlineNew))
		{
			continue;
		}

		return true;
	}

	return false;
}

// ---------------------------
// Validation / Instantiation
// ---------------------------
bool FFlowDataPinValueCustomization_InstancedObject::CanInstantiateClass(const UClass* Candidate) const
{
	if (!Candidate)
	{
		return false;
	}

	if (RequiredInterface && !Candidate->ImplementsInterface(RequiredInterface))
	{
		return false;
	}

	if (EffectiveFilterClass.IsValid() && !Candidate->IsChildOf(EffectiveFilterClass.Get()))
	{
		return false;
	}

	if (Candidate->HasAnyClassFlags(CLASS_Abstract))
	{
		return false;
	}

	if (!Candidate->HasAnyClassFlags(CLASS_EditInlineNew))
	{
		return false;
	}

	return true;
}

void FFlowDataPinValueCustomization_InstancedObject::InstantiateForHandle(TSharedPtr<IPropertyHandle> ElementHandle, const UClass* ChosenClass)
{
	if (!ElementHandle.IsValid() || !ChosenClass)
	{
		return;
	}

	if (!CanInstantiateClass(ChosenClass))
	{
		return;
	}

	FScopedTransaction Tx(LOCTEXT("CreateInstancedObjectPinValue", "Create Instanced Object Pin Value"));

	UObject* Outer = ResolveOuterForNewObject();

	if (!Outer)
	{
		Outer = GetTransientPackage();
	}

	Outer->Modify();

	UObject* NewObj = NewObject<UObject>(
		Outer,
		const_cast<UClass*>(ChosenClass),
		NAME_None,
		RF_Transactional);

	if (!NewObj)
	{
		return;
	}

	NewObj->SetFlags(RF_Transactional);

	if (!SetInstancedObjectHandleDirect(ElementHandle, NewObj))
	{
		SetInstancedObjectHandleDirect(ElementHandle, nullptr);
		return;
	}

	BindPerElementValueChange(ElementHandle);

	if (CustomizationUtils)
	{
		if (TSharedPtr<IPropertyUtilities> Utils = CustomizationUtils->GetPropertyUtilities())
		{
			Utils->RequestRefresh();
		}
	}
}

// Direct pointer write for instanced property
bool FFlowDataPinValueCustomization_InstancedObject::SetInstancedObjectHandleDirect(
	TSharedPtr<IPropertyHandle> ElementHandle,
	UObject* NewValue)
{
	if (!ElementHandle.IsValid())
	{
		return false;
	}

	void* Address = nullptr;

	if (ElementHandle->GetValueData(Address) != FPropertyAccess::Success || !Address)
	{
		return false;
	}

	TArray<UObject*> Outers;
	ElementHandle->GetOuterObjects(Outers);

	if (Outers.Num() > 0 && Outers[0])
	{
		Outers[0]->Modify();
	}

	ElementHandle->NotifyPreChange();

	UObject** Ptr = reinterpret_cast<UObject**>(Address);
	*Ptr = NewValue;

	ElementHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	ElementHandle->NotifyFinishedChangingProperties();

	return true;
}

UObject* FFlowDataPinValueCustomization_InstancedObject::ResolveOuterForNewObject() const
{
	if (!StructPropertyHandle.IsValid())
	{
		return nullptr;
	}

	TArray<UObject*> Outers;
	StructPropertyHandle->GetOuterObjects(Outers);

	if (Outers.Num() > 0)
	{
		return Outers[0];
	}

	return nullptr;
}

void FFlowDataPinValueCustomization_InstancedObject::BindPerElementValueChange(TSharedPtr<IPropertyHandle> ElementHandle)
{
	if (!ElementHandle.IsValid())
	{
		return;
	}

	ElementHandle->SetOnPropertyValueChanged(
		FSimpleDelegate::CreateSP(this, &FFlowDataPinValueCustomization_InstancedObject::OnElementValueChanged, ElementHandle));
}

void FFlowDataPinValueCustomization_InstancedObject::OnElementValueChanged(TSharedPtr<IPropertyHandle> ElementHandle)
{
	if (!ElementHandle.IsValid())
	{
		return;
	}

	UClass* Filter = EffectiveFilterClass.Get();

	if (!Filter)
	{
		return;
	}

	void* Address = nullptr;

	if (ElementHandle->GetValueData(Address) != FPropertyAccess::Success || !Address)
	{
		return;
	}

	UObject* Obj = *reinterpret_cast<UObject**>(Address);

	if (Obj && !Obj->IsA(Filter))
	{
		SetInstancedObjectHandleDirect(ElementHandle, nullptr);
	}

	if (CustomizationUtils)
	{
		if (TSharedPtr<IPropertyUtilities> Utils = CustomizationUtils->GetPropertyUtilities())
		{
			Utils->RequestRefresh();
		}
	}
}

#undef LOCTEXT_NAMESPACE