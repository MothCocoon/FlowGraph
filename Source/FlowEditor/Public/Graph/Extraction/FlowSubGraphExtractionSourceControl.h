// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Internationalization/Text.h"
#include "Templates/SharedPointer.h"

class ISourceControlChangelist;
class UFlowAsset;

namespace FlowSubGraphExtraction
{
	/** Source-control destination selected before the source asset is saved. */
	struct FSourceControlContext
	{
		/** Unset when the provider has no changelists. */
		TSharedPtr<ISourceControlChangelist, ESPMode::ThreadSafe> Changelist;
		FString ChangelistIdentifier;
		bool bEnabled = false;
	};

	/** Removes a newly created SubGraph asset after a failed extraction. */
	FLOWEDITOR_API bool DiscardCreatedAsset(UFlowAsset* NewAsset);

	/**
	 * Makes the source asset writable and resolves the target changelist before either asset is saved.
	 * The source is checked out only when the provider uses checkout and the file is not opened yet.
	 * With a provider that uses changelists, an existing source changelist is preserved, otherwise a new one is created.
	 */
	FLOWEDITOR_API FText PrepareSourceControl(
		const UFlowAsset& SourceAsset,
		const FString& NewAssetName,
		FSourceControlContext& OutContext);

	/** Saves both assets and, with a provider that uses changelists, moves both into the target changelist. */
	FLOWEDITOR_API FText SaveAndFinalizeSourceControl(
		const UFlowAsset& NewAsset,
		const UFlowAsset& SourceAsset,
		const FSourceControlContext& SourceControlContext);
}
