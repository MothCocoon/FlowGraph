// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowAssetParamsUtils.h"
#include "Types/FlowNamedDataPinProperty.h"

#include "Misc/DateTime.h"
#include "HAL/FileManager.h"

#if WITH_EDITOR
#include "Asset/FlowAssetParams.h"
#include "FlowLogChannels.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "FileHelpers.h"
#include "IContentBrowserSingleton.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "SourceControlHelpers.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowAssetParamsUtils)

#if WITH_EDITOR
FDateTime FFlowAssetParamsUtils::GetLastSavedTimestampForObject(const UObject* Object)
{
	if (!Object)
	{
		return FDateTime::MinValue();
	}

	const FString PackagePath = Object->GetPathName();
	return IFileManager::Get().GetTimeStamp(*PackagePath);
}

EFlowReconcilePropertiesResult FFlowAssetParamsUtils::CheckPropertiesMatch(
	const TArray<FFlowNamedDataPinProperty>& PropertiesA,
	const TArray<FFlowNamedDataPinProperty>& PropertiesB)
{
	if (PropertiesA.Num() != PropertiesB.Num())
	{
		return EFlowReconcilePropertiesResult::Error_PropertyCountMismatch;
	}

	for (int32 Index = 0; Index < PropertiesA.Num(); ++Index)
	{
		const FFlowNamedDataPinProperty& PropA = PropertiesA[Index];
		const FFlowNamedDataPinProperty& PropB = PropertiesB[Index];
		const UScriptStruct* ScriptStructA = PropA.DataPinValue.GetScriptStruct();
		const UScriptStruct* ScriptStructB = PropB.DataPinValue.GetScriptStruct();

		if (PropA.Name != PropB.Name ||
			ScriptStructA != ScriptStructB ||
			!IsValid(ScriptStructA))
		{
			return EFlowReconcilePropertiesResult::Error_PropertyTypeMismatch;
		}
	}

	return EFlowReconcilePropertiesResult::NoChanges;
}

const FFlowNamedDataPinProperty* FFlowAssetParamsUtils::FindPropertyByGuid(
	const TArray<FFlowNamedDataPinProperty>& Props,
	const FGuid& Guid)
{
	for (const FFlowNamedDataPinProperty& Prop : Props)
	{
		if (Prop.Guid == Guid)
		{
			return &Prop;
		}
	}

	return nullptr;
}

FFlowNamedDataPinProperty* FFlowAssetParamsUtils::FindPropertyByGuid(
	TArray<FFlowNamedDataPinProperty>& Props,
	const FGuid& Guid)
{
	for (FFlowNamedDataPinProperty& Prop : Props)
	{
		if (Prop.Guid == Guid)
		{
			return &Prop;
		}
	}

	return nullptr;
}

bool FFlowAssetParamsUtils::ArePropertyArraysEqual(
	const TArray<FFlowNamedDataPinProperty>& A,
	const TArray<FFlowNamedDataPinProperty>& B)
{
	if (A.Num() != B.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < A.Num(); ++Index)
	{
		if (!ArePropertiesEqual(A[Index], B[Index]))
		{
			return false;
		}
	}

	return true;
}

bool FFlowAssetParamsUtils::ArePropertiesEqual(
	const FFlowNamedDataPinProperty& A,
	const FFlowNamedDataPinProperty& B)
{
	if (A.Name != B.Name || A.Guid != B.Guid)
	{
		return false;
	}

	const UScriptStruct* ScriptStructA = A.DataPinValue.GetScriptStruct();
	const UScriptStruct* ScriptStructB = B.DataPinValue.GetScriptStruct();
	if (ScriptStructA != ScriptStructB)
	{
		return false;
	}

	return A.DataPinValue == B.DataPinValue;
}

