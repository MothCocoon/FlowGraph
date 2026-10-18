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
		TSharedPtr<ISourceControlChangelist, ESPMode::ThreadSafe> Changelist;
		FString ChangelistIdentifier;
		bool bEnabled = false;
	};

	/** Removes a newly created sub-graph asset after a failed extraction. */
	FLOWEDITOR_API bool DiscardCreatedAsset(UFlowAsset* NewAsset);

	/**
	 * Resolves the target changelist before either asset is saved. An existing source checkout is
	 * preserved; otherwise a changelist is created and the source is checked out into it.
	 */
	FLOWEDITOR_API FText PrepareSourceControl(
		const UFlowAsset& SourceAsset,
		const FString& NewAssetName,
		FSourceControlContext& OutContext);

	/** Saves both assets and moves the generated file into the source asset's changelist. */
	FLOWEDITOR_API FText SaveAndFinalizeSourceControl(
		const UFlowAsset& NewAsset,
		const UFlowAsset& SourceAsset,
		const FSourceControlContext& SourceControlContext);
}
