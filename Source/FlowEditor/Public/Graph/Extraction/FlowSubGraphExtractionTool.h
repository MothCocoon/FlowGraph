// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "CoreTypes.h"
#include "Graph/Extraction/FlowSubGraphExtractionApply.h"

class SFlowGraphEditor;
class UFlowAsset;

class FLOWEDITOR_API FFlowSubGraphExtractionTool
{
public:
	static void Execute(SFlowGraphEditor& GraphEditor);

	/**
	 * Applies the plan in an undoable transaction, then saves both assets under revision control.
	 * A failed extraction is undone and its generated asset is discarded, leaving Result.Error set and the result's objects cleared.
	 * OutSaveError describes a save or revision control failure that happened after a successful extraction.
	 */
	static FlowSubGraphExtraction::FExtractionResult ApplyAndSave(
		const FlowSubGraphExtraction::FExtractionPlan& Plan,
		UFlowAsset& SourceAsset,
		const FString& NewAssetName,
		FText& OutSaveError);
};