UFlowAssetParams* FFlowAssetParamsUtils::CreateChildParamsAsset(UFlowAssetParams& ParentParams,	const bool bShowDialogs, FText* OutOptionalFailureReason)
{
	if (!IsValid(&ParentParams))
	{
		FailCreateChild(
			NSLOCTEXT("FlowAssetParamsUtils", "InvalidParent", "Invalid Parent Flow Asset Params."),
			bShowDialogs,
			OutOptionalFailureReason);

		return nullptr;
	}

	const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

	const FString PackagePath = FPackageName::GetLongPackagePath(ParentParams.GetPackage()->GetPathName());
	const FString BaseAssetName = ParentParams.GetName();

	// Generate a unique name for the new asset params
	FString UniquePackageName;
	FString UniqueAssetName;
	AssetToolsModule.Get().CreateUniqueAssetName(PackagePath + TEXT("/") + BaseAssetName, TEXT(""), UniquePackageName, UniqueAssetName);

	if (UniqueAssetName.IsEmpty())
	{
		FailCreateChild(
			FText::Format(
				NSLOCTEXT("FlowAssetParamsUtils", "UniqueNameFail", "Failed to generate unique asset name for child params of {0}."),
				FText::FromString(BaseAssetName)),
			bShowDialogs,
			OutOptionalFailureReason);

		return nullptr;
	}

	// Create the new asset params
	UFlowAssetParams* NewParams = Cast<UFlowAssetParams>(
		AssetToolsModule.Get().CreateAsset(UniqueAssetName, PackagePath, ParentParams.GetClass(), nullptr));

	if (!IsValid(NewParams))
	{
		FailCreateChild(
			FText::Format(
				NSLOCTEXT("FlowAssetParamsUtils", "CreateAssetFail", "Failed to create child Flow Asset Params: {0}."),
				FText::FromString(UniqueAssetName)),
			bShowDialogs,
			OutOptionalFailureReason);

		return nullptr;
	}

	// Best-effort source control integration (before save)
	if (USourceControlHelpers::IsAvailable())
	{
		const FString FileName = USourceControlHelpers::PackageFilename(NewParams->GetPathName());
		if (!USourceControlHelpers::CheckOutOrAddFile(FileName))
		{
			UE_LOG(LogFlow, Warning, TEXT("Failed to check out/add %s; saved in-memory only"), *NewParams->GetPathName());
		}
	}

	// Configure from parent (copies OwnerFlowAsset + Properties, sets ParentParams, rebuilds PropertyMap, marks dirty)
	NewParams->ConfigureFlowAssetParams(ParentParams.OwnerFlowAsset, &ParentParams, ParentParams.Properties);

	// Reconcile (cycle detection, flattened inheritance, etc.)
	const EFlowReconcilePropertiesResult ReconcileResult = NewParams->ReconcilePropertiesWithParentParams();
	if (EFlowReconcilePropertiesResult_Classifiers::IsErrorResult(ReconcileResult))
	{
		FailCreateChild(
			FText::Format(
				NSLOCTEXT("FlowAssetParamsUtils", "ReconcileFail",
					"Created asset but reconciliation failed.\n\nAsset: {0}\nError: {1}\n\nThe asset may be invalid and should be reviewed."),
				FText::FromString(NewParams->GetPathName()),
				UEnum::GetDisplayValueAsText(ReconcileResult)),
			bShowDialogs,
			OutOptionalFailureReason);

		// Keep going: asset exists and may still be useful for debugging/fixing
	}

	// Save the package (force save even if not prompted)
	{
		UPackage* Package = NewParams->GetPackage();
		const TArray<UPackage*> PackagesToSave = { Package };

		constexpr bool bForceSave = true;
		if (!UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, bForceSave))
		{
			FailCreateChild(
				FText::Format(
					NSLOCTEXT("FlowAssetParamsUtils", "SaveFail", "Failed to save child Flow Asset Params: {0}."),
					FText::FromString(NewParams->GetPathName())),
				bShowDialogs,
				OutOptionalFailureReason);

			// Still return the in-memory asset
		}
	}

	// Register + sync to Content Browser
	{
		const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().AssetCreated(NewParams);

		const FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		const TArray<UObject*> AssetsToSync = { NewParams };
		ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSync, true);
	}

	return NewParams;
}

void FFlowAssetParamsUtils::FailCreateChild(const FText& Reason, const bool bShowDialogs, FText* OutOptionalFailureReason)
{
	if (OutOptionalFailureReason)
	{
		*OutOptionalFailureReason = Reason;
	}

	UE_LOG(LogFlow, Error, TEXT("%s"), *Reason.ToString());

	if (bShowDialogs)
	{
		FMessageDialog::Open(EAppMsgType::Ok, Reason);
	}
}

#endif