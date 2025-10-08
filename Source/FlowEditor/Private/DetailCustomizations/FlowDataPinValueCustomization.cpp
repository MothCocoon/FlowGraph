// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowDataPinValueCustomization.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "Interfaces/FlowDataPinValueOwnerInterface.h"
#include "IPropertyUtilities.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"
#include "FlowEditorLogChannels.h"
#include "DetailCustomizations/FlowValueSourcePolicy.h"
#include "Types/FlowDataPinValuesStandard.h"

#define LOCTEXT_NAMESPACE "FlowDataPinValueCustomization"

static const FText MultiTypeTooltip = LOCTEXT("MultiTypeTooltip",
	"Select whether this Data Pin holds a Single value or an Array of values.\n"
	"Changing from Array to Single will trim the array to keep only the first element.");

static const FText InputPinTooltip = LOCTEXT("InputPinTooltip",
	"Marks this Data Pin as an Input.\n"
	"When checked, the value is expected to be provided externally (upstream / user).\n"
	"When unchecked, the pin is treated as an Output / internally produced value.");

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

	if (MultiTypeOptions.Num() == 0)
	{
		MultiTypeOptions.Add(MakeShareable(new FString("Single")));
		MultiTypeOptions.Add(MakeShareable(new FString("Array")));
	}

	FLOW_ASSERT_ENUM_MAX(EFlowDataMultiType, 2);
	const EFlowDataMultiType CurrentType = GetCurrentMultiType();
	SelectedMultiType = MultiTypeOptions[CurrentType == EFlowDataMultiType::Single ? 0 : 1];

	TSharedRef<SHorizontalBox> HeaderBox =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SAssignNew(MultiTypeComboBox, SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&MultiTypeOptions)
				.OnGenerateWidget(this, &FFlowDataPinValueCustomization::GenerateMultiTypeWidget)
				.OnSelectionChanged(this, &FFlowDataPinValueCustomization::OnMultiTypeChanged)
				.IsEnabled(this, &FFlowDataPinValueCustomization::GetInputPinCheckboxEnabled)
				.ToolTipText(MultiTypeTooltip)
				.Content()
				[
					SNew(STextBlock)
						.Text(this, &FFlowDataPinValueCustomization::GetSelectedMultiTypeText)
						.Font(IDetailLayoutBuilder::GetDetailFont())
				]
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(4.f, 0.f)
		[
			SNew(SCheckBox)
				.IsChecked(this, &FFlowDataPinValueCustomization::GetCurrentIsInputPin)
				.OnCheckStateChanged(this, &FFlowDataPinValueCustomization::OnInputPinChanged)
				.IsEnabled(this, &FFlowDataPinValueCustomization::GetInputPinCheckboxEnabled)
				.Visibility(this, &FFlowDataPinValueCustomization::GetInputPinCheckboxVisibility)
				.ToolTipText(InputPinTooltip)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("InputPin", "Input Pin"))
						.Font(IDetailLayoutBuilder::GetDetailFont())
				]
		];

	AppendHeaderExtensions(HeaderBox);

	HeaderRow
		.NameContent()
		[
			SNew(STextBlock)
				.Text(StructPropertyHandle->GetPropertyDisplayName())
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(GetRowTint())
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

	if (!ValuesHandle.IsValid())
	{
		return;
	}

	EnsureSingleElementExists();
	BuildSingleBranch(StructBuilder);
	BuildArrayBranch(StructBuilder);
}

void FFlowDataPinValueCustomization::BuildSingleBranch(IDetailChildrenBuilder& StructBuilder)
{
	if (!ValuesHandle.IsValid())
	{
		return;
	}

	auto FirstElementHandle = ValuesHandle->GetChildHandle(0);

	if (!FirstElementHandle.IsValid())
	{
		return;
	}

	IDetailPropertyRow& Row = StructBuilder.AddProperty(FirstElementHandle.ToSharedRef());
	Row.ShouldAutoExpand(true);
	Row.Visibility(TAttribute<EVisibility>::CreateSP(this, &FFlowDataPinValueCustomization::GetSingleModeVisibility));
}

void FFlowDataPinValueCustomization::BuildArrayBranch(IDetailChildrenBuilder& StructBuilder)
{
	if (!ValuesHandle.IsValid())
	{
		return;
	}

	IDetailPropertyRow& Row = StructBuilder.AddProperty(ValuesHandle.ToSharedRef());
	Row.ShouldAutoExpand(true);
	Row.Visibility(TAttribute<EVisibility>::CreateSP(this, &FFlowDataPinValueCustomization::GetArrayModeVisibility));
}

