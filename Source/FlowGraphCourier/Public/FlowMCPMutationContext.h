// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "CoreTypes.h"
#include "ScopedTransaction.h"
#include "Templates/Function.h"
#include "Templates/UniquePtr.h"
#include "UObject/ObjectMacros.h"

#include "FlowMCPMutationContext.generated.h"

class UBlueprint;
class UObject;

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPMutationOptions
{
	GENERATED_BODY()

	/**
	 * Compute and report the change without applying it. Requires bUseTransaction true - a dry
	 * run needs its own owned transaction to guarantee rollback, and cannot run inside an ambient
	 * transaction the caller already started (Begin fails in that case rather than mutating anyway).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	bool bDryRun = false;

	/** Save modified packages after a successful mutation. False leaves them dirty in memory. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	bool bSave = true;

	/** Must be true for bDryRun to take effect; see bDryRun's comment for both preconditions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	bool bUseTransaction = true;

	/** Label shown for an owned editor transaction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString TransactionDescription = TEXT("Flow MCP mutation");

	/** Changelist identifier to check out into and move a newly-added file to. Empty (the
	 * default) uses the active/default changelist. A supplied changelist that does not exist,
	 * or that source control cannot place a file into, fails the mutation rather than falling
	 * back to the default changelist silently. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString TargetChangelist;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPMutationReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bDryRun = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bSaved = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bRolledBack = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bTransactionOwned = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bTransactionCommitted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> CheckedOutPackages;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> ModifiedPackages;
};

/**
 * Shared lifecycle policy for typed Flow MCP mutations.
 *
 * The context owns a transaction only when it starts one. Existing ambient
 * transactions remain the caller's responsibility. Dry runs require an
 * owned transaction so the context can guarantee rollback without undoing
 * unrelated caller work.
 */
class FLOWGRAPHCOURIER_API FFlowMCPMutationContext
{
public:
	explicit FFlowMCPMutationContext(const FFlowMCPMutationOptions& InOptions);

	bool Begin(FString& OutError);
	bool PrepareAsset(UObject* Asset, FString& OutError);
	void Modify(UObject* Object);
	bool FinalizeAsset(
		UObject* Asset,
		UBlueprint* Blueprint,
		bool bMarkBlueprintModified,
		FString& OutError);
	bool Complete(FFlowMCPMutationReport& OutReport, FString& OutError);
	void Abort();
	/** End an owned transaction that has no recorded object changes. */
	void CancelUnmodified();
	void RegisterDryRunRollback(TFunction<void()>&& InRollback);

	bool IsDryRun() const
	{
		return Options.bDryRun;
	}

private:
	FFlowMCPMutationOptions Options;
	bool bOwnsTransaction = false;
	bool bBegun = false;
	TUniquePtr<FScopedTransaction> Transaction;
	TFunction<void()> DryRunRollback;
	TArray<FString> CheckedOutPackages;
	TArray<FString> ModifiedPackages;
};
