// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowObjectDiff.h"

#include "Asset/FlowDiffControl.h"
#include "Nodes/FlowNodeBase.h"
#include "EdGraph/EdGraph.h"

#include "Graph/Nodes/FlowGraphNode.h"
#include "SBlueprintDiff.h"

#include "DiffResults.h"

/////////////////////////////////////////////////////////////////////////////
/// FFlowNodePropertyDiff
FFlowObjectDiffArgs::FFlowObjectDiffArgs(TWeakPtr<FFlowObjectDiff> InFlowNodeDiff, const FSingleObjectDiffEntry& InPropertyDiff)
	: FlowNodeDiff(InFlowNodeDiff),
	PropertyDiff(InPropertyDiff)
{
}

/////////////////////////////////////////////////////////////////////////////
/// FFlowObjectDiff
FFlowObjectDiff::FFlowObjectDiff(TSharedPtr<FDiffResultItem> InDiffResult, const FFlowGraphToDiff& GraphToDiff)
	: DiffResult(InDiffResult)
{
	//ensure we do not generate details panels for pin changes.
	if (InDiffResult->Result.Pin1 == nullptr && InDiffResult->Result.Pin2 == nullptr)
	{
		InitializeDetailsDiffFromNode(InDiffResult->Result.Node1, InDiffResult->Result.Object1, GraphToDiff);
		InitializeDetailsDiffFromNode(InDiffResult->Result.Node2, InDiffResult->Result.Object2, GraphToDiff);
	}
}

void FFlowObjectDiff::InitializeDetailsDiffFromNode(UEdGraphNode* Node, const UObject* Object, const FFlowGraphToDiff& GraphToDiff)
{
	if (!IsValid(Node))
	{
		return;
	}

	if (!IsValid(Object))
	{
		const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Node);
		if (IsValid(FlowGraphNode))
		{
			Object = FlowGraphNode->GetNodeTemplate();
		}
	}
	const ENodeDiffType NodeDiffType = GraphToDiff.GetNodeDiffType(*Node);

	if (NodeDiffType == ENodeDiffType::Old && !OldDetailsView.IsValid())
	{
		OldDetailsView = MakeShared<FDetailsDiff>(Object, FOnDisplayedPropertiesChanged());
	}
	else if (NodeDiffType == ENodeDiffType::New && !NewDetailsView.IsValid())
	{
		NewDetailsView = MakeShared<FDetailsDiff>(Object, FOnDisplayedPropertiesChanged());
	}
}

void FFlowObjectDiff::DiffProperties(TArray<FSingleObjectDiffEntry>& OutPropertyDiffsArray) const
{
	if (OldDetailsView.IsValid() && NewDetailsView.IsValid())
	{
		static constexpr bool bSortByDisplayOrder = true;
		OldDetailsView->DiffAgainst(*NewDetailsView.Get(), OutPropertyDiffsArray, bSortByDisplayOrder);
	}
}

void FFlowObjectDiff::OnSelectDiff(const FSingleObjectDiffEntry& Property) const
{
	if (Property.DiffType == EPropertyDiffType::Type::Invalid)
	{
		return;
	}

	if (OldDetailsView.IsValid())
	{
		OldDetailsView->HighlightProperty(Property.Identifier);
	}

	if (NewDetailsView.IsValid())
	{
		NewDetailsView->HighlightProperty(Property.Identifier);
	}
}
