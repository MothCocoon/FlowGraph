// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowDataPinValueCustomization_Enum.h"
#include "Interfaces/FlowDataPinValueOwnerInterface.h"
#include "Types/FlowDataPinValuesStandard.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"

#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FlowDataPinValueCustomization_Enum"

FFlowDataPinValue_Enum* FFlowDataPinValueCustomization_Enum::GetEnumValueStruct() const
{
	return IFlowExtendedPropertyTypeCustomization::TryGetTypedStructValue<FFlowDataPinValue_Enum>(StructPropertyHandle);
}

bool FFlowDataPinValueCustomization_Enum::ShouldShowSourceRow() const
{
	return OwnerInterface ? OwnerInterface->ShowFlowDataPinValueClassFilter(GetEnumValueStruct()) : true;
}

bool FFlowDataPinValueCustomization_Enum::IsSourceEditable() const
{
	return OwnerInterface ? OwnerInterface->CanEditFlowDataPinValueClassFilter(GetEnumValueStruct()) : true;
}

void FFlowDataPinValueCustomization_Enum::BuildValueRows(
	TSharedRef<IPropertyHandle> InStructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	CacheHandles(InStructPropertyHandle, StructCustomizationUtils);
	CacheArraySupported(); // base
	CacheEnumHandles(InStructPropertyHandle);

	if (!bMultiTypeDelegateBound && MultiTypeHandle.IsValid())
	{
		MultiTypeHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FFlowDataPinValueCustomization_Enum::OnMultiTypeChanged));
		bMultiTypeDelegateBound = true;
	}

	const bool bShowSource = ShouldShowSourceRow();
	const bool bSourceEditable = IsSourceEditable();

	if (bShowSource && EnumClassHandle.IsValid())
	{
		IDetailPropertyRow& RowEnumClass = StructBuilder.AddProperty(EnumClassHandle.ToSharedRef());
		RowEnumClass.IsEnabled(bSourceEditable);
		RowEnumClass.ToolTip(GetEnumSourceTooltip());
		EnumClassHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FFlowDataPinValueCustomization_Enum::OnEnumSourceChanged));
	}

	if (bShowSource && EnumNameHandle.IsValid())
	{
		IDetailPropertyRow& RowEnumClassName = StructBuilder.AddProperty(EnumNameHandle.ToSharedRef());
		RowEnumClassName.IsEnabled(bSourceEditable);
		RowEnumClassName.ToolTip(LOCTEXT("EnumNameTooltip", "Name of native C++ enum type to derive EnumClass."));
		EnumNameHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FFlowDataPinValueCustomization_Enum::OnEnumSourceChanged));
	}

	RebuildEnumData();
	EnsureSingleElementExists();
	BuildSingle(StructBuilder);
	if (bArraySupported)
	{
		BuildArray(StructBuilder);
	}
}

void FFlowDataPinValueCustomization_Enum::CacheEnumHandles(const TSharedRef<IPropertyHandle>& StructHandle)
{
	EnumClassHandle = StructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinValue_Enum, EnumClass));
	EnumNameHandle = StructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinValue_Enum, EnumName));
}

void FFlowDataPinValueCustomization_Enum::OnEnumSourceChanged()
{
	RebuildEnumData();

	if (CustomizationUtils)
	{
		if (auto Utils = CustomizationUtils->GetPropertyUtilities())
		{
			Utils->RequestRefresh();
		}
	}
}

void FFlowDataPinValueCustomization_Enum::RebuildEnumData()
{
	EnumeratorOptions.Reset();
	bEnumResolved = false;

	if (FFlowDataPinValue_Enum* EnumStruct = GetEnumValueStruct())
	{
		EnumStruct->OnEnumNameChanged();
	}

	if (UEnum* EnumObj = ResolveEnum())
	{
		CollectEnumerators(*EnumObj);
		bEnumResolved = EnumeratorOptions.Num() > 0;
	}

	ValidateStoredValues();
}

UEnum* FFlowDataPinValueCustomization_Enum::ResolveEnum() const
{
	const FFlowDataPinValue_Enum* Data = GetEnumValueStruct();
	return Data ? Data->EnumClass.LoadSynchronous() : nullptr;
}

void FFlowDataPinValueCustomization_Enum::CollectEnumerators(UEnum& EnumObj)
{
	const int32 Max = EnumObj.GetMaxEnumValue();
	static const TCHAR* HiddenKey = TEXT("Hidden");

	for (int32 Index = 0; Index < Max; ++Index)
	{
		if (!EnumObj.IsValidEnumValue(Index))
		{
			continue;
		}
		if (EnumObj.HasMetaData(HiddenKey, Index))
		{
			continue;
		}

		const FText Display = EnumObj.GetDisplayNameTextByIndex(Index);
		EnumeratorOptions.Add(MakeShared<FName>(*Display.ToString()));
	}
}

void FFlowDataPinValueCustomization_Enum::ValidateStoredValues()
{
	if (!ValuesHandle.IsValid())
	{
		return;
	}

	TArray<FName> ValidNames;
	for (auto& Opt : EnumeratorOptions)
	{
		if (Opt.IsValid())
		{
			ValidNames.Add(*Opt);
		}
	}

	if (auto AsArray = ValuesHandle->AsArray())
	{
		uint32 Count = 0;
		AsArray->GetNumElements(Count);

		if (GetSingleModeVisibility() == EVisibility::Visible && Count == 0)
		{
			AsArray->AddItem();
			AsArray->GetNumElements(Count);
		}

		for (uint32 i = 0; i < Count; ++i)
		{
			auto Elem = ValuesHandle->GetChildHandle(i);
			if (!Elem.IsValid())
			{
				continue;
			}

			FName Current;
			if (Elem->GetValue(Current) == FPropertyAccess::Success)
			{
				if (!IsValueValid(Current))
				{
					Elem->SetValue(ValidNames.Num() > 0 ? ValidNames[0] : FName(NAME_None));
				}
			}
		}
	}
}

bool FFlowDataPinValueCustomization_Enum::IsValueValid(const FName& Candidate) const
{
	if (Candidate.IsNone())
	{
		return EnumeratorOptions.Num() == 0;
	}
	for (auto& Opt : EnumeratorOptions)
	{
		if (Opt.IsValid() && *Opt == Candidate)
		{
			return true;
		}
	}
	return false;
}

TSharedPtr<FName> FFlowDataPinValueCustomization_Enum::FindEnumeratorMatch(const FName& Current) const
{
	for (auto& Opt : EnumeratorOptions)
	{
		if (Opt.IsValid() && *Opt == Current)
		{
			return Opt;
		}
	}
	return nullptr;
}

void FFlowDataPinValueCustomization_Enum::BuildSingle(IDetailChildrenBuilder& StructBuilder)
{
	if (!ValuesHandle.IsValid())
	{
		return;
	}

	auto First = ValuesHandle->GetChildHandle(0);
	if (!First.IsValid())
	{
		if (auto AsArray = ValuesHandle->AsArray())
		{
			AsArray->AddItem();
			First = ValuesHandle->GetChildHandle(0);
		}
	}

	if (!First.IsValid())
	{
		return;
	}

	StructBuilder.AddCustomRow(LOCTEXT("EnumSingleSearch", "Value"))
		.Visibility(TAttribute<EVisibility>::CreateSP(this, &FFlowDataPinValueCustomization_Enum::GetSingleModeVisibility))
		.NameContent()
		[
			SNew(STextBlock)
				.Text(LOCTEXT("EnumValueLabel", "Value"))
				.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		.MinDesiredWidth(200.f)
		[
			SNew(SComboBox<TSharedPtr<FName>>)
				.OptionsSource(&EnumeratorOptions)
				.OnGenerateWidget(this, &FFlowDataPinValueCustomization_Enum::GenerateEnumeratorWidget)
				.OnSelectionChanged_Static(&FFlowDataPinValueCustomization_Enum::OnSingleValueChanged, First)
				.IsEnabled(this, &FFlowDataPinValueCustomization_Enum::IsValueEditingEnabled)
				.InitiallySelectedItem([this, First]()
					{
						FName Current;
						if (First->GetValue(Current) == FPropertyAccess::Success)
						{
							return FindEnumeratorMatch(Current);
						}
						return EnumeratorOptions.Num() > 0 ? EnumeratorOptions[0] : nullptr;
					}())
				.Content()
				[
					SNew(STextBlock)
						.Text_Lambda([this, First]()
							{
								FName Current;
								if (First->GetValue(Current) == FPropertyAccess::Success && !Current.IsNone())
								{
									return GetEnumeratorDisplayText(Current);
								}
								return LOCTEXT("EnumNonePlaceholder", "<None>");
							})
						.Font(IDetailLayoutBuilder::GetDetailFont())
						.ToolTipText(GetEnumSourceTooltip())
				]
		];
}

void FFlowDataPinValueCustomization_Enum::BuildArray(IDetailChildrenBuilder& StructBuilder)
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
						.Text(FText::AsNumber(Index))
						.Font(IDetailLayoutBuilder::GetDetailFont())
				]
				.ValueContent()
				.MinDesiredWidth(200.f)
				[
					SNew(SComboBox<TSharedPtr<FName>>)
						.OptionsSource(&EnumeratorOptions)
						.OnGenerateWidget(this, &FFlowDataPinValueCustomization_Enum::GenerateEnumeratorWidget)
						.OnSelectionChanged_Static(&FFlowDataPinValueCustomization_Enum::OnArrayElementChanged, TSharedPtr<IPropertyHandle>(ElementHandle))
						.IsEnabled(this, &FFlowDataPinValueCustomization_Enum::IsValueEditingEnabled)
						.InitiallySelectedItem([this, ElementHandle]()
							{
								FName Current;
								if (ElementHandle->GetValue(Current) == FPropertyAccess::Success)
								{
									return FindEnumeratorMatch(Current);
								}
								return EnumeratorOptions.Num() > 0 ? EnumeratorOptions[0] : nullptr;
							}())
						.Content()
						[
							SNew(STextBlock)
								.Text_Lambda([this, ElementHandle]()
									{
										FName Current;
										if (ElementHandle->GetValue(Current) == FPropertyAccess::Success && !Current.IsNone())
										{
											return GetEnumeratorDisplayText(Current);
										}
										return LOCTEXT("EnumNonePlaceholder", "<None>");
									})
								.Font(IDetailLayoutBuilder::GetDetailFont())
								.ToolTipText(GetEnumSourceTooltip())
						]
				];
		},
		TAttribute<EVisibility>::CreateSP(this, &FFlowDataPinValueCustomization_Enum::GetArrayModeVisibility));
}

TSharedRef<SWidget> FFlowDataPinValueCustomization_Enum::GenerateEnumeratorWidget(TSharedPtr<FName> Item) const
{
	const FName Name = Item.IsValid() ? *Item : NAME_None;
	return SNew(STextBlock)
		.Text(GetEnumeratorDisplayText(Name))
		.Font(IDetailLayoutBuilder::GetDetailFont());
}

FText FFlowDataPinValueCustomization_Enum::GetEnumeratorDisplayText(const FName& Value)
{
	return Value.IsNone() ? LOCTEXT("EnumNoneDisplay", "<None>") : FText::FromName(Value);
}

FText FFlowDataPinValueCustomization_Enum::GetEnumSourceTooltip() const
{
	const FFlowDataPinValue_Enum* Data = GetEnumValueStruct();
	if (!Data)
	{
		return LOCTEXT("EnumTooltipMissing", "Enum value struct not available.");
	}

	FString Source;
	if (!Data->EnumName.IsEmpty())
	{
		Source = FString::Printf(TEXT("Native Enum: %s"), *Data->EnumName);
	}
	if (Source.IsEmpty() && Data->EnumClass.IsValid())
	{
		Source = FString::Printf(TEXT("Enum Asset: %s"), *Data->EnumClass.ToString());
	}
	if (Source.IsEmpty())
	{
		Source = TEXT("No enum source selected");
	}
	return FText::FromString(Source);
}

void FFlowDataPinValueCustomization_Enum::OnSingleValueChanged(TSharedPtr<FName> NewSelection, ESelectInfo::Type SelectInfo, TSharedPtr<IPropertyHandle> ElementHandle)
{
	if (!ElementHandle.IsValid() || !NewSelection.IsValid())
	{
		return;
	}

	FName Current;
	ElementHandle->GetValue(Current);
	if (Current == *NewSelection)
	{
		return;
	}

	FScopedTransaction Tx(LOCTEXT("SetEnumSingleValue", "Set Enum Value"));
	ElementHandle->SetValue(*NewSelection);
}

void FFlowDataPinValueCustomization_Enum::OnArrayElementChanged(TSharedPtr<FName> NewSelection,	ESelectInfo::Type SelectInfo,TSharedPtr<IPropertyHandle> ElementHandle)
{
	if (!ElementHandle.IsValid() || !NewSelection.IsValid())
	{
		return;
	}

	FName Current;
	ElementHandle->GetValue(Current);
	if (Current == *NewSelection)
	{
		return;
	}

	FScopedTransaction Tx(LOCTEXT("SetEnumArrayElement", "Set Enum Array Element"));
	ElementHandle->SetValue(*NewSelection);
}

void FFlowDataPinValueCustomization_Enum::OnMultiTypeChanged()
{
	// If array not supported, ignore switching
	if (!bArraySupported)
	{
		return;
	}

	if (GetArrayModeVisibility() == EVisibility::Collapsed)
	{
		EnsureSingleElementExists();
	}

	if (CustomizationUtils)
	{
		if (auto Utils = CustomizationUtils->GetPropertyUtilities())
		{
			Utils->RequestRefresh();
		}
	}
}

#undef LOCTEXT_NAMESPACE
