// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowAssetParamsPtrCustomization.h"
#include "Asset/FlowAssetParams.h"
#include "FlowAsset.h"
#include "FlowComponent.h"
#include "FlowEditorLogChannels.h"
#include "Interfaces/FlowAssetProviderInterface.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IContentBrowserSingleton.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "FlowAssetParamsPtrCustomization"

TSharedRef<IPropertyTypeCustomization> FFlowAssetParamsPtrCustomization::MakeInstance()
{
	return MakeShared<FFlowAssetParamsPtrCustomization>();
}

void FFlowAssetParamsPtrCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> PropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructPropertyHandle = PropertyHandle;

	const TSharedRef<SWidget> ObjectPicker = SNew(SObjectPropertyEntryBox)
		.PropertyHandle(PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowAssetParamsPtr, AssetPtr)))
		.AllowedClass(UFlowAssetParams::StaticClass())
		.AllowClear(true)
		.DisplayUseSelected(false)
		.DisplayBrowse(false)
		.DisplayCompactSize(true)
		.OnShouldFilterAsset(this, &FFlowAssetParamsPtrCustomization::ShouldFilterAsset);

	// Show create button if ShowCreateNew metadata is specified
	const bool bShowCreateButton = PropertyHandle->HasMetaData(TEXT("ShowCreateNew"));

	HeaderRow
		.NameContent()[PropertyHandle->CreatePropertyNameWidget()]
		.ValueContent()
		.MinDesiredWidth(200.f)
		.MaxDesiredWidth(800.f)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					ObjectPicker
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
				.VAlign(VAlign_Center)
				[
					bShowCreateButton ?
						PropertyCustomizationHelpers::MakeAddButton(
							FSimpleDelegate::CreateSP(this, &FFlowAssetParamsPtrCustomization::HandleCreateNew),
							LOCTEXT("CreateNewAsset", "Create New")
						) :
						SNullWidget::NullWidget
				]
		];
}

void FFlowAssetParamsPtrCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> PropertyHandle,
	IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

void FFlowAssetParamsPtrCustomization::HandleCreateNew()
{
	if (!StructPropertyHandle.IsValid())
	{
		UE_LOG(LogFlowEditor, Error, TEXT("Invalid property handle for FFlowAssetParamsPtr customization"));

		return;
	}

	TArray<UObject*> OuterObjects;
	StructPropertyHandle->GetOuterObjects(OuterObjects);
	if (OuterObjects.Num() == 0)
	{
		UE_LOG(LogFlowEditor, Error, TEXT("No outer objects found for BaseAssetParams"));

		return;
	}

	const FName PropertyName = StructPropertyHandle->GetProperty()->GetFName();
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TArray<UObject*> AssetsToSync;

	for (UObject* OuterObject : OuterObjects)
	{
		UFlowAsset* FlowAsset = CastChecked<UFlowAsset>(OuterObject, ECastCheckedType::NullAllowed);
		if (!IsValid(FlowAsset))
		{
			UE_LOG(LogFlowEditor, Error, TEXT("Outer object is not a valid UFlowAsset: %s"), *OuterObject->GetPathName());

			continue;
		}

		if (PropertyName != GET_MEMBER_NAME_CHECKED(UFlowAsset, BaseAssetParams))
		{
			UE_LOG(LogFlowEditor, Error, TEXT("Property %s is not BaseAssetParams for %s"), *PropertyName.ToString(), *FlowAsset->GetPathName());

			continue;
		}

		UFlowAssetParams* NewParams = FlowAsset->GenerateParamsFromStartNode();
		if (IsValid(NewParams))
		{
			StructPropertyHandle->NotifyPreChange();
			StructPropertyHandle->SetValueFromFormattedString(NewParams->GetPathName());
			StructPropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);

			AssetRegistryModule.Get().AssetCreated(NewParams);
			AssetsToSync.Add(NewParams);
		}
	}

	if (!AssetsToSync.IsEmpty())
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSync, true);
	}
}

bool FFlowAssetParamsPtrCustomization::ShouldFilterAsset(const FAssetData& AssetData) const
{
	UFlowAssetParams* Params = Cast<UFlowAssetParams>(AssetData.GetAsset());
	if (!Params)
	{
		// Filter out invalid assets
		return true;
	}

	// Ensure Params->OwnerFlowAsset is valid
	if (Params->OwnerFlowAsset.IsNull())
	{
		UE_LOG(LogFlowEditor, Warning, TEXT("OwnerFlowAsset is null for %s"), *AssetData.GetFullName());

		// Filter out if OwnerFlowAsset is invalid
		return true;
	}

	// Check if child params are allowed
	const bool bHideChildParams = StructPropertyHandle->HasMetaData(TEXT("HideChildParams"));
	if (bHideChildParams && !Params->ParentParams.AssetPtr.IsNull())
	{
		// Filter out params with non-null ParentParams unless allowed
		return true;
	}

	TArray<UObject*> OuterObjects;
	StructPropertyHandle->GetOuterObjects(OuterObjects);
	if (OuterObjects.IsEmpty())
	{
		UE_LOG(LogFlowEditor, Warning, TEXT("No outer objects found for FFlowAssetParamsPtr customization"));

		// Filter out if no outer objects
		return true; 
	}

	// All OwnerAssets must match Params->OwnerFlowAsset
	for (UObject* OuterObject : OuterObjects)
	{
		UFlowAsset* OwnerAssetCur = Cast<UFlowAsset>(OuterObject);
		if (!OwnerAssetCur)
		{
			if (IFlowAssetProviderInterface* AssetProvider = Cast<IFlowAssetProviderInterface>(OuterObject))
			{
				OwnerAssetCur = AssetProvider->ProvideFlowAsset();
			}
		}

		if (!IsValid(OwnerAssetCur) || Params->OwnerFlowAsset != OwnerAssetCur)
		{
			// Filter out if any OwnerAsset doesn't match
			return true; 
		}
	}

	// Allow the asset
	return false;
}

#undef LOCTEXT_NAMESPACE