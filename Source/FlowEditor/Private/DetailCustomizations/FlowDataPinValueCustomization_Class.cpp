// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowDataPinValueCustomization_Class.h"

#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "Types/FlowDataPinValuesStandard.h"
#include "EditorClassUtils.h"
#include "PropertyCustomizationHelpers.h"
#include "UObject/SoftObjectPath.h"
#include "UnrealExtensions/VisibilityArrayBuilder.h"
#include "IPropertyUtilities.h"
#include "Interfaces/FlowDataPinValueOwnerInterface.h"
#include "ScopedTransaction.h"
#include "DetailCustomizations/FlowValueSourcePolicy.h"

#define LOCTEXT_NAMESPACE "FlowDataPinValueCustomization_Class"

void FFlowDataPinValueCustomization_Class::BuildValueRows(
	TSharedRef<IPropertyHandle> InStructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	CacheHandles(InStructPropertyHandle, StructCustomizationUtils);

	if (!ValuesHandle.IsValid())
	{
		return;
	}

	ClassFilterHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinValue_Class, ClassFilter));

	TrySetClassFilterFromMetaData();
	ExtractMetadata();
	RefreshEffectiveFilter();
	ComputePolicy();

	// Source row visible only if not locked/forced and policy allows
	if (SourcePolicy.bShowSourceRow && !SourcePolicy.bLocked && !SourcePolicy.bMetaForced && ClassFilterHandle.IsValid())
	{
		BuildClassFilterRow(StructBuilder);

		ClassFilterHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FFlowDataPinValueCustomization_Class::OnClassFilterChanged));
	}

	EnsureSingleElementExists();
	BuildSingleBranch(StructBuilder);
	BuildArrayBranch(StructBuilder);

	BindValidationDelegates();
	ValidateAllElements();
}

void FFlowDataPinValueCustomization_Class::OnSourceLockToggled()
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

void FFlowDataPinValueCustomization_Class::ExtractMetadata()
{
	if (!StructPropertyHandle.IsValid())
	{
		return;
	}

	const FString& MustImplement = StructPropertyHandle->GetMetaData(TEXT("MustImplement"));
	RequiredInterface = FEditorClassUtils::GetClassFromString(MustImplement);

	bAllowAbstract = StructPropertyHandle->HasMetaData(TEXT("AllowAbstract"));
	bIsBlueprintBaseOnly = StructPropertyHandle->HasMetaData(TEXT("IsBlueprintBaseOnly")) ||
		StructPropertyHandle->HasMetaData(TEXT("BlueprintBaseOnly"));
	bShowTreeView = StructPropertyHandle->HasMetaData(TEXT("ShowTreeView"));
	bHideViewOptions = StructPropertyHandle->HasMetaData(TEXT("HideViewOptions"));
	bShowDisplayNames = StructPropertyHandle->HasMetaData(TEXT("ShowDisplayNames"));
	bMetaClassForced = StructPropertyHandle->HasMetaData(TEXT("MetaClass"));

	if (const FProperty* MetaProp = StructPropertyHandle->GetMetaDataProperty())
	{
		bAllowNone = !(MetaProp->PropertyFlags & CPF_NoClear);
	}
	else
	{
		bAllowNone = true;
	}
}

void FFlowDataPinValueCustomization_Class::ComputePolicy()
{
	FFlowDataPinValue_Class* ValueStruct =
		IFlowExtendedPropertyTypeCustomization::TryGetTypedStructValue<FFlowDataPinValue_Class>(StructPropertyHandle);

	bool bPerValueLock = false;

#if WITH_EDITORONLY_DATA
	if (ValueStruct)
	{
		bPerValueLock = ValueStruct->bLockClassFilter;
	}
#endif

	SourcePolicy = ComputeFlowValueSourcePolicy(
		OwnerInterface,
		reinterpret_cast<const FFlowDataPinValue*>(ValueStruct),
		bMetaClassForced,
		bPerValueLock,
		true);
}

void FFlowDataPinValueCustomization_Class::BuildClassFilterRow(IDetailChildrenBuilder& StructBuilder)
{
	IDetailPropertyRow& Row = StructBuilder.AddProperty(ClassFilterHandle.ToSharedRef());
	Row.DisplayName(LOCTEXT("ClassFilterLabel", "Class Filter"));
	Row.IsEnabled(SourcePolicy.bFinalEditableSource);
}

