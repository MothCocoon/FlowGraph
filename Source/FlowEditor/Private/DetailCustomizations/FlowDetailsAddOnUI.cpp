// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowDetailsAddOnUI.h"

#include "Graph/Nodes/FlowGraphNode.h"
#include "Graph/Widgets/SGraphEditorActionMenuFlow.h"
#include "Nodes/FlowNodeBase.h"

#include "EdGraph/EdGraph.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FlowDetailsAddOnUI"

UFlowGraphNode* FFlowDetailsAddOnUI::FindGraphNodeForEditedObject(UObject* EditedObject)
{
	if (!IsValid(EditedObject))
	{
		return nullptr;
	}

	// Find any UFlowGraphNode whose node instance matches the edited object.
	for (TObjectIterator<UFlowGraphNode> It; It; ++It)
	{
		UFlowGraphNode* Candidate = *It;
		if (!IsValid(Candidate))
		{
			continue;
		}

		UObject* NodeInstance = Candidate->GetFlowNodeBase();
		if (NodeInstance == EditedObject)
		{
			return Candidate;
		}
	}

	return nullptr;
}

UEdGraph* FFlowDetailsAddOnUI::GetOwningEdGraph(UFlowGraphNode* GraphNode)
{
	return IsValid(GraphNode) ? GraphNode->GetGraph() : nullptr;
}

bool FFlowDetailsAddOnUI::CanAttachAddOn(UObject* EditedObject)
{
	UFlowGraphNode* GraphNode = FindGraphNodeForEditedObject(EditedObject);
	if (!IsValid(GraphNode))
	{
		return false;
	}

	return IsValid(GetOwningEdGraph(GraphNode));
}

TSharedRef<SWidget> FFlowDetailsAddOnUI::BuildAttachAddOnMenuContent(UObject* EditedObject)
{
	UFlowGraphNode* GraphNode = FindGraphNodeForEditedObject(EditedObject);
	UEdGraph* Graph = GetOwningEdGraph(GraphNode);

	if (!IsValid(GraphNode) || !IsValid(Graph))
	{
		return SNew(STextBlock)
			.Text(LOCTEXT("AttachAddOnUnavailable", "Attach AddOn is unavailable (no owning graph node found)."));
	}

	return BuildAttachAddOnMenuContent(Graph, GraphNode);
}

TSharedRef<SWidget> FFlowDetailsAddOnUI::BuildAttachAddOnMenuContent(UEdGraph* Graph, UFlowGraphNode* GraphNode)
{
	if (!IsValid(Graph) || !IsValid(GraphNode))
	{
		return SNew(STextBlock)
			.Text(LOCTEXT("AttachAddOnUnavailable2", "Attach AddOn is unavailable."));
	}

	// Wrap for sizing similar to the context menu widget
	return SNew(SBox)
		.WidthOverride(420.0f)
		.HeightOverride(520.0f)
		[
			SNew(SGraphEditorActionMenuFlow)
			.GraphObj(Graph)
			.GraphNode(GraphNode)
			.AutoExpandActionMenu(true)
		];
}

#undef LOCTEXT_NAMESPACE