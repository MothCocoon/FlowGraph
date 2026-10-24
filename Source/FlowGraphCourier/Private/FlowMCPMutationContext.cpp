// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowMCPMutationContext.h"

#include "Editor.h"
#include "Editor/Transactor.h"
#include "Engine/Blueprint.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "ISourceControlModule.h"
#include "ISourceControlOperation.h"
#include "ISourceControlProvider.h"
#include "ISourceControlState.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "SourceControlHelpers.h"
#include "SourceControlOperations.h"

namespace
{
	FString GetPackageFilename(const UObject* Asset)
	{
		if (!Asset || !Asset->GetOutermost())
		{
			return FString();
		}

		const FString PackageName = Asset->GetOutermost()->GetName();
		if (!FPackageName::IsValidLongPackageName(PackageName))
		{
			return FString();
		}

		return FPaths::ConvertRelativePathToFull(
			FPackageName::LongPackageNameToFilename(
				PackageName,
				FPackageName::GetAssetPackageExtension()));
	}

	bool IsAlreadyReadyForEdit(const FString& PackageFilename)
	{
		if (PackageFilename.IsEmpty())
		{
			return true;
		}

		ISourceControlModule& SourceControl = ISourceControlModule::Get();
		if (!SourceControl.IsEnabled())
		{
			return true;
		}

		ISourceControlProvider& Provider = SourceControl.GetProvider();
		if (!Provider.IsAvailable())
		{
			return false;
		}

		const FSourceControlStatePtr State =
			Provider.GetState(PackageFilename, EStateCacheUsage::ForceUpdate);
		return State.IsValid() && (State->IsCheckedOut() || State->IsAdded());
	}

	// Moves an already checked-out or added file into TargetChangelist. No-op (returns true)
	// if TargetChangelist is empty, so every call site can call this unconditionally. Fails
	// rather than leaving the file in the default changelist: a caller who asked for a specific
	// changelist and silently got the default would have no way to notice.
	//
	// Uses the active Unreal source-control provider; a target changelist is optional.
	bool MoveToTargetChangelist(
		const FString& PackageFilename, const FString& TargetChangelist, FString& OutError)
	{
		if (TargetChangelist.IsEmpty() || PackageFilename.IsEmpty())
		{
			return true;
		}

		ISourceControlModule& SourceControl = ISourceControlModule::Get();
		if (!SourceControl.IsEnabled())
		{
			OutError = FString::Printf(
				TEXT("Target changelist '%s' requested, but source control is not enabled."),
				*TargetChangelist);
			return false;
		}

		ISourceControlProvider& Provider = SourceControl.GetProvider();
		if (!Provider.IsAvailable())
		{
			OutError = FString::Printf(
				TEXT("Target changelist '%s' requested, but the source control provider is not available."),
				*TargetChangelist);
			return false;
		}

		// Refresh the provider's changelist cache so a changelist created earlier this session
		// is visible; GetChangelists alone reads a cache that may predate it.
		const TSharedRef<FUpdatePendingChangelistsStatus> UpdateOperation =
			ISourceControlOperation::Create<FUpdatePendingChangelistsStatus>();
		UpdateOperation->SetUpdateAllChangelists(true);
		Provider.Execute(UpdateOperation, EConcurrency::Synchronous);

		FSourceControlChangelistPtr ResolvedChangelist;
		for (const FSourceControlChangelistRef& Changelist :
			Provider.GetChangelists(EStateCacheUsage::Use))
		{
			if (Changelist->GetIdentifier() == TargetChangelist)
			{
				ResolvedChangelist = Changelist;
				break;
			}
		}

		if (!ResolvedChangelist.IsValid())
		{
			OutError = FString::Printf(
				TEXT("Target changelist '%s' does not exist or is not visible to source control."),
				*TargetChangelist);
			return false;
		}

		const ECommandResult::Type Result = Provider.Execute(
			ISourceControlOperation::Create<FMoveToChangelist>(),
			ResolvedChangelist,
			TArray<FString>{PackageFilename},
			EConcurrency::Synchronous);
		if (Result != ECommandResult::Succeeded)
		{
			OutError = FString::Printf(
				TEXT("Failed to move '%s' to changelist '%s'."),
				*PackageFilename, *TargetChangelist);
			return false;
		}

		return true;
	}
}

