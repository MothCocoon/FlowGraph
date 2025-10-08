// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowDataPinValueCustomization_ObjectBase.h"

#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "Types/FlowDataPinValuesStandard.h"
#include "Interfaces/FlowDataPinValueOwnerInterface.h"
#include "IPropertyUtilities.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "UnrealExtensions/VisibilityArrayBuilder.h"
#include "DetailCustomizations/FlowValueSourcePolicy.h"
#include "EditorClassUtils.h"

#define LOCTEXT_NAMESPACE "FlowDataPinValueCustomization_ObjectBase"

void FFlowDataPinValueCustomization_ObjectBase::BuildValueRows(
	TSharedRef<IPropertyHandle> InStructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	CacheHandles(InStructPropertyHandle, StructCustomizationUtils);

	if (!ValuesHandle.IsValid())
	{
		return;
	}

	ClassFilterHandle = StructPropertyHandle->GetChildHandle(TEXT("ClassFilter"));

	TryApplyMetaClass();
	ResolveEffectiveFilter();
	ComputePolicy();

	if (SourcePolicy.bShowSourceRow && !SourcePolicy.bLocked && !SourcePolicy.bMetaForced && ClassFilterHandle.IsValid())
	{
		BuildClassFilterRow(StructBuilder);

		ClassFilterHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FFlowDataPinValueCustomization_ObjectBase::OnClassFilterChanged));
	}

	EnsureSingleElementExists();
	BuildSingleBranch(StructBuilder);
	BuildArrayBranch(StructBuilder);

	BindDelegates();
	ValidateAll();
}

void FFlowDataPinValueCustomization_ObjectBase::OnSourceLockToggled()
{
	ComputePolicy();

	if (CustomizationUtils)
	{
		if (auto Utils = CustomizationUtils->GetPropertyUtilities())
		{
			Utils->RequestRefresh();
		}
	}
}

void FFlowDataPinValueCustomization_ObjectBase::TryApplyMetaClass()
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

void FFlowDataPinValueCustomization_ObjectBase::ResolveEffectiveFilter()
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

void FFlowDataPinValueCustomization_ObjectBase::ComputePolicy()
{
	bool bPerValueLock = false;

	if (FFlowDataPinValue_Object* ObjStruct =
		IFlowExtendedPropertyTypeCustomization::TryGetTypedStructValue<FFlowDataPinValue_Object>(StructPropertyHandle))
	{
#if WITH_EDITORONLY_DATA
		bPerValueLock = ObjStruct->bLockClassFilter;
#endif
	}
	else if (FFlowDataPinValue_InstancedObject* InstStruct =
		IFlowExtendedPropertyTypeCustomization::TryGetTypedStructValue<FFlowDataPinValue_InstancedObject>(StructPropertyHandle))
	{
#if WITH_EDITORONLY_DATA
		bPerValueLock = InstStruct->bLockClassFilter;
#endif
	}

	const FFlowDataPinValue* BaseValue =
		IFlowExtendedPropertyTypeCustomization::TryGetTypedStructValue<FFlowDataPinValue>(StructPropertyHandle);

	SourcePolicy = ComputeFlowValueSourcePolicy(
		OwnerInterface,
		BaseValue,
		bMetaClassForced,
		bPerValueLock,
		true);
}

void FFlowDataPinValueCustomization_ObjectBase::BuildClassFilterRow(IDetailChildrenBuilder& StructBuilder)
{
	IDetailPropertyRow& Row = StructBuilder.AddProperty(ClassFilterHandle.ToSharedRef());
	Row.DisplayName(LOCTEXT("ObjClassFilter", "Class Filter"));
	Row.IsEnabled(SourcePolicy.bFinalEditableSource);
}

