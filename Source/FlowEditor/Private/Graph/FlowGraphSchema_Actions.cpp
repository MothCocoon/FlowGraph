#include "Graph/FlowGraphSchema_Actions.h"

#include "Asset/FlowAssetEditor.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphUtils.h"

#include "Graph/Nodes/FlowGraphNode.h"
#include "Graph/Nodes/FlowGraphNode_In.h"
#include "Graph/Nodes/FlowGraphNode_Out.h"
#include "Graph/Nodes/FlowGraphNode_Reroute.h"

#include "FlowAsset.h"
#include "Nodes/FlowNode.h"

#include "Nodes/Route/FlowNode_Start.h"
#include "Nodes/Route/FlowNode_Finish.h"
#include "Nodes/Route/FlowNode_Reroute.h"

#include "Developer/ToolMenus/Public/ToolMenus.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphNode_Comment.h"
#include "Editor.h"
#include "Layout/SlateRect.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "FlowGraphSchema_Actions"

/////////////////////////////////////////////////////
// Flow Node

UEdGraphNode* FFlowGraphSchemaAction_NewNode::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode /* = true*/)
{
	// prevent adding new nodes while playing
	if (GEditor->PlayWorld != nullptr)
	{
		return nullptr;
	}

	if (NodeClass->IsChildOf(UFlowNode_Start::StaticClass()))
	{
		return CreateNode(ParentGraph, FromPin, UFlowNode_Start::StaticClass(), UFlowGraphNode_In::StaticClass(), Location, bSelectNewNode);
	}

	if (NodeClass->IsChildOf(UFlowNode_Finish::StaticClass()))
	{
		return CreateNode(ParentGraph, FromPin, UFlowNode_Finish::StaticClass(), UFlowGraphNode_Out::StaticClass(), Location, bSelectNewNode);
	}

	if (NodeClass->IsChildOf(UFlowNode_Reroute::StaticClass()))
	{
		return CreateNode(ParentGraph, FromPin, UFlowNode_Reroute::StaticClass(), UFlowGraphNode_Reroute::StaticClass(), Location, bSelectNewNode);
	}

	return CreateNode(ParentGraph, FromPin, NodeClass, UFlowGraphNode::StaticClass(), Location, bSelectNewNode);
}

UFlowGraphNode* FFlowGraphSchemaAction_NewNode::CreateNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin, UClass* NodeClass, UClass* GraphNodeClass, const FVector2D Location, const bool bSelectNewNode /*= true*/)
{
	check(NodeClass);

	const FScopedTransaction Transaction(LOCTEXT("AddNode", "Add Node"));

	ParentGraph->Modify();
	if (FromPin)
	{
		FromPin->Modify();
	}

	UFlowAsset* FlowAsset = CastChecked<UFlowGraph>(ParentGraph)->GetFlowAsset();
	FlowAsset->Modify();

	UFlowGraphNode* NewGraphNode = NewObject<UFlowGraphNode>(ParentGraph, GraphNodeClass, NAME_None, RF_Transactional);
	NewGraphNode->CreateNewGuid();

	NewGraphNode->NodePosX = Location.X;
	NewGraphNode->NodePosY = Location.Y;
	ParentGraph->AddNode(NewGraphNode, false, bSelectNewNode);

	UFlowNode* NewNode = FlowAsset->CreateNode(NodeClass, NewGraphNode);
	NewGraphNode->SetFlowNode(NewNode);

	NewGraphNode->PostPlacedNewNode();
	NewGraphNode->AllocateDefaultPins();

	ParentGraph->NotifyGraphChanged();

	const TSharedPtr<FFlowAssetEditor> FlowEditor = FFlowGraphUtils::GetFlowAssetEditor(ParentGraph);
	if (FlowEditor.IsValid())
	{
		FlowEditor->SelectSingleNode(NewGraphNode);
	}

	FlowAsset->PostEditChange();
	FlowAsset->MarkPackageDirty();

	return NewGraphNode;
}

UEdGraphNode* FFlowGraphSchemaAction_Paste::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode/* = true*/)
{
	// prevent adding new nodes while playing
	if (GEditor->PlayWorld == nullptr)
	{
		FFlowGraphUtils::GetFlowAssetEditor(ParentGraph)->PasteNodesHere(Location);
	}

	return nullptr;
}

/////////////////////////////////////////////////////
// Comment Node

UEdGraphNode* FFlowGraphSchemaAction_NewComment::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode/* = true*/)
{
	// prevent adding new nodes while playing
	if (GEditor->PlayWorld != nullptr)
	{
		return nullptr;
	}

	UEdGraphNode_Comment* CommentTemplate = NewObject<UEdGraphNode_Comment>();
	FVector2D SpawnLocation = Location;

	FSlateRect Bounds;
	if (FFlowGraphUtils::GetFlowAssetEditor(ParentGraph)->GetBoundsForSelectedNodes(Bounds, 50.0f))
	{
		CommentTemplate->SetBounds(Bounds);
		SpawnLocation.X = CommentTemplate->NodePosX;
		SpawnLocation.Y = CommentTemplate->NodePosY;
	}

	return FEdGraphSchemaAction_NewNode::SpawnNodeFromTemplate<UEdGraphNode_Comment>(ParentGraph, CommentTemplate, SpawnLocation);
}