FFlowMCPMutationContext::FFlowMCPMutationContext(const FFlowMCPMutationOptions& InOptions)
	: Options(InOptions)
{
}

bool FFlowMCPMutationContext::Begin(FString& OutError)
{
	if (bBegun)
	{
		OutError = TEXT("Mutation context has already begun.");
		return false;
	}

	if (Options.bDryRun && !Options.bUseTransaction)
	{
		OutError = TEXT("Dry runs require bUseTransaction=true so the mutation can be rolled back.");
		return false;
	}

	const bool bHasAmbientTransaction =
		GEditor != nullptr &&
		GEditor->Trans != nullptr &&
		GEditor->Trans->IsActive();
	if (bHasAmbientTransaction)
	{
		if (Options.bDryRun)
		{
			OutError = TEXT("Dry runs cannot run inside an existing transaction.");
			return false;
		}
	}
	else if (Options.bUseTransaction)
	{
		const FString Description = Options.TransactionDescription.IsEmpty()
			? TEXT("Flow MCP mutation")
			: Options.TransactionDescription;
		Transaction = MakeUnique<FScopedTransaction>(FText::FromString(Description));
		if (!Transaction->IsOutstanding())
		{
			OutError = TEXT("Failed to begin the editor transaction.");
			Transaction.Reset();
			return false;
		}
		bOwnsTransaction = true;
	}

	bBegun = true;
	return true;
}

bool FFlowMCPMutationContext::PrepareAsset(UObject* Asset, FString& OutError)
{
	if (!bBegun)
	{
		OutError = TEXT("Mutation context has not begun.");
		return false;
	}

	if (!Asset)
	{
		OutError = TEXT("Mutation target is null.");
		return false;
	}

	if (Options.bDryRun || !Options.bSave)
	{
		return true;
	}

	const FString PackageFilename = GetPackageFilename(Asset);
	if (!PackageFilename.IsEmpty() &&
		!IFileManager::Get().FileExists(*PackageFilename))
	{
		// New assets have no package file until their first save. Deferring
		// source-control add lets unsaved mutations remain in-memory only.
		return true;
	}

	const bool bWasReadyForEdit = IsAlreadyReadyForEdit(PackageFilename);
	if (!PackageFilename.IsEmpty() &&
		!USourceControlHelpers::CheckOutOrAddFile(PackageFilename, true))
	{
		OutError = FString::Printf(
			TEXT("Source control checkout failed for '%s'."),
			*PackageFilename);
		return false;
	}

	if (!PackageFilename.IsEmpty() &&
		!MoveToTargetChangelist(PackageFilename, Options.TargetChangelist, OutError))
	{
		return false;
	}

	if (!bWasReadyForEdit && !PackageFilename.IsEmpty())
	{
		CheckedOutPackages.AddUnique(PackageFilename);
	}

	return true;
}

void FFlowMCPMutationContext::Modify(UObject* Object)
{
	if (!Object)
	{
		return;
	}

	if (bOwnsTransaction || (GEditor != nullptr && GEditor->Trans != nullptr && GEditor->Trans->IsActive()))
	{
		// Modify() only records to the transaction buffer for transactional objects, so the
		// flag has to be set on every path - otherwise a rollback reverts nothing.
		Object->SetFlags(RF_Transactional);
		Object->Modify();
	}
}

