// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowDataPinValueCustomization.h"
#include "Interfaces/FlowDataPinValueOwnerInterface.h"
#include "Types/FlowDataPinValuesStandard.h"
#include "UnrealExtensions/VisibilityArrayBuilder.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "IPropertyUtilities.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "UObject/EnumProperty.h"

#define LOCTEXT_NAMESPACE "FlowDataPinValueCustomization"

static const TCHAR HiddenMeta[] = TEXT("Hidden");

FText FFlowDataPinValueCustomization::GetMultiTypeTooltip()
{
	return LOCTEXT("MultiTypeTooltip",
		"Select whether this Data Pin holds a Single value or an Array of values.\n"
		"Changing from Array to Single trims the array to the first element.");
}
FText FFlowDataPinValueCustomization::GetInputPinTooltip()
{
	return LOCTEXT("InputPinTooltip",
		"Marks this Data Pin as an Input.\nChecked = Input Pin, Unchecked = Output Pin.");
}

TSharedRef<IPropertyTypeCustomization> FFlowDataPinValueCustomization::MakeInstance()
{
	return MakeShareable(new FFlowDataPinValueCustomization());
}

void FFlowDataPinValueCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	Super::CustomizeHeader(InStructPropertyHandle, HeaderRow, StructCustomizationUtils);

	CacheHandles(InStructPropertyHandle, StructCustomizationUtils);
	CacheOwnerInterface();
	CacheArraySupported();

	// Populate MultiTypeOptions from enum (respect bArraySupported)
	MultiTypeOptions.Reset();
	if (const UEnum* MultiTypeEnum = StaticEnum<EFlowDataMultiType>())
	{
		const int32 NumEnums = FMath::Min(static_cast<int32>(FlowEnum::MaxOf<EFlowDataMultiType>()), MultiTypeEnum->NumEnums());
		for (int32 i = 0; i < NumEnums; ++i)
		{
			if (MultiTypeEnum->HasMetaData(HiddenMeta, i))
			{
				continue;
			}
			const int64 Value = MultiTypeEnum->GetValueByIndex(i);
			EFlowDataMultiType MT = static_cast<EFlowDataMultiType>(Value);
			if (!bArraySupported && MT == EFlowDataMultiType::Array)
			{
				continue;
			}
			MultiTypeOptions.Add(MakeShareable(new int32(static_cast<int32>(Value))));
		}
	}

	// If current mode is Array but unsupported, force Single (non-transactable)
	if (!bArraySupported && MultiTypeHandle.IsValid())
	{
		uint8 CurrentValue = 0;
		if (MultiTypeHandle->GetValue(CurrentValue) == FPropertyAccess::Success &&
			static_cast<EFlowDataMultiType>(CurrentValue) == EFlowDataMultiType::Array)
		{
			MultiTypeHandle->SetValue(static_cast<uint8>(EFlowDataMultiType::Single),
				EPropertyValueSetFlags::NotTransactable);
		}

		if (MultiTypeComboBox.IsValid())
		{
			MultiTypeComboBox->SetEnabled(false);
		}
	}

	// Select current
	const EFlowDataMultiType CurrentType = GetCurrentMultiType();
	for (auto& Opt : MultiTypeOptions)
	{
		if (Opt.IsValid() && static_cast<EFlowDataMultiType>(*Opt) == CurrentType)
		{
			SelectedMultiType = Opt;
			break;
		}
	}

	TSharedRef<SHorizontalBox> HeaderBox = SNew(SHorizontalBox);

	// MultiType control (combo or static label if array unsupported)
	if (bArraySupported)
	{
		HeaderBox->AddSlot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SAssignNew(MultiTypeComboBox, SComboBox<TSharedPtr<int32>>)
					.OptionsSource(&MultiTypeOptions)
					.OnGenerateWidget(this, &FFlowDataPinValueCustomization::GenerateMultiTypeWidget)
					.OnSelectionChanged(this, &FFlowDataPinValueCustomization::OnMultiTypeChanged)
					.IsEnabled(this, &FFlowDataPinValueCustomization::GetInputPinCheckboxEnabled)
					.ToolTipText(GetMultiTypeTooltip())
					.Content()
					[
						SNew(STextBlock)
							.Text(this, &FFlowDataPinValueCustomization::GetSelectedMultiTypeText)
							.Font(IDetailLayoutBuilder::GetDetailFont())
					]
			];
	}
	else
	{
		HeaderBox->AddSlot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
					.Text(LOCTEXT("MultiTypeForcedSingle", "Single"))
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.ToolTipText(LOCTEXT("MultiTypeForcedSingleTooltip", "This pin type does not support Array mode."))
			];
	}

	// Input Pin checkbox
	HeaderBox->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(4.f, 0.f)
		[
			SNew(SCheckBox)
				.IsChecked(this, &FFlowDataPinValueCustomization::GetCurrentIsInputPin)
				.OnCheckStateChanged(this, &FFlowDataPinValueCustomization::OnInputPinChanged)
				.IsEnabled(this, &FFlowDataPinValueCustomization::GetInputPinCheckboxEnabled)
				.Visibility(this, &FFlowDataPinValueCustomization::GetInputPinCheckboxVisibility)
				.ToolTipText(GetInputPinTooltip())
				[
					SNew(STextBlock)
						.Text(LOCTEXT("InputPin", "Input Pin"))
						.Font(IDetailLayoutBuilder::GetDetailFont())
				]
		];

	HeaderRow
		.NameContent()
		[
			SNew(STextBlock)
				.Text(StructPropertyHandle->GetPropertyDisplayName())
				.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		.MinDesiredWidth(250.f)
		[
			HeaderBox
		];
}

void FFlowDataPinValueCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	BuildValueRows(InStructPropertyHandle, StructBuilder, StructCustomizationUtils);
}

void FFlowDataPinValueCustomization::BuildValueRows(TSharedRef<IPropertyHandle> InStructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	CacheHandles(InStructPropertyHandle, StructCustomizationUtils);
	CacheArraySupported();

	if (!ValuesHandle.IsValid())
	{
		return;
	}

	if (bArraySupported)
	{
		EnsureSingleElementExists();
	}

	BuildSingleBranch(StructBuilder);
	if (bArraySupported)
	{
		BuildArrayBranch(StructBuilder);
	}
}

void FFlowDataPinValueCustomization::BuildSingleBranch(IDetailChildrenBuilder& StructBuilder)
{
	if (GetSingleModeVisibility() == EVisibility::Collapsed)
	{
		return;
	}

	if (!ValuesHandle.IsValid())
	{
		return;
	}

	TSharedPtr<IPropertyHandle> ValueToShow = bArraySupported
		? ValuesHandle->GetChildHandle(0)
		: ValuesHandle;

	if (!ValueToShow.IsValid())
	{
		return;
	}

	IDetailPropertyRow& Row = StructBuilder.AddProperty(ValueToShow.ToSharedRef());
	Row.ShouldAutoExpand(true);
}

void FFlowDataPinValueCustomization::BuildArrayBranch(IDetailChildrenBuilder& StructBuilder)
{
	if (GetArrayModeVisibility() == EVisibility::Collapsed)
	{
		return;
	}

	if (bArraySupported && ValuesHandle.IsValid() && ValuesHandle->AsArray())
	{
		IDetailPropertyRow& Row = StructBuilder.AddProperty(ValuesHandle.ToSharedRef());
		Row.ShouldAutoExpand(true);
	}
}

void FFlowDataPinValueCustomization::RequestRefresh()
{
	if (PropertyUtilities.IsValid())
	{
		PropertyUtilities->RequestRefresh();
	}
}

void FFlowDataPinValueCustomization::EnsureSingleElementExists()
{
	if (!ValuesHandle.IsValid())
	{
		return;
	}

	if (bArraySupported)
	{
		if (auto AsArray = ValuesHandle->AsArray())
		{
			uint32 Num = 0;
			AsArray->GetNumElements(Num);
			if (Num == 0)
			{
				AsArray->AddItem();
			}
		}
	}
}

void FFlowDataPinValueCustomization::CacheHandles(const TSharedRef<IPropertyHandle>& PropertyHandle,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	CustomizationUtils = &StructCustomizationUtils;
	MultiTypeHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinValue, MultiType));
	IsInputPinHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinValue, bIsInputPin));
	PropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();

	if (auto* Value = GetFlowDataPinValueBeingEdited())
	{
		DataPinType = Value->LookupPinType();
		if (DataPinType)
		{
			ValuesHandle = DataPinType->GetValuesHandle(PropertyHandle);
		}
	}
}

