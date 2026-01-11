// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowDataPinValueCustomization_Object.h"
#include "Interfaces/FlowDataPinValueOwnerInterface.h"
#include "Types/FlowDataPinValuesStandard.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "IPropertyUtilities.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "EditorClassUtils.h"

#define LOCTEXT_NAMESPACE "FlowDataPinValueCustomization_Object"

FFlowDataPinValue_Object* FFlowDataPinValueCustomization_Object::GetValueStruct() const
{
	return IFlowExtendedPropertyTypeCustomization::TryGetTypedStructValue<FFlowDataPinValue_Object>(StructPropertyHandle);
}

bool FFlowDataPinValueCustomization_Object::ShouldShowSourceRow() const
{
	return OwnerInterface ? OwnerInterface->ShowFlowDataPinValueClassFilter(GetValueStruct()) : true;
}

bool FFlowDataPinValueCustomization_Object::IsSourceEditable() const
{
	if (bMetaClassForced)
	{
		return false;
	}
	return OwnerInterface ? OwnerInterface->CanEditFlowDataPinValueClassFilter(GetValueStruct()) : true;
}

void FFlowDataPinValueCustomization_Object::BuildValueRows(
	TSharedRef<IPropertyHandle> InStructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	CacheHandles(InStructPropertyHandle, StructCustomizationUtils);
	CacheArraySupported(); // base
	if (!ValuesHandle.IsValid())
	{
		return;
	}

	ClassFilterHandle = StructPropertyHandle->GetChildHandle(TEXT("ClassFilter"));

	TryApplyMetaClass();
	ResolveEffectiveFilter();

	const bool bShowSource = ShouldShowSourceRow();
	if (bShowSource)
	{
		BuildClassFilterRow(StructBuilder, IsSourceEditable());
	}

	EnsureSingleElementExists();
	BuildSingleBranch(StructBuilder);
	if (bArraySupported)
	{
		BuildArrayBranch(StructBuilder);
	}

	BindDelegates();
	ValidateAll();
}

void FFlowDataPinValueCustomization_Object::TryApplyMetaClass()
{
	if (!StructPropertyHandle.IsValid() || !ClassFilterHandle.IsValid())
	{
		return;
	}

	const FString& MetaClassName = StructPropertyHandle->GetMetaData(TEXT("MetaClass"));
	if (MetaClassName.IsEmpty())
	{
		bMetaClassForced = false;
		return;
	}

	if (UClass* Meta = FEditorClassUtils::GetClassFromString(MetaClassName))
	{
		UObject* Existing = nullptr;
		ClassFilterHandle->GetValue(Existing);
		if (Existing != Meta)
		{
			ClassFilterHandle->SetValue(Meta, EPropertyValueSetFlags::DefaultFlags);
		}
		bMetaClassForced = true;
	}
	else
	{
		bMetaClassForced = false;
	}
}

void FFlowDataPinValueCustomization_Object::ResolveEffectiveFilter()
{
	if (bMetaClassForced)
	{
		const FString& MetaClassName = StructPropertyHandle->GetMetaData(TEXT("MetaClass"));
		EffectiveFilterClass = FEditorClassUtils::GetClassFromString(MetaClassName);
		return;
	}

	if (ClassFilterHandle.IsValid())
	{
		UObject* Obj = nullptr;
		if (ClassFilterHandle->GetValue(Obj) == FPropertyAccess::Success)
		{
			EffectiveFilterClass = Cast<UClass>(Obj);
			return;
		}
	}

	EffectiveFilterClass = nullptr;
}

void FFlowDataPinValueCustomization_Object::BuildClassFilterRow(IDetailChildrenBuilder& StructBuilder, bool bSourceEditable) const
{
	if (!ClassFilterHandle.IsValid())
	{
		return;
	}

	IDetailPropertyRow& Row = StructBuilder.AddProperty(ClassFilterHandle.ToSharedRef());
	Row.DisplayName(LOCTEXT("ObjClassFilter", "Class Filter"));
	Row.IsEnabled(bSourceEditable);
	Row.ToolTip(bMetaClassForced
		? LOCTEXT("ObjClassFilterMetaTooltip", "Class Filter is fixed by MetaClass metadata and cannot be edited.")
		: LOCTEXT("ObjClassFilterTooltip", "Class Filter constrains which object classes are selectable."));
}

