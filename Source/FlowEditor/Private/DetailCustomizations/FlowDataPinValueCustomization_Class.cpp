// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowDataPinValueCustomization_Class.h"
#include "Interfaces/FlowDataPinValueOwnerInterface.h"
#include "Types/FlowDataPinValuesStandard.h"
#include "UnrealExtensions/VisibilityArrayBuilder.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "EditorClassUtils.h"
#include "UObject/SoftObjectPath.h"
#include "IPropertyUtilities.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "FlowDataPinValueCustomization_Class"

FFlowDataPinValue_Class* FFlowDataPinValueCustomization_Class::GetValueStruct() const
{
	return IFlowExtendedPropertyTypeCustomization::TryGetTypedStructValue<FFlowDataPinValue_Class>(StructPropertyHandle);
}

bool FFlowDataPinValueCustomization_Class::ShouldShowSourceRow() const
{
	return OwnerInterface ? OwnerInterface->ShowFlowDataPinValueClassFilter(GetValueStruct()) : true;
}

bool FFlowDataPinValueCustomization_Class::IsSourceEditable() const
{
	if (bHasMetaClass)
	{
		return false; // forced meta class: show disabled
	}
	return OwnerInterface ? OwnerInterface->CanEditFlowDataPinValueClassFilter(GetValueStruct()) : true;
}

void FFlowDataPinValueCustomization_Class::BuildValueRows(
	TSharedRef<IPropertyHandle> InStructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	CacheHandles(InStructPropertyHandle, StructCustomizationUtils);
	CacheArraySupported(); // from base
	if (!ValuesHandle.IsValid())
	{
		return;
	}

	ClassFilterHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinValue_Class, ClassFilter));

	TrySetClassFilterFromMetaData();
	ExtractMetadata();
	RefreshEffectiveFilter();

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
	ValidateAllElements();
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

	bHasMetaClass = !StructPropertyHandle->GetMetaData(TEXT("MetaClass")).IsEmpty();

	if (const FProperty* MetaProp = StructPropertyHandle->GetMetaDataProperty())
	{
		bAllowNone = !(MetaProp->PropertyFlags & CPF_NoClear);
	}
	else
	{
		bAllowNone = true;
	}
}

void FFlowDataPinValueCustomization_Class::BuildClassFilterRow(IDetailChildrenBuilder& StructBuilder, bool bSourceEditable) const
{
	if (!ClassFilterHandle.IsValid())
	{
		return;
	}

	IDetailPropertyRow& Row = StructBuilder.AddProperty(ClassFilterHandle.ToSharedRef());
	Row.DisplayName(LOCTEXT("ClassFilterLabel", "Class Filter"));
	Row.IsEnabled(bSourceEditable);
	Row.ToolTip(bHasMetaClass
		? LOCTEXT("ClassFilterMetaTooltip", "Class Filter is fixed by MetaClass metadata and cannot be edited.")
		: LOCTEXT("ClassFilterTooltip", "Class Filter constrains which classes can be selected."));
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
				.IsEnabled(AreValuesEditable())
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
						.IsEnabled(AreValuesEditable())
						.SelectedClass_Lambda([this, ElementHandle]() -> const UClass*
							{
								return GetSelectedClassForHandle(ElementHandle);
							})
						.OnSetClass_Lambda([this, ElementHandle](const UClass* NewClass)
							{
								OnSetClassForHandle(NewClass, ElementHandle);
							})
				];
		},
		TAttribute<EVisibility>::CreateSP(this, &FFlowDataPinValueCustomization_Class::GetArrayModeVisibility));
}

void FFlowDataPinValueCustomization_Class::BindDelegates()
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

void FFlowDataPinValueCustomization_Class::TrySetClassFilterFromMetaData() const
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
	ValidateArrayElements(ValuesHandle,
		[this](TSharedPtr<IPropertyHandle> Elem)
		{
			return IsElementValid(Elem);
		},
		[](TSharedPtr<IPropertyHandle> Elem)
		{
			if (Elem.IsValid())
			{
				Elem->SetValueFromFormattedString(TEXT("None"));
			}
		});
}

bool FFlowDataPinValueCustomization_Class::IsElementValid(TSharedPtr<IPropertyHandle> ElementHandle) const
{
	if (!ElementHandle.IsValid())
	{
		return true;
	}

	UClass* FilterClass = CachedEffectiveFilter.Get();
	if (!FilterClass)
	{
		return true;
	}

	FString Path;
	if (!GetElementPathString(ElementHandle, Path) || IsNoneString(Path))
	{
		return true;
	}

	FSoftClassPath SCP(Path);
	if (UClass* Loaded = SCP.TryLoadClass<UObject>())
	{
		return Loaded->IsChildOf(FilterClass);
	}
	return false;
}

const UClass* FFlowDataPinValueCustomization_Class::GetSelectedClassForHandle(TSharedPtr<IPropertyHandle> ElementHandle)
{
	if (!ElementHandle.IsValid())
	{
		return nullptr;
	}

	FString Path;
	if (ElementHandle->GetValueAsFormattedString(Path) != FPropertyAccess::Success || IsNoneString(Path))
	{
		return nullptr;
	}
	return FEditorClassUtils::GetClassFromString(Path);
}

void FFlowDataPinValueCustomization_Class::OnSetClassForHandle(const UClass* NewClass, TSharedPtr<IPropertyHandle> ElementHandle) const
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

	FString Current;
	ElementHandle->GetValueAsFormattedString(Current);
	const FString NewValue = NewClass ? NewClass->GetPathName() : TEXT("None");
	if (Current == NewValue)
	{
		return;
	}

	FScopedTransaction Tx(LOCTEXT("SetClassArrayElement", "Set Class Value"));
	ElementHandle->SetValueFromFormattedString(NewValue);
}

bool FFlowDataPinValueCustomization_Class::GetElementPathString(const TSharedPtr<IPropertyHandle>& ElementHandle, FString& OutPath)
{
	if (!ElementHandle.IsValid())
	{
		return false;
	}
	return ElementHandle->GetValueAsFormattedString(OutPath) == FPropertyAccess::Success;
}

bool FFlowDataPinValueCustomization_Class::IsNoneString(const FString& Str)
{
	return Str.IsEmpty() || Str.Equals(TEXT("None"), ESearchCase::IgnoreCase);
}

#undef LOCTEXT_NAMESPACE