void FFlowDataPinValueCustomization::CacheOwnerInterface()
{
	OwnerInterface = nullptr;
	TArray<UObject*> Outers;
	StructPropertyHandle->GetOuterObjects(Outers);

	if (Outers.Num() == 1)
	{
		OwnerInterface = Cast<IFlowDataPinValueOwnerInterface>(Outers[0]);
	}
}

void FFlowDataPinValueCustomization::CacheArraySupported()
{
	bArraySupported = DataPinType ? DataPinType->SupportsMultiType(EFlowDataMultiType::Array) : true;
}

void FFlowDataPinValueCustomization::OnMultiTypeChanged(TSharedPtr<int32> NewSelection, ESelectInfo::Type)
{
	if (!NewSelection.IsValid() || !MultiTypeHandle.IsValid())
	{
		return;
	}

	if (!bArraySupported)
	{
		return;
	}

	const EFlowDataMultiType NewType = static_cast<EFlowDataMultiType>(*NewSelection);

	bool bNeedsTrim = (NewType == EFlowDataMultiType::Single);
	if (bNeedsTrim && ValuesHandle.IsValid())
	{
		if (auto AsArray = ValuesHandle->AsArray())
		{
			uint32 NumElements = 0;
			AsArray->GetNumElements(NumElements);
			bNeedsTrim = NumElements > 1;
		}
	}

	FScopedTransaction Transaction(LOCTEXT("ChangePinMultiType", "Change Pin MultiType"));
	MultiTypeHandle->NotifyPreChange();
	MultiTypeHandle->SetValue(static_cast<uint8>(NewType));
	if (bNeedsTrim)
	{
		TrimArrayToSingle();
	}
	MultiTypeHandle->NotifyPostChange(EPropertyChangeType::ValueSet);

	SelectedMultiType = NewSelection;

	// Preferred: trigger owner rebuild
#if WITH_EDITOR
	if (OwnerInterface)
	{
		OwnerInterface->RequestFlowDataPinValuesDetailsRebuild();
	}
	else
	{
		RequestRefresh();
	}
#endif
}

void FFlowDataPinValueCustomization::OnInputPinChanged(ECheckBoxState NewState)
{
	if (!IsInputPinHandle.IsValid())
	{
		return;
	}

	bool Existing = false;
	IsInputPinHandle->GetValue(Existing);
	const bool bNewValue = NewState == ECheckBoxState::Checked;

	if (Existing == bNewValue)
	{
		return;
	}

	FScopedTransaction Transaction(LOCTEXT("ChangeInputPin", "Change Input Pin"));
	IsInputPinHandle->NotifyPreChange();
	IsInputPinHandle->SetValue(bNewValue);
	IsInputPinHandle->NotifyPostChange(EPropertyChangeType::ValueSet);

	RequestRefresh();
}

void FFlowDataPinValueCustomization::TrimArrayToSingle()
{
	if (!ValuesHandle.IsValid())
	{
		return;
	}

	if (auto AsArray = ValuesHandle->AsArray())
	{
		uint32 NumElements = 0;
		AsArray->GetNumElements(NumElements);

		if (NumElements == 0)
		{
			AsArray->AddItem();
		}
		else
		{
			while (NumElements > 1)
			{
				AsArray->DeleteItem(NumElements - 1);
				AsArray->GetNumElements(NumElements);
			}
		}

		RequestRefresh();
	}
}

EFlowDataMultiType FFlowDataPinValueCustomization::GetCurrentMultiType() const
{
	if (MultiTypeHandle.IsValid())
	{
		uint8 Value = 0;
		if (MultiTypeHandle->GetValue(Value) == FPropertyAccess::Success)
		{
			return static_cast<EFlowDataMultiType>(Value);
		}
	}
	return EFlowDataMultiType::Single;
}