void FFlowDataPinValueCustomization::EnsureSingleElementExists()
{
	if (!ValuesHandle.IsValid())
	{
		return;
	}

	uint32 NumChildren = 0;
	ValuesHandle->GetNumChildren(NumChildren);

	if (NumChildren == 0)
	{
		if (auto AsArray = ValuesHandle->AsArray())
		{
			AsArray->AddItem();
		}
	}
}

void FFlowDataPinValueCustomization::AppendHeaderExtensions(TSharedRef<SHorizontalBox> HeaderBox)
{
	const FFlowValueSourcePolicy* Policy = GetSourcePolicy();

	if (!Policy || !Policy->bShowLockToggle)
	{
		return;
	}

	TSharedPtr<IPropertyHandle> LockHandle = StructPropertyHandle->GetChildHandle(TEXT("bLockClassFilter"));
	bool bEnum = false;
	bool bIsObjectLike = false;
	bool bIsClass = false;

	if (LockHandle.IsValid())
	{
		if (IFlowExtendedPropertyTypeCustomization::TryGetTypedStructValue<FFlowDataPinValue_Class>(StructPropertyHandle))
		{
			bIsClass = true;
		}
		else if (IFlowExtendedPropertyTypeCustomization::TryGetTypedStructValue<FFlowDataPinValue_Object>(StructPropertyHandle) ||
			IFlowExtendedPropertyTypeCustomization::TryGetTypedStructValue<FFlowDataPinValue_InstancedObject>(StructPropertyHandle))
		{
			bIsObjectLike = true;
		}
	}

	if (!LockHandle.IsValid())
	{
		LockHandle = StructPropertyHandle->GetChildHandle(TEXT("bLockEnumClass"));
		bEnum = LockHandle.IsValid();
	}

	if (!LockHandle.IsValid())
	{
		return;
	}

	const bool bMetaForced = Policy->bMetaForced;

	FText LockTooltip;
	FText MetaTooltip;

	if (bEnum)
	{
		LockTooltip = LOCTEXT("EnumLockTooltip",
			"Lock Enum Class & Name.\nPrevents changing the Enum asset or native enum name.\nEnumerator values remain editable.");
		MetaTooltip = LOCTEXT("EnumLockMetaTooltip", "Enum source locked by metadata.");
	}
	else if (bIsClass)
	{
		LockTooltip = LOCTEXT("ClassLockTooltip",
			"Lock Class Filter.\nPrevents changing the Class Filter.\nClass values remain editable.");
		MetaTooltip = LOCTEXT("ClassLockMetaTooltip", "Class filter locked by metadata (MetaClass).");
	}
	else if (bIsObjectLike)
	{
		LockTooltip = LOCTEXT("ObjectLockTooltip",
			"Lock Object Class Filter.\nPrevents changing the Class Filter.\nObject references remain editable.");
		MetaTooltip = LOCTEXT("ObjectLockMetaTooltip", "Object class filter locked by metadata (MetaClass).");
	}
	else
	{
		LockTooltip = LOCTEXT("GenericLockTooltip",
			"Lock source settings (disables changing the source). Values remain editable.");
		MetaTooltip = LOCTEXT("GenericMetaTooltip", "Source locked by metadata.");
	}

	if (bMetaForced)
	{
		HeaderBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4.f, 0.f, 0.f, 0.f)
			[
				SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.Lock"))
					.ToolTipText(MetaTooltip)
			];

		return;
	}

	HeaderBox->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(4.f, 0.f, 0.f, 0.f)
		[
			SNew(SCheckBox)
				.IsChecked_Lambda([LockHandle]()
					{
						bool bLocked = false;
						LockHandle->GetValue(bLocked);
						return bLocked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
					})
				.OnCheckStateChanged_Lambda([this, LockHandle](ECheckBoxState NewState)
					{
						if (!LockHandle.IsValid())
						{
							return;
						}

						const bool bNew = (NewState == ECheckBoxState::Checked);

						FScopedTransaction Tx(LOCTEXT("ToggleSourceLock", "Toggle Source Lock"));
						LockHandle->SetValue(bNew);
						OnSourceLockToggled();
					})
				.ToolTipText(LockTooltip)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("LockShortLabel", "Lock"))
						.Font(IDetailLayoutBuilder::GetDetailFont())
				]
		];

	LockHandle->SetOnPropertyValueChanged(
		FSimpleDelegate::CreateSP(this, &FFlowDataPinValueCustomization::OnSourceLockToggled));
}

void FFlowDataPinValueCustomization::OnSourceLockToggled()
{
	if (CustomizationUtils)
	{
		if (auto Utils = CustomizationUtils->GetPropertyUtilities())
		{
			Utils->RequestRefresh();
		}
	}
}