bool FFlowMCPMutationContext::FinalizeAsset(
	UObject* Asset,
	UBlueprint* Blueprint,
	bool bMarkBlueprintModified,
	FString& OutError)
{
	if (!Asset)
	{
		OutError = TEXT("Cannot finalize a null asset.");
		return false;
	}

	if (bMarkBlueprintModified && Blueprint)
	{
		// CDO property writes are deliberately non-structural. Structural
		// recompilation regenerates the CDO and discards direct default writes.
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}

	if (Asset->GetOutermost())
	{
		ModifiedPackages.AddUnique(Asset->GetOutermost()->GetName());
	}

	if (Options.bDryRun)
	{
		Asset->MarkPackageDirty();
		return true;
	}

	if (Options.bSave)
	{
		TArray<UPackage*> PackagesToSave;
		PackagesToSave.Add(Asset->GetOutermost());
		if (!UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, true))
		{
			OutError = FString::Printf(
				TEXT("Save failed for '%s'."),
				*Asset->GetPathName());
			return false;
		}

		const FString PackageFilename = GetPackageFilename(Asset);
		if (!PackageFilename.IsEmpty())
		{
			// SavePackages above may itself have already added a brand-new file to source
			// control (editor auto-add-on-save), so IsAlreadyReadyForEdit can be true here even
			// though nothing has moved it to TargetChangelist yet. Gate the add on it, but call
			// MoveToTargetChangelist unconditionally afterward - same split PrepareAsset uses -
			// or a caller-requested changelist silently loses to whatever the save auto-added to.
			if (!IsAlreadyReadyForEdit(PackageFilename) &&
				!USourceControlHelpers::CheckOutOrAddFile(PackageFilename, true))
			{
				OutError = FString::Printf(
					TEXT("Source control add failed for '%s' after save."),
					*PackageFilename);
				return false;
			}

			if (!MoveToTargetChangelist(PackageFilename, Options.TargetChangelist, OutError))
			{
				return false;
			}
		}
	}
	else
	{
		Asset->MarkPackageDirty();
	}

	return true;
}

bool FFlowMCPMutationContext::Complete(FFlowMCPMutationReport& OutReport, FString& OutError)
{
	if (!bBegun)
	{
		OutError = TEXT("Mutation context has not begun.");
		return false;
	}

	OutReport = FFlowMCPMutationReport();
	OutReport.bDryRun = Options.bDryRun;
	OutReport.bTransactionOwned = bOwnsTransaction;
	OutReport.CheckedOutPackages = CheckedOutPackages;
	OutReport.ModifiedPackages = ModifiedPackages;

	if (Options.bDryRun)
	{
		if (!bOwnsTransaction || !Transaction.IsValid() || !Transaction->IsOutstanding())
		{
			OutError = TEXT("Dry run did not own an active transaction and cannot guarantee rollback.");
			return false;
		}

		Transaction.Reset();
		if (GEditor && GEditor->Trans)
		{
			const bool bWasSuspended = GEditor->bSuspendBroadcastPostUndoRedo;
			GEditor->bSuspendBroadcastPostUndoRedo = true;
			GEditor->Trans->Undo(false);
			GEditor->bSuspendBroadcastPostUndoRedo = bWasSuspended;
		}
		if (DryRunRollback)
		{
			DryRunRollback();
			DryRunRollback = nullptr;
		}
		OutReport.bRolledBack = true;
		OutReport.bTransactionCommitted = false;
		OutReport.bSaved = false;
		bBegun = false;
		return true;
	}

	if (bOwnsTransaction)
	{
		Transaction.Reset();
	}

	OutReport.bTransactionCommitted = bOwnsTransaction;
	OutReport.bSaved = Options.bSave;

	bBegun = false;
	return true;
}

void FFlowMCPMutationContext::Abort()
{
	if (!bBegun)
	{
		return;
	}

	if (bOwnsTransaction && Transaction.IsValid())
	{
		// Every abort must discard the partial mutation, not just dry runs: the
		// FScopedTransaction destructor commits unless the change is rolled back.
		Transaction.Reset();
		if (GEditor && GEditor->Trans)
		{
			const bool bWasSuspended = GEditor->bSuspendBroadcastPostUndoRedo;
			GEditor->bSuspendBroadcastPostUndoRedo = true;
			GEditor->Trans->Undo(false);
			GEditor->bSuspendBroadcastPostUndoRedo = bWasSuspended;
		}
	}

	if (DryRunRollback)
	{
		DryRunRollback();
		DryRunRollback = nullptr;
	}

	bBegun = false;
}

void FFlowMCPMutationContext::CancelUnmodified()
{
	if (!bBegun)
	{
		return;
	}

	if (bOwnsTransaction && Transaction.IsValid())
	{
		Transaction->Cancel();
		Transaction.Reset();
	}
	bBegun = false;
}

void FFlowMCPMutationContext::RegisterDryRunRollback(TFunction<void()>&& InRollback)
{
	if (Options.bDryRun)
	{
		DryRunRollback = MoveTemp(InRollback);
	}
}
