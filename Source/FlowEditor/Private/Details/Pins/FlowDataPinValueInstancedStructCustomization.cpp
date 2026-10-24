// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Details/Pins/FlowDataPinValueInstancedStructCustomization.h"

#include "Nodes/FlowPin.h"
#include "Types/FlowDataPinValue.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "InstancedStructDetails.h"
#include "IPropertyUtilities.h"
#include "ScopedTransaction.h"
#include "StructUtilsMetadata.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Modules/ModuleManager.h"
#include "StructUtils/InstancedStruct.h"
#include "Styling/SlateIconFinder.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FlowDataPinValueInstancedStructCustomization"

bool FFlowDataPinValueInstancedStructIdentifier::IsPropertyTypeCustomized(const IPropertyHandle& PropertyHandle) const
{
	const FString& BaseStructName = PropertyHandle.GetMetaData(UE::StructUtils::Metadata::BaseStructName);
	if (!BaseStructName.IsEmpty())
	{
		const UScriptStruct* BaseStruct = UClass::TryFindTypeSlow<UScriptStruct>(BaseStructName);
		return BaseStruct && BaseStruct->IsChildOf(FFlowDataPinValue::StaticStruct());
	}

	return false;
}

TSharedRef<IPropertyTypeCustomization> FFlowDataPinValueInstancedStructCustomization::MakeInstance()
{
	return MakeShared<FFlowDataPinValueInstancedStructCustomization>();
}

void FFlowDataPinValueInstancedStructCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructPropertyHandle = InStructPropertyHandle;
	PropertyUtilities = CustomizationUtils.GetPropertyUtilities();

	const bool bCanChangeStructType = !InStructPropertyHandle->HasMetaData(UE::StructUtils::Metadata::StructTypeConstName);

	HeaderRow
		.NameContent()
		[
			InStructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(250.f)
		.VAlign(VAlign_Center)
		[
			SNew(SComboButton)
				.OnGetMenuContent(this, &FFlowDataPinValueInstancedStructCustomization::GenerateStructMenu)
				.ContentPadding(0.f)
				.IsEnabled(InStructPropertyHandle->IsEditable() && bCanChangeStructType)
				.ButtonContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0.f, 0.f, 4.f, 0.f)
						[
							SNew(SImage)
								.Image(this, &FFlowDataPinValueInstancedStructCustomization::GetSelectedStructIcon)
						]
					+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
								.Text(this, &FFlowDataPinValueInstancedStructCustomization::GetSelectedStructText)
								.ToolTipText(this, &FFlowDataPinValueInstancedStructCustomization::GetSelectedStructTooltip)
								.Font(IDetailLayoutBuilder::GetDetailFont())
						]
				]
		]
		.IsEnabled(InStructPropertyHandle->IsEditable());
}

void FFlowDataPinValueInstancedStructCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructBuilder.AddCustomBuilder(MakeShared<FFlowDataPinValueInstancedStructDataDetails>(InStructPropertyHandle));
}

FText FFlowDataPinValueInstancedStructCustomization::GetShortDisplayName(const UScriptStruct& Struct)
{
	const FString& PinTypeName = Struct.GetMetaData(FFlowPin::MetadataKey_FlowPinType);
	return PinTypeName.IsEmpty() ? Struct.GetDisplayNameText() : FText::FromString(PinTypeName);
}

const UScriptStruct* FFlowDataPinValueInstancedStructCustomization::GetBaseStruct() const
{
	return UClass::TryFindTypeSlow<UScriptStruct>(StructPropertyHandle->GetMetaData(UE::StructUtils::Metadata::BaseStructName));
}

