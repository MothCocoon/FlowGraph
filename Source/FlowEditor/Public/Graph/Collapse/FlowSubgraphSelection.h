// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Misc/Guid.h"

class UFlowAsset;
class UFlowGraphNode;

namespace FlowSubgraphSelection
{
	struct FLOWEDITOR_API FSelectionPlanSummary
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

	FLOWEDITOR_API bool PlanSelection(
		UFlowAsset* SourceAsset,
		const TArray<FGuid>& SelectionGuids,
		const FString& NewAssetName,
		FSelectionPlanSummary& OutPlan,
		FString& OutError);

	FLOWEDITOR_API bool ApplySelection(
		UFlowAsset* SourceAsset,
		const TArray<FGuid>& SelectionGuids,
		const FString& NewAssetName,
		const FSelectionPlanSummary& PlanSummary,
		UFlowAsset*& OutNewAsset,
		UFlowGraphNode*& OutSubgraphNode,
		FString& OutError);
}
