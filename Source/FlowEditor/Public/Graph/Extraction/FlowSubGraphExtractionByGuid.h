// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Misc/Guid.h"

class UFlowAsset;
class UFlowGraphNode;

/**
 * SubGraph extraction for procedural callers, such as editor tools, scripts and agents.
 * Nodes are addressed by GUID, so no open graph editor or editor selection is needed.
 */
namespace FlowSubGraphExtraction
{
	struct FLOWEDITOR_API FExtractionPlanSummary
	{
		bool bCanApply = false;
		int32 SelectedNodeCount = 0;
		int32 EntryCount = 0;
		int32 ExitCount = 0;
		TArray<FString> Errors;
		TArray<FString> Warnings;
		TArray<FString> InterfaceInputs;
		TArray<FString> InterfaceOutputs;
	};

	FLOWEDITOR_API bool PlanExtractionByGuid(
		const UFlowAsset* SourceAsset,
		const TArray<FGuid>& SelectionGuids,
		const FString& NewAssetName,
		FExtractionPlanSummary& OutPlan,
		FString& OutError);

	/**
	 * Extracts the selection in an undoable transaction and saves both assets under revision control.
	 * OutError may describe a save or revision control failure even when the extraction succeeded.
	 */
	FLOWEDITOR_API bool ApplyExtractionByGuid(
		UFlowAsset* SourceAsset,
		const TArray<FGuid>& SelectionGuids,
		const FString& NewAssetName,
		const FExtractionPlanSummary& PlanSummary,
		UFlowAsset*& OutNewAsset,
		UFlowGraphNode*& OutSubGraphNode,
		FString& OutError);
}