FPropertyAccess::Result FFlowDataPinValueInstancedStructCustomization::GetCommonStruct(const UScriptStruct*& OutCommonStruct) const
{
	bool bHasResult = false;
	bool bHasMultipleValues = false;

	StructPropertyHandle->EnumerateConstRawData([&OutCommonStruct, &bHasResult, &bHasMultipleValues](const void* RawData, const int32 /*DataIndex*/, const int32 /*NumDatas*/)
	{
		if (const FInstancedStruct* InstancedStruct = static_cast<const FInstancedStruct*>(RawData))
		{
			const UScriptStruct* Struct = InstancedStruct->GetScriptStruct();
			if (!bHasResult)
			{
				OutCommonStruct = Struct;
			}
			else if (OutCommonStruct != Struct)
			{
				bHasMultipleValues = true;
			}

			bHasResult = true;
		}

		return true;
	});

	if (bHasMultipleValues)
	{
		return FPropertyAccess::MultipleValues;
	}

	return bHasResult ? FPropertyAccess::Success : FPropertyAccess::Fail;
}

FText FFlowDataPinValueInstancedStructCustomization::GetSelectedStructText() const
{
	const UScriptStruct* CommonStruct = nullptr;
	const FPropertyAccess::Result Result = GetCommonStruct(CommonStruct);

	if (Result == FPropertyAccess::Success)
	{
		return CommonStruct ? GetShortDisplayName(*CommonStruct) : LOCTEXT("None", "None");
	}
	else if (Result == FPropertyAccess::MultipleValues)
	{
		return LOCTEXT("MultipleValues", "Multiple Values");
	}

	return FText::GetEmpty();
}

FText FFlowDataPinValueInstancedStructCustomization::GetSelectedStructTooltip() const
{
	const UScriptStruct* CommonStruct = nullptr;
	if (GetCommonStruct(CommonStruct) == FPropertyAccess::Success && CommonStruct)
	{
		return CommonStruct->GetToolTipText();
	}

	return GetSelectedStructText();
}

const FSlateBrush* FFlowDataPinValueInstancedStructCustomization::GetSelectedStructIcon() const
{
	const UScriptStruct* CommonStruct = nullptr;
	if (GetCommonStruct(CommonStruct) == FPropertyAccess::Success)
	{
		return FSlateIconFinder::FindIconBrushForClass(CommonStruct, "ClassIcon.Object");
	}

	return nullptr;
}

TSharedRef<SWidget> FFlowDataPinValueInstancedStructCustomization::GenerateStructMenu()
{
	const UScriptStruct* BaseStruct = GetBaseStruct();
	const bool bAllowBaseStruct = !StructPropertyHandle->HasMetaData(TEXT("ExcludeBaseStruct"));
	const bool bAllowNone = !(StructPropertyHandle->GetMetaDataProperty()->PropertyFlags & CPF_NoClear);

	// Same filtering as the engine's instanced struct picker
	TArray<const UScriptStruct*> Structs;
	if (BaseStruct)
	{
		for (TObjectIterator<UScriptStruct> It; It; ++It)
		{
			const UScriptStruct* Struct = *It;
			if (Struct->IsChildOf(BaseStruct) && (bAllowBaseStruct || Struct != BaseStruct) && !Struct->HasMetaData(TEXT("Hidden")))
			{
				Structs.Add(Struct);
			}
		}
	}

	Structs.Sort([](const UScriptStruct& A, const UScriptStruct& B)
	{
		return GetShortDisplayName(A).CompareTo(GetShortDisplayName(B)) < 0;
	});

	FMenuBuilder MenuBuilder(true, nullptr);

	if (bAllowNone)
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("None", "None"),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &FFlowDataPinValueInstancedStructCustomization::OnStructPicked, static_cast<const UScriptStruct*>(nullptr))));
	}

	for (const UScriptStruct* Struct : Structs)
	{
		MenuBuilder.AddMenuEntry(
			GetShortDisplayName(*Struct),
			Struct->GetToolTipText(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &FFlowDataPinValueInstancedStructCustomization::OnStructPicked, Struct)));
	}

	return MenuBuilder.MakeWidget();
}