void FFlowDataPinValueCustomization_Class::BuildSingleBranch(IDetailChildrenBuilder& StructBuilder)
{
	auto First = ValuesHandle->GetChildHandle(0);

	if (!First.IsValid())
	{
		return;
	}

	StructBuilder.AddCustomRow(LOCTEXT("ClassSingleSearch", "Class"))
		.Visibility(TAttribute<EVisibility>::CreateSP(this, &FFlowDataPinValueCustomization_Class::GetSingleModeVisibility))
		.NameContent()
		[
			SNew(STextBlock)
				.Text(LOCTEXT("ClassValueLabel", "Class"))
				.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		.MinDesiredWidth(250.f)
		[
			SNew(SClassPropertyEntryBox)
				.MetaClass(CachedEffectiveFilter.Get() ? CachedEffectiveFilter.Get() : UObject::StaticClass())
				.RequiredInterface(RequiredInterface)
				.AllowAbstract(bAllowAbstract)
				.IsBlueprintBaseOnly(bIsBlueprintBaseOnly)
				.AllowNone(bAllowNone)
				.ShowTreeView(bShowTreeView)
				.HideViewOptions(bHideViewOptions)
				.ShowDisplayNames(bShowDisplayNames)
				.IsEnabled(SourcePolicy.bFinalEditableValues)
				.SelectedClass_Lambda([this, First]() -> const UClass*
					{
						return GetSelectedClassForHandle(First);
					})
				.OnSetClass_Lambda([this, First](const UClass* NewClass)
					{
						OnSetClassForHandle(NewClass, First);
					})
		];
}

void FFlowDataPinValueCustomization_Class::BuildArrayBranch(IDetailChildrenBuilder& StructBuilder)
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
			&FFlowDataPinValueCustomization_Class::GenerateArrayElementRow));

	StructBuilder.AddCustomBuilder(ArrayBuilder);
}

void FFlowDataPinValueCustomization_Class::GenerateArrayElementRow(
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
				.Text(FText::Format(LOCTEXT("ClassArrayElemLabelFmt", "Class {0}"), FText::AsNumber(Index)))
				.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		.MinDesiredWidth(250.f)
		[
			SNew(SClassPropertyEntryBox)
				.MetaClass(CachedEffectiveFilter.Get() ? CachedEffectiveFilter.Get() : UObject::StaticClass())
				.RequiredInterface(RequiredInterface)
				.AllowAbstract(bAllowAbstract)
				.IsBlueprintBaseOnly(bIsBlueprintBaseOnly)
				.AllowNone(bAllowNone)
				.ShowTreeView(bShowTreeView)
				.HideViewOptions(bHideViewOptions)
				.ShowDisplayNames(bShowDisplayNames)
				.IsEnabled(SourcePolicy.bFinalEditableValues)
				.SelectedClass_Lambda([this, ElementHandle]() -> const UClass*
					{
						return GetSelectedClassForHandle(ElementHandle);
					})
				.OnSetClass_Lambda([this, ElementHandle](const UClass* NewClass)
					{
						OnSetClassForHandle(NewClass, ElementHandle);
					})
		];
}

void FFlowDataPinValueCustomization_Class::BindValidationDelegates()
{
	if (ClassFilterHandle.IsValid())
	{
		ClassFilterHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FFlowDataPinValueCustomization_Class::OnClassFilterChanged));
	}

	if (ValuesHandle.IsValid())
	{
		ValuesHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FFlowDataPinValueCustomization_Class::OnValuesChanged));
	}
}

void FFlowDataPinValueCustomization_Class::OnClassFilterChanged()
{
	RefreshEffectiveFilter();
	ValidateAllElements();

	if (CustomizationUtils)
	{
		if (auto Utils = CustomizationUtils->GetPropertyUtilities())
		{
			Utils->RequestRefresh();
		}
	}
}

void FFlowDataPinValueCustomization_Class::OnValuesChanged()
{
	ValidateAllElements();
}

void FFlowDataPinValueCustomization_Class::TrySetClassFilterFromMetaData()
{
	if (!StructPropertyHandle.IsValid() || !ClassFilterHandle.IsValid())
	{
		return;
	}

	const FString& MetaClassName = StructPropertyHandle->GetMetaData(TEXT("MetaClass"));

	if (MetaClassName.IsEmpty())
	{
		return;
	}

	if (UClass* MetaClass = FEditorClassUtils::GetClassFromString(MetaClassName))
	{
		UObject* Existing = nullptr;
		ClassFilterHandle->GetValue(Existing);

		if (Existing != MetaClass)
		{
			ClassFilterHandle->SetValue(MetaClass, EPropertyValueSetFlags::DefaultFlags);
		}
	}
}