ECheckBoxState FFlowDataPinValueCustomization::GetCurrentIsInputPin() const
{
	if (IsInputPinHandle.IsValid())
	{
		bool Value = false;
		IsInputPinHandle->GetValue(Value);
		return Value ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	return ECheckBoxState::Unchecked;
}

EVisibility FFlowDataPinValueCustomization::GetSingleModeVisibility() const
{
	return GetCurrentMultiType() == EFlowDataMultiType::Single ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FFlowDataPinValueCustomization::GetArrayModeVisibility() const
{
	if (!bArraySupported)
	{
		return EVisibility::Collapsed;
	}
	return GetCurrentMultiType() == EFlowDataMultiType::Array ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FFlowDataPinValueCustomization::GetInputPinCheckboxVisibility() const
{
	return OwnerInterface && OwnerInterface->ShowFlowDataPinValueInputPinCheckbox()
		? EVisibility::Visible
		: EVisibility::Collapsed;
}

bool FFlowDataPinValueCustomization::GetInputPinCheckboxEnabled() const
{
	return OwnerInterface ? OwnerInterface->CanModifyFlowDataPinType() : true;
}

TSharedRef<SWidget> FFlowDataPinValueCustomization::GenerateMultiTypeWidget(TSharedPtr<int32> Item) const
{
	const UEnum* MultiTypeEnum = StaticEnum<EFlowDataMultiType>();
	return SNew(STextBlock)
		.Text(Item.IsValid() && MultiTypeEnum
			? MultiTypeEnum->GetDisplayNameTextByValue(*Item)
			: FText::GetEmpty())
		.Font(IDetailLayoutBuilder::GetDetailFont());
}

FText FFlowDataPinValueCustomization::GetSelectedMultiTypeText() const
{
	const UEnum* MultiTypeEnum = StaticEnum<EFlowDataMultiType>();
	return (SelectedMultiType.IsValid() && MultiTypeEnum)
		? MultiTypeEnum->GetDisplayNameTextByValue(*SelectedMultiType)
		: FText::GetEmpty();
}

void FFlowDataPinValueCustomization::BuildVisibilityAwareArray(
	IDetailChildrenBuilder& StructBuilder,
	TSharedPtr<IPropertyHandle> ArrayHandle,
	TFunction<void(TSharedRef<IPropertyHandle>, int32, IDetailChildrenBuilder&, const TAttribute<EVisibility>&)> Generator,
	TAttribute<EVisibility> VisibilityAttribute)
{
	if (!ArrayHandle.IsValid() || !bArraySupported)
	{
		return;
	}

	TSharedRef<FVisibilityArrayBuilder> ArrayBuilder =
		MakeShareable(new FVisibilityArrayBuilder(ArrayHandle.ToSharedRef(), true, true, true));

	ArrayBuilder->SetVisibilityGetter([VisibilityAttribute]()
		{
			return VisibilityAttribute.Get();
		});

	ArrayBuilder->OnGenerateArrayElementWidget(
		FOnGenerateArrayElementWidgetVisible::CreateLambda(
			[Generator](TSharedRef<IPropertyHandle> Elem, int32 Index, IDetailChildrenBuilder& Child, const TAttribute<EVisibility>& RowVis)
			{
				Generator(Elem, Index, Child, RowVis);
			}));

	StructBuilder.AddCustomBuilder(ArrayBuilder);
}

void FFlowDataPinValueCustomization::ValidateArrayElements(TSharedPtr<IPropertyHandle> ArrayHandle,
	TFunction<bool(TSharedPtr<IPropertyHandle>)> IsValidPredicate,
	TFunction<void(TSharedPtr<IPropertyHandle>)> InvalidateAction)
{
	if (!ArrayHandle.IsValid())
	{
		return;
	}

	auto AsArray = ArrayHandle->AsArray();
	if (!AsArray.IsValid())
	{
		return;
	}

	uint32 Num = 0;
	AsArray->GetNumElements(Num);

	TArray<TSharedPtr<IPropertyHandle>> ToInvalidate;
	ToInvalidate.Reserve(Num);

	for (uint32 i = 0; i < Num; ++i)
	{
		TSharedPtr<IPropertyHandle> Elem = ArrayHandle->GetChildHandle(i);
		if (!Elem.IsValid())
		{
			continue;
		}
		if (!IsValidPredicate(Elem))
		{
			ToInvalidate.Add(Elem);
		}
	}

	if (ToInvalidate.Num() > 0)
	{
		const FScopedTransaction Tx(LOCTEXT("InvalidateArrayElements", "Clear Invalid Data Pin Values"));
		for (auto& H : ToInvalidate)
		{
			if (H.IsValid())
			{
				InvalidateAction(H);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE