// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowDataPinValueCustomization_Enum.h"

#include "Types/FlowDataPinValuesStandard.h"
#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Interfaces/FlowDataPinValueOwnerInterface.h"
#include "DetailCustomizations/FlowValueSourcePolicy.h"

#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FlowDataPinValueCustomization_Enum"

void FFlowDataPinValueCustomization_Enum::BuildValueRows(
	TSharedRef<IPropertyHandle> InStructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	CacheHandles(InStructPropertyHandle, StructCustomizationUtils);
	CacheEnumHandles(InStructPropertyHandle);

	if (!bMultiTypeDelegateBound && MultiTypeHandle.IsValid())
	{
		MultiTypeHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FFlowDataPinValueCustomization_Enum::OnMultiTypeChanged));
		bMultiTypeDelegateBound = true;
	}

	LockEnumHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinValue_Enum, bLockEnumClass));
	ComputePolicy();

	// SOURCE rows: only if visible & not locked
	if (SourcePolicy.bShowSourceRow && !SourcePolicy.bLocked && EnumClassHandle.IsValid())
	{
		IDetailPropertyRow& Row = StructBuilder.AddProperty(EnumClassHandle.ToSharedRef());
		Row.IsEnabled(SourcePolicy.bFinalEditableSource);
		Row.ToolTip(GetEnumSourceTooltip());

		EnumClassHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FFlowDataPinValueCustomization_Enum::OnEnumSourceChanged));
	}

#if WITH_EDITORONLY_DATA
	if (SourcePolicy.bShowSourceRow && !SourcePolicy.bLocked && EnumNameHandle.IsValid())
	{
		IDetailPropertyRow& Row = StructBuilder.AddProperty(EnumNameHandle.ToSharedRef());
		Row.IsEnabled(SourcePolicy.bFinalEditableSource);
		Row.ToolTip(LOCTEXT("EnumNameTooltip", "Name of native C++ enum type (overrides asset if provided)."));

		EnumNameHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FFlowDataPinValueCustomization_Enum::OnEnumSourceChanged));
	}
#endif

	RebuildEnumData();
	EnsureSingleElementExists();
	BuildSingle(StructBuilder);
	BuildArray(StructBuilder);
}

void FFlowDataPinValueCustomization_Enum::OnSourceLockToggled()
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

void FFlowDataPinValueCustomization_Enum::ComputePolicy()
{
	FFlowDataPinValue_Enum* EnumStruct = GetEnumValueStruct();

	bool bPerValueLock = false;

#if WITH_EDITORONLY_DATA
	if (EnumStruct)
	{
		bPerValueLock = EnumStruct->bLockEnumClass;
	}
#endif

	// Enum has no MetaClass forcing, pass false
	SourcePolicy = ComputeFlowValueSourcePolicy(
		OwnerInterface,
		reinterpret_cast<const FFlowDataPinValue*>(EnumStruct),
		false,
		bPerValueLock,
		true);
}

void FFlowDataPinValueCustomization_Enum::CacheEnumHandles(const TSharedRef<IPropertyHandle>& StructHandle)
{
	EnumClassHandle = StructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinValue_Enum, EnumClass));

#if WITH_EDITORONLY_DATA
	EnumNameHandle = StructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinValue_Enum, EnumName));
#endif
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
#if WITH_EDITOR
		EnumStruct->OnEnumNameChanged();
#endif
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
	ValidNames.Reserve(EnumeratorOptions.Num());

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

		if (GetSingleVisibility() == EVisibility::Visible && Count == 0)
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