void FFlowDataPinValueInstancedStructCustomization::OnStructPicked(const UScriptStruct* Struct)
{
	if (StructPropertyHandle.IsValid() && StructPropertyHandle->IsValidHandle())
	{
		FScopedTransaction Transaction(LOCTEXT("SetStruct", "Set Struct"));

		StructPropertyHandle->NotifyPreChange();

		StructPropertyHandle->EnumerateRawData([Struct](void* RawData, const int32 /*DataIndex*/, const int32 /*NumDatas*/)
		{
			if (FInstancedStruct* InstancedStruct = static_cast<FInstancedStruct*>(RawData))
			{
				InstancedStruct->InitializeAs(Struct);
			}

			return true;
		});

		StructPropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
		StructPropertyHandle->NotifyFinishedChangingProperties();

		// Expand, so child properties of the new type are editable right away
		StructPropertyHandle->SetExpanded(true);

		// Property tree is invalid after changing the struct type
		if (PropertyUtilities.IsValid())
		{
			PropertyUtilities->ForceRefresh();
		}
	}
}

FFlowDataPinValueInstancedStructDataDetails::FFlowDataPinValueInstancedStructDataDetails(const TSharedRef<IPropertyHandle>& InStructPropertyHandle)
	: StructPropertyHandle(InStructPropertyHandle)
{
}

void FFlowDataPinValueInstancedStructDataDetails::SetOnRebuildChildren(FSimpleDelegate InOnRegenerateChildren)
{
	OnRegenerateChildren = InOnRegenerateChildren;
}

void FFlowDataPinValueInstancedStructDataDetails::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
	const TSharedRef<FInstancedStructProvider> StructProvider = MakeShared<FInstancedStructProvider>(StructPropertyHandle);

	if (const UScriptStruct* BaseStruct = Cast<UScriptStruct>(StructProvider->GetBaseStructure()))
	{
		const FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		if (PropertyEditorModule.IsCustomizedStruct(BaseStruct, FCustomPropertyTypeLayoutMap()))
		{
			// AddChildStructure applies the struct's IPropertyTypeCustomization, which takes the short name as its row label
			const FName PropertyName = StructPropertyHandle->GetProperty()->GetFName();
			ChildrenBuilder.AddChildStructure(StructPropertyHandle, StructProvider, PropertyName, FFlowDataPinValueInstancedStructCustomization::GetShortDisplayName(*BaseStruct));
		}
		else
		{
			StructPropertyHandle->RemoveChildren();
			for (const TSharedPtr<IPropertyHandle>& ChildHandle : StructPropertyHandle->AddChildStructure(StructProvider))
			{
				ChildrenBuilder.AddProperty(ChildHandle.ToSharedRef());
			}
		}
	}

	CachedInstanceTypes = GetInstanceTypes();
}

void FFlowDataPinValueInstancedStructDataDetails::Tick(float DeltaTime)
{
	// Struct type may change outside of the picker, e.g. on undo or paste
	if (GetInstanceTypes() != CachedInstanceTypes)
	{
		OnRegenerateChildren.ExecuteIfBound();
	}
}

FName FFlowDataPinValueInstancedStructDataDetails::GetName() const
{
	static const FName Name(TEXT("FlowDataPinValueInstancedStructDataDetails"));
	return Name;
}

TArray<TWeakObjectPtr<const UScriptStruct>> FFlowDataPinValueInstancedStructDataDetails::GetInstanceTypes() const
{
	TArray<TWeakObjectPtr<const UScriptStruct>> Result;

	StructPropertyHandle->EnumerateConstRawData([&Result](const void* RawData, const int32 /*DataIndex*/, const int32 /*NumDatas*/)
	{
		const FInstancedStruct* InstancedStruct = static_cast<const FInstancedStruct*>(RawData);
		Result.Add(InstancedStruct ? InstancedStruct->GetScriptStruct() : nullptr);

		return true;
	});

	return Result;
}

#undef LOCTEXT_NAMESPACE