void FFlowDataPinValueCustomization_Object::BuildSingleBranch(IDetailChildrenBuilder& StructBuilder)
{
	auto First = ValuesHandle->GetChildHandle(0);
	if (!First.IsValid())
	{
		return;
	}

	StructBuilder.AddCustomRow(LOCTEXT("ObjectSingleSearch", "Object"))
		.Visibility(TAttribute<EVisibility>::CreateSP(this, &FFlowDataPinValueCustomization_Object::GetSingleModeVisibility))
		.NameContent()
		[
			SNew(STextBlock)
				.Text(LOCTEXT("ObjectValueLabel", "Object"))
				.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		.MinDesiredWidth(250.f)
		[
			BuildObjectValueWidgetForElement(First)
		];
}

void FFlowDataPinValueCustomization_Object::BuildArrayBranch(IDetailChildrenBuilder& StructBuilder)
{
	BuildVisibilityAwareArray(StructBuilder,
		ValuesHandle,
		[this](TSharedRef<IPropertyHandle> ElementHandle, int32 Index, IDetailChildrenBuilder& ChildBuilder, const TAttribute<EVisibility>& RowVis)
		{
			IDetailPropertyRow& Row = ChildBuilder.AddProperty(ElementHandle);
			Row.Visibility(RowVis);

			Row.CustomWidget()
				.NameContent()
				[
					SNew(STextBlock)
						.Text(FText::Format(LOCTEXT("ObjectArrayElemFmt", "Object {0}"), FText::AsNumber(Index)))
						.Font(IDetailLayoutBuilder::GetDetailFont())
				]
				.ValueContent()
				.MinDesiredWidth(250.f)
				[
					BuildObjectValueWidgetForElement(ElementHandle)
				];
		},
		TAttribute<EVisibility>::CreateSP(this, &FFlowDataPinValueCustomization_Object::GetArrayModeVisibility));
}

TSharedRef<SWidget> FFlowDataPinValueCustomization_Object::BuildObjectValueWidgetForElement(TSharedPtr<IPropertyHandle> ElementHandle) const
{
	return SNew(SObjectPropertyEntryBox)
		.PropertyHandle(ElementHandle)
		.AllowedClass(EffectiveFilterClass.Get() ? EffectiveFilterClass.Get() : UObject::StaticClass())
		.AllowClear(AreValuesEditable())
		.IsEnabled(AreValuesEditable())
		.ToolTipText(AreValuesEditable()
			? LOCTEXT("ObjectPickerTooltip", "Select an object reference.")
			: LOCTEXT("ObjectPickerLockedTooltip", "Object references are not editable."));
}

void FFlowDataPinValueCustomization_Object::BindDelegates()
{
	if (ClassFilterHandle.IsValid())
	{
		ClassFilterHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FFlowDataPinValueCustomization_Object::OnClassFilterChanged));
	}
	if (ValuesHandle.IsValid())
	{
		ValuesHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FFlowDataPinValueCustomization_Object::OnValuesChanged));
	}
}

void FFlowDataPinValueCustomization_Object::OnClassFilterChanged()
{
	ResolveEffectiveFilter();
	ValidateAll();

	if (CustomizationUtils)
	{
		if (auto Utils = CustomizationUtils->GetPropertyUtilities())
		{
			Utils->RequestRefresh();
		}
	}
}

void FFlowDataPinValueCustomization_Object::OnValuesChanged()
{
	ValidateAll();
}

void FFlowDataPinValueCustomization_Object::ValidateAll()
{
	ValidateArrayElements(ValuesHandle,
		[this](TSharedPtr<IPropertyHandle> Elem)
		{
			return IsElementValid(Elem);
		},
		[this](TSharedPtr<IPropertyHandle> Elem)
		{
			InvalidateElement(Elem);
		});
}

bool FFlowDataPinValueCustomization_Object::IsElementValid(TSharedPtr<IPropertyHandle> ElementHandle) const
{
	if (!ElementHandle.IsValid())
	{
		return true;
	}

	UClass* Filter = EffectiveFilterClass.Get();
	if (!Filter)
	{
		return true;
	}

	UObject* Obj = GetObjectValue(ElementHandle);
	return !Obj || Obj->IsA(Filter);
}

void FFlowDataPinValueCustomization_Object::InvalidateElement(TSharedPtr<IPropertyHandle> ElementHandle)
{
	if (ElementHandle.IsValid())
	{
		SetObjectValue(ElementHandle, nullptr);
	}
}

UObject* FFlowDataPinValueCustomization_Object::GetObjectValue(TSharedPtr<IPropertyHandle> ElementHandle)
{
	UObject* Obj = nullptr;
	if (ElementHandle.IsValid())
	{
		ElementHandle->GetValue(Obj);
	}
	return Obj;
}

void FFlowDataPinValueCustomization_Object::SetObjectValue(TSharedPtr<IPropertyHandle> ElementHandle, UObject* NewObj)
{
	if (!ElementHandle.IsValid())
	{
		return;
	}

	UObject* Current = nullptr;
	ElementHandle->GetValue(Current);
	if (Current == NewObj)
	{
		return;
	}

	FScopedTransaction Tx(LOCTEXT("SetObjectValue", "Set Object Reference"));
	ElementHandle->SetValue(NewObj);
}

#undef LOCTEXT_NAMESPACE