void FFlowDataPinValueCustomization::CacheHandles(const TSharedRef<IPropertyHandle>& PropertyHandle,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	CustomizationUtils = &StructCustomizationUtils;
	MultiTypeHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinValue, MultiType));
	IsInputPinHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinValue, bIsInputPin));

	if (auto* Value = GetFlowDataPinValueBeingEdited())
	{
		PinType = Value->LookupDataPinType();

		if (PinType)
		{
			ValuesHandle = PinType->GetValuesHandle(PropertyHandle);
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

void FFlowDataPinValueCustomization::OnMultiTypeChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type /*SelectInfo*/)
{
	if (!NewSelection.IsValid() || !MultiTypeHandle.IsValid())
	{
		return;
	}

	FLOW_ASSERT_ENUM_MAX(EFlowDataMultiType, 2);
	static FString ArrayValueAsString = UEnum::GetDisplayValueAsText(EFlowDataMultiType::Array).ToString();

	const EFlowDataMultiType NewType =
		*NewSelection == ArrayValueAsString ? EFlowDataMultiType::Array : EFlowDataMultiType::Single;

	FScopedTransaction Transaction(LOCTEXT("ChangePinMultiType", "Change Pin MultiType"));

	MultiTypeHandle->NotifyPreChange();
	MultiTypeHandle->SetValue(static_cast<uint8>(NewType));

	if (NewType == EFlowDataMultiType::Single)
	{
		TrimArrayToSingle();
	}

	MultiTypeHandle->NotifyPostChange(EPropertyChangeType::ValueSet);

	if (CustomizationUtils)
	{
		if (TSharedPtr<IPropertyUtilities> PropUtils = CustomizationUtils->GetPropertyUtilities())
		{
			PropUtils->RequestRefresh();
		}
	}

	SelectedMultiType = NewSelection;
}

void FFlowDataPinValueCustomization::OnInputPinChanged(ECheckBoxState NewState)
{
	if (!IsInputPinHandle.IsValid())
	{
		return;
	}

	FScopedTransaction Transaction(LOCTEXT("ChangeInputPin", "Change Input Pin"));

	IsInputPinHandle->NotifyPreChange();
	IsInputPinHandle->SetValue(NewState == ECheckBoxState::Checked);
	IsInputPinHandle->NotifyPostChange(EPropertyChangeType::ValueSet);

	if (CustomizationUtils)
	{
		if (TSharedPtr<IPropertyUtilities> PropUtils = CustomizationUtils->GetPropertyUtilities())
		{
			PropUtils->RequestRefresh();
		}
	}
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

		FScopedTransaction Transaction(LOCTEXT("TrimArrayToSingle", "Trim Array to Single"));

		if (NumElements == 0)
		{
			AsArray->AddItem();
		}
		else
		{
			for (uint32 Index = NumElements - 1; Index >= 1; --Index)
			{
				AsArray->DeleteItem(Index);
			}
		}

		if (CustomizationUtils)
		{
			if (TSharedPtr<IPropertyUtilities> PropUtils = CustomizationUtils->GetPropertyUtilities())
			{
				PropUtils->RequestRefresh();
			}
		}
	}
}

EFlowDataMultiType FFlowDataPinValueCustomization::GetCurrentMultiType() const
{
	if (MultiTypeHandle.IsValid())
	{
		uint8 Value = 0;
		MultiTypeHandle->GetValue(Value);
		return static_cast<EFlowDataMultiType>(Value);
	}

	FLOW_ASSERT_ENUM_MAX(EFlowDataMultiType, 2);
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
	FLOW_ASSERT_ENUM_MAX(EFlowDataMultiType, 2);
	return GetCurrentMultiType() == EFlowDataMultiType::Single ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FFlowDataPinValueCustomization::GetArrayModeVisibility() const
{
	FLOW_ASSERT_ENUM_MAX(EFlowDataMultiType, 2);
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
	return OwnerInterface ? OwnerInterface->CanModifyFlowDataPinValueType() : true;
}

FLinearColor FFlowDataPinValueCustomization::GetRowTint() const
{
	return PinType ? PinType->GetPinColor() : FLinearColor::White;
}

TSharedRef<SWidget> FFlowDataPinValueCustomization::GenerateMultiTypeWidget(TSharedPtr<FString> Item) const
{
	return SNew(STextBlock)
		.Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty())
		.Font(IDetailLayoutBuilder::GetDetailFont());
}

FText FFlowDataPinValueCustomization::GetSelectedMultiTypeText() const
{
	return SelectedMultiType.IsValid() ? FText::FromString(*SelectedMultiType) : FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE