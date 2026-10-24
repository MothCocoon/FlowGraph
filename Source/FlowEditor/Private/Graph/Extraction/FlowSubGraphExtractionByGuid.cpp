// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Graph/Extraction/FlowSubGraphExtractionByGuid.h"

#include "FlowAsset.h"
#include "Graph/Extraction/FlowSubGraphExtractionApply.h"
#include "Graph/Extraction/FlowSubGraphExtractionPlan.h"
#include "Graph/Extraction/FlowSubGraphExtractionTool.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Nodes/FlowNode.h"
#include "Misc/Optional.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	bool ResolveSelection(
		const UFlowAsset& SourceAsset,
		const TArray<FGuid>& SelectionGuids,
		TSet<UFlowGraphNode*>& OutSelectedNodes,
		FString& OutError)
	{
		OutSelectedNodes.Reset();
		for (const FGuid& SelectionGuid : SelectionGuids)
		{
			const UFlowNode* FlowNode = SourceAsset.GetNode(SelectionGuid);
			if (!IsValid(FlowNode))
			{
				OutError = FString::Printf(
					TEXT("Selection contains unknown node GUID: %s"),
					*SelectionGuid.ToString());
				return false;
			}

			UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(FlowNode->GetGraphNode());
			if (!IsValid(GraphNode))
			{
				OutError = FString::Printf(
					TEXT("Selection node %s has no editor graph node."),
					*SelectionGuid.ToString());
				return false;
			}

			OutSelectedNodes.Add(GraphNode);
		}

		if (OutSelectedNodes.IsEmpty())
		{
			OutError = TEXT("SelectionGuids must contain at least one node GUID.");
			return false;
		}

		return true;
	}

	void CopyPlan(
		const FlowSubGraphExtraction::FExtractionPlan& Source,
		FlowSubGraphExtraction::FExtractionPlanSummary& Destination)
	{
		Destination.bCanApply = Source.CanApply();
		Destination.SelectedNodeCount = Source.SelectedNodes.Num();
		Destination.EntryCount = Source.Entries.Num();
		Destination.ExitCount = Source.Exits.Num();

		for (const FText& Error : Source.Errors)
		{
			Destination.Errors.Add(Error.ToString());
		}
		for (const FText& Warning : Source.Warnings)
		{
			Destination.Warnings.Add(Warning.ToString());
		}
		for (const FlowSubGraphExtraction::FBoundaryPin& Entry : Source.Entries)
		{
			Destination.InterfaceInputs.Add(Entry.InterfaceName.ToString());
		}
		for (const FlowSubGraphExtraction::FBoundaryPin& Exit : Source.Exits)
		{
			Destination.InterfaceOutputs.Add(Exit.InterfaceName.ToString());
		}
	}

	// Plans again against the live graph, so a summary made before the graph changed cannot authorize an apply.
	TOptional<FlowSubGraphExtraction::FExtractionPlan> ReplanForApply(
		const UFlowAsset* SourceAsset,
		const TArray<FGuid>& SelectionGuids,
		const FlowSubGraphExtraction::FExtractionPlanSummary& PlanSummary,
		FString& OutError)
	{
		if (!PlanSummary.bCanApply)
		{
			OutError = TEXT("The supplied SubGraph extraction plan cannot be applied.");
			return {};
		}

		TSet<UFlowGraphNode*> SelectedNodes;
		if (!IsValid(SourceAsset) || !ResolveSelection(*SourceAsset, SelectionGuids, SelectedNodes, OutError))
		{
			return {};
		}

		FlowSubGraphExtraction::FExtractionPlan Plan = FlowSubGraphExtraction::PlanExtraction(SelectedNodes);
		if (!Plan.CanApply())
		{
			for (const FText& Error : Plan.Errors)
			{
				OutError += Error.ToString() + TEXT(" ");
			}
			return {};
		}

		return Plan;
	}
}

namespace FlowSubGraphExtraction
{
	bool PlanExtractionByGuid(
		const UFlowAsset* SourceAsset,
		const TArray<FGuid>& SelectionGuids,
		const FString& NewAssetName,
		FExtractionPlanSummary& OutPlan,
		FString& OutError)
	{
		OutPlan = FExtractionPlanSummary();
		OutError.Reset();

		if (!IsValid(SourceAsset))
		{
			OutError = TEXT("Source asset is invalid.");
			return false;
		}

		const FText NameError = FlowSubGraphExtraction::ValidateNewAssetName(*SourceAsset, NewAssetName);

		TSet<UFlowGraphNode*> SelectedNodes;
		if (!ResolveSelection(*SourceAsset, SelectionGuids, SelectedNodes, OutError))
		{
			return false;
		}

		const FlowSubGraphExtraction::FExtractionPlan Plan =
			FlowSubGraphExtraction::PlanExtraction(SelectedNodes);
		CopyPlan(Plan, OutPlan);
		if (!NameError.IsEmpty())
		{
			OutPlan.Errors.Add(NameError.ToString());
			OutPlan.bCanApply = false;
		}
		return true;
	}

	bool ApplyExtractionByGuid(
		UFlowAsset* SourceAsset,
		const TArray<FGuid>& SelectionGuids,
		const FString& NewAssetName,
		const FExtractionPlanSummary& PlanSummary,
		UFlowAsset*& OutNewAsset,
		UFlowGraphNode*& OutSubGraphNode,
		FString& OutError)
	{
		OutNewAsset = nullptr;
		OutSubGraphNode = nullptr;
		OutError.Reset();

		const TOptional<FExtractionPlan> Plan = ReplanForApply(SourceAsset, SelectionGuids, PlanSummary, OutError);
		if (!Plan.IsSet())
		{
			return false;
		}

		FText SaveError;
		const FlowSubGraphExtraction::FExtractionResult Result =
			FFlowSubGraphExtractionTool::ApplyAndSave(Plan.GetValue(), *SourceAsset, NewAssetName, SaveError);
		if (!Result.Succeeded())
		{
			OutError = Result.Error.ToString();
			return false;
		}

		OutError = SaveError.ToString();
		OutNewAsset = Result.NewAsset;
		OutSubGraphNode = Result.SubGraphNode;
		return true;
	}

	bool ApplyExtractionByGuidWithoutSave(
		UFlowAsset* SourceAsset,
		const TArray<FGuid>& SelectionGuids,
		const FString& NewAssetName,
		const FExtractionPlanSummary& PlanSummary,
		UFlowAsset*& OutNewAsset,
		UFlowGraphNode*& OutSubGraphNode,
		FString& OutError)
	{
		OutNewAsset = nullptr;
		OutSubGraphNode = nullptr;
		OutError.Reset();

		const TOptional<FExtractionPlan> Plan = ReplanForApply(SourceAsset, SelectionGuids, PlanSummary, OutError);
		if (!Plan.IsSet())
		{
			return false;
		}

		const FlowSubGraphExtraction::FExtractionResult Result =
			FlowSubGraphExtraction::ApplyExtraction(Plan.GetValue(), *SourceAsset, NewAssetName);

		// Reported on failure too, so the caller can discard a partially built asset
		OutNewAsset = Result.NewAsset;
		if (!Result.Succeeded())
		{
			OutError = Result.Error.ToString();
			return false;
		}

		OutSubGraphNode = Result.SubGraphNode;
		return true;
	}
}