EVisibility FFlowDataPinValueCustomization_Enum::GetSingleVisibility() const
{
	FLOW_ASSERT_ENUM_MAX(EFlowDataMultiType, 2);

	uint8 Mode = 0;

	if (MultiTypeHandle.IsValid() &&
		MultiTypeHandle->GetValue(Mode) == FPropertyAccess::Success &&
		(EFlowDataMultiType)Mode == EFlowDataMultiType::Single)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

EVisibility FFlowDataPinValueCustomization_Enum::GetArrayVisibility() const
{
	FLOW_ASSERT_ENUM_MAX(EFlowDataMultiType, 2);

	uint8 Mode = 0;

	if (MultiTypeHandle.IsValid() &&
		MultiTypeHandle->GetValue(Mode) == FPropertyAccess::Success &&
		(EFlowDataMultiType)Mode == EFlowDataMultiType::Array)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
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
		.Visibility(TAttribute<EVisibility>::Create(
			TAttribute<EVisibility>::FGetter::CreateSP(this, &FFlowDataPinValueCustomization_Enum::GetSingleVisibility)))
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
				.OnSelectionChanged(this, &FFlowDataPinValueCustomization_Enum::OnSingleValueChanged, First)
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
	if (!ValuesHandle.IsValid())
	{
		return;
	}

	TSharedRef<FVisibilityArrayBuilder> ArrayBuilder =
		MakeShareable(new FVisibilityArrayBuilder(ValuesHandle.ToSharedRef(), true, true, true));

	ArrayBuilder->SetVisibilityGetter([this]()
		{
			return GetArrayVisibility();
		});

	ArrayBuilder->OnGenerateArrayElementWidget(
		FOnGenerateArrayElementWidgetVisible::CreateSP(
			this,
			&FFlowDataPinValueCustomization_Enum::GenerateArrayElementVisible));

	StructBuilder.AddCustomBuilder(ArrayBuilder);
}

void FFlowDataPinValueCustomization_Enum::GenerateArrayElementVisible(
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
				.Text(FText::AsNumber(Index))
				.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		.MinDesiredWidth(200.f)
		[
			SNew(SComboBox<TSharedPtr<FName>>)
				.OptionsSource(&EnumeratorOptions)
				.OnGenerateWidget(this, &FFlowDataPinValueCustomization_Enum::GenerateEnumeratorWidget)
				.OnSelectionChanged(this,
					&FFlowDataPinValueCustomization_Enum::OnArrayElementChanged,
					TSharedPtr<IPropertyHandle>(ElementHandle))
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
}

TSharedRef<SWidget> FFlowDataPinValueCustomization_Enum::GenerateEnumeratorWidget(TSharedPtr<FName> Item) const
{
	const FName Name = Item.IsValid() ? *Item : NAME_None;

	return SNew(STextBlock)
		.Text(GetEnumeratorDisplayText(Name))
		.Font(IDetailLayoutBuilder::GetDetailFont());
}

FText FFlowDataPinValueCustomization_Enum::GetEnumeratorDisplayText(const FName& Value) const
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

#if WITH_EDITORONLY_DATA
	if (!Data->EnumName.IsEmpty())
	{
		Source = FString::Printf(TEXT("Native Enum: %s"), *Data->EnumName);
	}
#endif

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

void FFlowDataPinValueCustomization_Enum::OnSingleValueChanged(
	TSharedPtr<FName> NewSelection,
	ESelectInfo::Type,
	TSharedPtr<IPropertyHandle> ElementHandle)
{
	if (!ElementHandle.IsValid() || !NewSelection.IsValid())
	{
		return;
	}

	FScopedTransaction Tx(LOCTEXT("SetEnumSingleValue", "Set Enum Value"));
	ElementHandle->SetValue(*NewSelection);
}

void FFlowDataPinValueCustomization_Enum::OnArrayElementChanged(
	TSharedPtr<FName> NewSelection,
	ESelectInfo::Type,
	TSharedPtr<IPropertyHandle> ElementHandle)
{
	if (!ElementHandle.IsValid() || !NewSelection.IsValid())
	{
		return;
	}

	FScopedTransaction Tx(LOCTEXT("SetEnumArrayElement", "Set Enum Array Element"));
	ElementHandle->SetValue(*NewSelection);
}

void FFlowDataPinValueCustomization_Enum::OnMultiTypeChanged()
{
	if (GetArrayVisibility() == EVisibility::Collapsed)
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

FFlowDataPinValue_Enum* FFlowDataPinValueCustomization_Enum::GetEnumValueStruct() const
{
	return IFlowExtendedPropertyTypeCustomization::TryGetTypedStructValue<FFlowDataPinValue_Enum>(StructPropertyHandle);
}

#undef LOCTEXT_NAMESPACE