void FFlowDataPinValueCustomization_ObjectBase::BuildSingleBranch(IDetailChildrenBuilder& StructBuilder)
{
	auto First = ValuesHandle->GetChildHandle(0);

	if (!First.IsValid())
	{
		return;
	}

	StructBuilder.AddCustomRow(LOCTEXT("ObjectSingleSearch", "Object"))
		.Visibility(TAttribute<EVisibility>::CreateSP(this, &FFlowDataPinValueCustomization_ObjectBase::GetSingleModeVisibility))
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

void FFlowDataPinValueCustomization_ObjectBase::BuildArrayBranch(IDetailChildrenBuilder& StructBuilder)
{
	TSharedRef<FVisibilityArrayBuilder> ArrayBuilder =
		MakeShareable(new FVisibilityArrayBuilder(ValuesHandle.ToSharedRef(),
			true, true, true));

	ArrayBuilder->SetVisibilityGetter([this]()
		{
			return GetArrayModeVisibility();
		});

	ArrayBuilder->OnGenerateArrayElementWidget(
		FOnGenerateArrayElementWidgetVisible::CreateSP(
			this,
			&FFlowDataPinValueCustomization_ObjectBase::GenerateArrayElementRow));

	StructBuilder.AddCustomBuilder(ArrayBuilder);
}

void FFlowDataPinValueCustomization_ObjectBase::GenerateArrayElementRow(
	TSharedRef<IPropertyHandle> ElementHandle,
	int32 Index,
	IDetailChildrenBuilder& ChildBuilder,
	const TAttribute<EVisibility>& RowVisibility)
{
	IDetailPropertyRow& Row = ChildBuilder.AddProperty(ElementHandle);
	Row.Visibility(RowVisibility);

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
}

TSharedRef<SWidget> FFlowDataPinValueCustomization_ObjectBase::BuildObjectValueWidgetForElement(TSharedPtr<IPropertyHandle> ElementHandle)
{
	return SNew(SObjectPropertyEntryBox)
		.PropertyHandle(ElementHandle)
		.AllowedClass(EffectiveFilterClass.Get() ? EffectiveFilterClass.Get() : UObject::StaticClass())
		.AllowClear(SourcePolicy.bFinalEditableValues)
		.IsEnabled(SourcePolicy.bFinalEditableValues)
		.ToolTipText(SourcePolicy.bFinalEditableValues
			? LOCTEXT("ObjectPickerTooltip", "Select an object reference (filter may be locked).")
			: LOCTEXT("ObjectPickerLockedTooltip", "Object references are not editable by owner policy or metadata."));
}

void FFlowDataPinValueCustomization_ObjectBase::BindDelegates()
{
	if (ClassFilterHandle.IsValid())
	{
		ClassFilterHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FFlowDataPinValueCustomization_ObjectBase::OnClassFilterChanged));
	}

	if (ValuesHandle.IsValid())
	{
		ValuesHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FFlowDataPinValueCustomization_ObjectBase::OnValuesChanged));
	}
}

void FFlowDataPinValueCustomization_ObjectBase::OnClassFilterChanged()
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

void FFlowDataPinValueCustomization_ObjectBase::OnValuesChanged()
{
	ValidateAll();
}

void FFlowDataPinValueCustomization_ObjectBase::ValidateAll()
{
	if (!ValuesHandle.IsValid())
	{
		return;
	}

	UClass* Filter = EffectiveFilterClass.Get();

	if (!Filter)
	{
		return;
	}

	auto AsArray = ValuesHandle->AsArray();

	if (!AsArray.IsValid())
	{
		return;
	}

	uint32 Num = 0;
	AsArray->GetNumElements(Num);

	TArray<TSharedPtr<IPropertyHandle>> ToClear;
	ToClear.Reserve(Num);

	for (uint32 i = 0; i < Num; ++i)
	{
		auto Elem = ValuesHandle->GetChildHandle(i);

		if (!Elem.IsValid())
		{
			continue;
		}

		UObject* Obj = GetObjectValue(Elem);

		if (Obj && !Obj->IsA(Filter))
		{
			ToClear.Add(Elem);
		}
	}

	if (ToClear.Num() > 0)
	{
		const FScopedTransaction Tx(LOCTEXT("ClearInvalidObjects", "Clear Invalid Object References"));

		for (auto& H : ToClear)
		{
			if (H.IsValid())
			{
				SetObjectValue(H, nullptr);
			}
		}
	}
}

void FFlowDataPinValueCustomization_ObjectBase::ValidateElement(TSharedPtr<IPropertyHandle> ElementHandle, UClass* Filter)
{
	if (!ElementHandle.IsValid() || !Filter)
	{
		return;
	}

	UObject* Obj = GetObjectValue(ElementHandle);

	if (Obj && !Obj->IsA(Filter))
	{
		SetObjectValue(ElementHandle, nullptr);
	}
}

UObject* FFlowDataPinValueCustomization_ObjectBase::GetObjectValue(TSharedPtr<IPropertyHandle> ElementHandle) const
{
	UObject* Obj = nullptr;

	if (ElementHandle.IsValid())
	{
		ElementHandle->GetValue(Obj);
	}

	return Obj;
}

void FFlowDataPinValueCustomization_ObjectBase::SetObjectValue(TSharedPtr<IPropertyHandle> ElementHandle, UObject* NewObj)
{
	if (!ElementHandle.IsValid())
	{
		return;
	}

	ElementHandle->SetValue(NewObj);
}

UClass* FFlowDataPinValueCustomization_ObjectBase::GetCurrentFilterClassProperty() const
{
	if (!ClassFilterHandle.IsValid())
	{
		return nullptr;
	}

	UObject* Obj = nullptr;

	if (ClassFilterHandle->GetValue(Obj) == FPropertyAccess::Success)
	{
		return Cast<UClass>(Obj);
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE