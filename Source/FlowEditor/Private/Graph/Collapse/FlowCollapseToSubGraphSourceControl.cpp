// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowCollapseToSubGraphSourceControl.h"

#include "FlowAsset.h"

#include "FileHelpers.h"
#include "ISourceControlChangelist.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "ObjectTools.h"
#include "SourceControlHelpers.h"
#include "SourceControlOperations.h"
#include "UObject/Package.h"

#define LOCTEXT_NAMESPACE "FlowCollapseToSubGraph"

namespace FlowCollapseToSubGraph
{
	bool DiscardCreatedAsset(UFlowAsset* NewAsset)
	{
		if (!IsValid(NewAsset))
		{
			return true;
		}

		const TArray<UObject*> AssetsToDelete = { NewAsset };
		constexpr bool bShowConfirmation = false;
		return ObjectTools::ForceDeleteObjects(AssetsToDelete, bShowConfirmation) == AssetsToDelete.Num();
	}

	FText PrepareSourceControl(UFlowAsset& SourceAsset, const FString& NewAssetName, FSourceControlContext& OutContext)
	{
		OutContext = FSourceControlContext();

		ISourceControlModule& SourceControlModule = ISourceControlModule::Get();
		if (!SourceControlModule.IsEnabled())
		{
			return FText::GetEmpty();
		}

		ISourceControlProvider& SourceControlProvider = SourceControlModule.GetProvider();
		if (!SourceControlProvider.IsEnabled())
		{
			return FText::GetEmpty();
		}

		UPackage* SourceAssetPackage = SourceAsset.GetPackage();
		if (!SourceAssetPackage)
		{
			return LOCTEXT("NoSourcePackage", "The source asset has no package to check out.");
		}

		const FString SourceAssetFile = USourceControlHelpers::PackageFilename(SourceAssetPackage);
		const FSourceControlStatePtr SourceState = SourceControlProvider.GetState(SourceAssetFile, EStateCacheUsage::ForceUpdate);
		if (!SourceState.IsValid())
		{
			return LOCTEXT("SourceStateFailed", "Revision control could not determine the source asset's state.");
		}

		OutContext.bEnabled = true;
		if (SourceState->CanCheckIn())
		{
			OutContext.Changelist = SourceState->GetCheckInIdentifier();
			if (!OutContext.Changelist.IsValid())
			{
				return LOCTEXT("ExistingChangelistMissing", "Revision control did not report the source asset's existing changelist.");
			}

			OutContext.ChangelistIdentifier = OutContext.Changelist->GetIdentifier();
			return FText::GetEmpty();
		}

		const TSharedRef<FNewChangelist> NewChangelistOperation = ISourceControlOperation::Create<FNewChangelist>();
		NewChangelistOperation->SetDescription(FText::Format(
			LOCTEXT("ChangelistDescription", "Create sub-graph {0} from a selection in {1}"),
			FText::FromString(NewAssetName),
			FText::FromString(SourceAsset.GetName())));

		if (SourceControlProvider.Execute(NewChangelistOperation, EConcurrency::Synchronous) != ECommandResult::Succeeded)
		{
			return LOCTEXT("NewChangelistFailed", "A changelist could not be created for the sub-graph assets.");
		}

		OutContext.Changelist = NewChangelistOperation->GetNewChangelist();
		if (!OutContext.Changelist.IsValid())
		{
			return LOCTEXT("NoChangelistReturned", "Revision control created no changelist for the sub-graph assets.");
		}

		OutContext.ChangelistIdentifier = OutContext.Changelist->GetIdentifier();

		const TArray<FString> SourceAssetFiles = { SourceAssetFile };
		const bool bCheckedOut = SourceControlProvider.Execute(
			ISourceControlOperation::Create<FCheckOut>(),
			OutContext.Changelist,
			SourceAssetFiles,
			EConcurrency::Synchronous) == ECommandResult::Succeeded;
		const bool bMovedToChangelist = bCheckedOut && SourceControlProvider.Execute(
			ISourceControlOperation::Create<FMoveToChangelist>(),
			OutContext.Changelist,
			SourceAssetFiles,
			EConcurrency::Synchronous) == ECommandResult::Succeeded;
		if (!bMovedToChangelist)
		{
			SourceControlProvider.Execute(
				ISourceControlOperation::Create<FDeleteChangelist>(),
				OutContext.Changelist,
				EConcurrency::Synchronous);
			return FText::Format(
				LOCTEXT("SourceCheckoutFailed", "The source asset could not be checked out into changelist {0}."),
				FText::FromString(OutContext.ChangelistIdentifier));
		}

		return FText::GetEmpty();
	}

	FText SaveAndFinalizeSourceControl(
		UFlowAsset& NewAsset,
		UFlowAsset& SourceAsset,
		const FSourceControlContext& SourceControlContext)
	{
		UPackage* NewAssetPackage = NewAsset.GetPackage();
		UPackage* SourceAssetPackage = SourceAsset.GetPackage();
		if (!NewAssetPackage || !SourceAssetPackage)
		{
			return LOCTEXT("NoPackages", "The collapsed assets have no packages to save.");
		}

		constexpr bool bOnlyDirty = false;
		if (!UEditorLoadingAndSavingUtils::SavePackages({NewAssetPackage}, bOnlyDirty))
		{
			return LOCTEXT("SaveNewAssetFailed", "The generated sub-graph could not be saved; the source asset was not saved.");
		}
		if (!UEditorLoadingAndSavingUtils::SavePackages({SourceAssetPackage}, bOnlyDirty))
		{
			return LOCTEXT("SaveSourceAssetFailed", "The sub-graph was saved, but the source asset could not be saved.");
		}

		if (!SourceControlContext.bEnabled)
		{
			return FText::GetEmpty();
		}
		if (!SourceControlContext.Changelist.IsValid())
		{
			return LOCTEXT("InvalidTargetChangelist", "The sub-graph assets have no valid target changelist.");
		}

		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		const TArray<FString> AssetFiles = {
			USourceControlHelpers::PackageFilename(NewAssetPackage),
			USourceControlHelpers::PackageFilename(SourceAssetPackage)};
		if (SourceControlProvider.Execute(
			ISourceControlOperation::Create<FMoveToChangelist>(),
			SourceControlContext.Changelist,
			AssetFiles,
			EConcurrency::Synchronous) != ECommandResult::Succeeded)
		{
			return FText::Format(
				LOCTEXT("MoveAssetsFailed", "The assets were saved but could not be moved to changelist {0}."),
				FText::FromString(SourceControlContext.ChangelistIdentifier));
		}

		TArray<FSourceControlStateRef> AssetStates;
		if (SourceControlProvider.GetState(AssetFiles, AssetStates, EStateCacheUsage::ForceUpdate) != ECommandResult::Succeeded
			|| AssetStates.Num() != AssetFiles.Num())
		{
			return LOCTEXT("VerifyChangelistFailed", "The assets were saved, but their changelist could not be verified.");
		}

		for (const FSourceControlStateRef& AssetState : AssetStates)
		{
			const FSourceControlChangelistPtr AssetChangelist = AssetState->GetCheckInIdentifier();
			if (!AssetState->CanCheckIn()
				|| !AssetChangelist.IsValid()
				|| AssetChangelist->GetIdentifier() != SourceControlContext.ChangelistIdentifier)
			{
				return FText::Format(
					LOCTEXT("WrongChangelist", "The assets were saved but are not both in changelist {0}."),
					FText::FromString(SourceControlContext.ChangelistIdentifier));
			}
		}

		return FText::GetEmpty();
	}
}

#undef LOCTEXT_NAMESPACE