UClass* FFlowDataPinValueCustomization_Class::DeriveBestClassFilter() const
{
	if (!StructPropertyHandle.IsValid())
	{
		return nullptr;
	}

	const FString& MetaClassName = StructPropertyHandle->GetMetaData(TEXT("MetaClass"));

	if (!MetaClassName.IsEmpty())
	{
		if (UClass* MetaClass = FEditorClassUtils::GetClassFromString(MetaClassName))
		{
			return MetaClass;
		}
	}

	if (ClassFilterHandle.IsValid())
	{
		UObject* Raw = nullptr;

		if (ClassFilterHandle->GetValue(Raw) == FPropertyAccess::Success && Raw)
		{
			return Cast<UClass>(Raw);
		}
	}

	return nullptr;
}

void FFlowDataPinValueCustomization_Class::RefreshEffectiveFilter()
{
	CachedEffectiveFilter = DeriveBestClassFilter();
}

void FFlowDataPinValueCustomization_Class::ValidateAllElements()
{
	if (!ValuesHandle.IsValid())
	{
		return;
	}

	UClass* FilterClass = CachedEffectiveFilter.Get();

	if (!FilterClass)
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
		TSharedPtr<IPropertyHandle> Elem = ValuesHandle->GetChildHandle(i);

		if (!Elem.IsValid())
		{
			continue;
		}

		FString Path;

		if (!GetElementPathString(Elem, Path) || IsNoneString(Path))
		{
			continue;
		}

		FSoftClassPath SCP(Path);

		if (UClass* Loaded = SCP.TryLoadClass<UObject>())
		{
			if (!Loaded->IsChildOf(FilterClass))
			{
				ToClear.Add(Elem);
			}
		}
		else
		{
			ToClear.Add(Elem);
		}
	}

	if (ToClear.Num() > 0)
	{
		const FScopedTransaction Tx(LOCTEXT("ClearInvalidClassValues", "Clear Invalid Class Values"));

		for (const TSharedPtr<IPropertyHandle>& Elem : ToClear)
		{
			if (Elem.IsValid())
			{
				Elem->SetValueFromFormattedString(TEXT("None"));
			}
		}
	}
}

void FFlowDataPinValueCustomization_Class::ValidateElement(const TSharedPtr<IPropertyHandle>& ElementHandle, UClass* FilterClass)
{
	if (!ElementHandle.IsValid() || !FilterClass)
	{
		return;
	}

	FString Path;

	if (!GetElementPathString(ElementHandle, Path) || IsNoneString(Path))
	{
		return;
	}

	FSoftClassPath SCP(Path);

	if (UClass* Loaded = SCP.TryLoadClass<UObject>())
	{
		if (!Loaded->IsChildOf(FilterClass))
		{
			ElementHandle->SetValueFromFormattedString(TEXT("None"));
		}
	}
	else
	{
		ElementHandle->SetValueFromFormattedString(TEXT("None"));
	}
}

const UClass* FFlowDataPinValueCustomization_Class::GetSelectedClassForHandle(TSharedPtr<IPropertyHandle> ElementHandle) const
{
	if (!ElementHandle.IsValid())
	{
		return nullptr;
	}

	FString Path;

	if (ElementHandle->GetValueAsFormattedString(Path) != FPropertyAccess::Success)
	{
		return nullptr;
	}

	if (IsNoneString(Path))
	{
		return nullptr;
	}

	return FEditorClassUtils::GetClassFromString(Path);
}

void FFlowDataPinValueCustomization_Class::OnSetClassForHandle(const UClass* NewClass, TSharedPtr<IPropertyHandle> ElementHandle)
{
	if (!ElementHandle.IsValid())
	{
		return;
	}

	const UClass* Filter = CachedEffectiveFilter.Get();

	if (Filter && NewClass && !NewClass->IsChildOf(Filter))
	{
		NewClass = nullptr;
	}

	const FString NewValue = NewClass ? NewClass->GetPathName() : TEXT("None");
	ElementHandle->SetValueFromFormattedString(NewValue);

	if (Filter)
	{
		ValidateElement(ElementHandle, const_cast<UClass*>(Filter));
	}
}

bool FFlowDataPinValueCustomization_Class::GetElementPathString(const TSharedPtr<IPropertyHandle>& ElementHandle, FString& OutPath) const
{
	if (!ElementHandle.IsValid())
	{
		return false;
	}

	return ElementHandle->GetValueAsFormattedString(OutPath) == FPropertyAccess::Success;
}

bool FFlowDataPinValueCustomization_Class::IsNoneString(const FString& Str) const
{
	return Str.IsEmpty() || Str.Equals(TEXT("None"), ESearchCase::IgnoreCase);
}

#undef LOCTEXT_NAMESPACE