// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/Collapse/FlowSubgraphSelection.h"

#include "FlowAsset.h"
#include "FlowCollapseToSubGraphApply.h"
#include "FlowCollapseToSubGraphPlan.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Nodes/FlowNode.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	bool ResolveSelection(
		UFlowAsset& SourceAsset,
		const TArray<FGuid>& SelectionGuids,
		TSet<UFlowGraphNode*>& OutSelectedNodes,
		FString& OutError)
	{
		OutSelectedNodes.Reset();
		for (const FGuid& SelectionGuid : SelectionGuids)
		{
			UFlowNode* FlowNode = SourceAsset.GetNode(SelectionGuid);
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
		const FlowCollapseToSubGraph::FCollapsePlan& Source,
		FlowSubgraphSelection::FSelectionPlanSummary& Destination)
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
		for (const FlowCollapseToSubGraph::FBoundaryPin& Entry : Source.Entries)
		{
			Destination.InterfaceInputs.Add(Entry.InterfaceName.ToString());
		}
		for (const FlowCollapseToSubGraph::FBoundaryPin& Exit : Source.Exits)
		{
			Destination.InterfaceOutputs.Add(Exit.InterfaceName.ToString());
		}
	}
}

namespace FlowSubgraphSelection
{
	bool PlanSelection(
		UFlowAsset* SourceAsset,
		const TArray<FGuid>& SelectionGuids,
		const FString& NewAssetName,
		FSelectionPlanSummary& OutPlan,
		FString& OutError)
	{
		OutPlan = FSelectionPlanSummary();
		OutError.Reset();

		if (!IsValid(SourceAsset))
		{
			OutError = TEXT("Source asset is invalid.");
			return false;
		}

		const FText NameError = FlowCollapseToSubGraph::ValidateNewAssetName(*SourceAsset, NewAssetName);

		TSet<UFlowGraphNode*> SelectedNodes;
		if (!ResolveSelection(*SourceAsset, SelectionGuids, SelectedNodes, OutError))
		{
			return false;
		}

		const FlowCollapseToSubGraph::FCollapsePlan Plan =
			FlowCollapseToSubGraph::PlanCollapse(SelectedNodes);
		CopyPlan(Plan, OutPlan);
		if (!NameError.IsEmpty())
		{
			OutPlan.Errors.Add(NameError.ToString());
			OutPlan.bCanApply = false;
		}
		return true;
	}

	bool ApplySelection(
		UFlowAsset* SourceAsset,
		const TArray<FGuid>& SelectionGuids,
		const FString& NewAssetName,
		const FSelectionPlanSummary& PlanSummary,
		UFlowAsset*& OutNewAsset,
		UFlowGraphNode*& OutSubgraphNode,
		FString& OutError)
	{
		OutNewAsset = nullptr;
		OutSubgraphNode = nullptr;
		OutError.Reset();

		if (!PlanSummary.bCanApply)
		{
			OutError = TEXT("The supplied subgraph selection plan cannot be applied.");
			return false;
		}

		TSet<UFlowGraphNode*> SelectedNodes;
		if (!IsValid(SourceAsset) || !ResolveSelection(*SourceAsset, SelectionGuids, SelectedNodes, OutError))
		{
			return false;
		}

		const FlowCollapseToSubGraph::FCollapsePlan Plan =
			FlowCollapseToSubGraph::PlanCollapse(SelectedNodes);
		if (!Plan.CanApply())
		{
			for (const FText& Error : Plan.Errors)
			{
				OutError += Error.ToString() + TEXT(" ");
			}
			return false;
		}

		const FlowCollapseToSubGraph::FCollapseResult Result =
			FlowCollapseToSubGraph::ApplyCollapse(Plan, *SourceAsset, NewAssetName);
		if (!Result.Succeeded())
		{
			OutError = Result.Error.ToString();
			return false;
		}

		OutNewAsset = Result.NewAsset;
		OutSubgraphNode = Result.SubGraphNode;
		return true;
	}
}
