// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraphSchema_Actions.h"

#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphEditor.h"
#include "Graph/FlowGraphSettings.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/FlowGraphUtils.h"
#include "Graph/Nodes/FlowGraphNode.h"

#include "FlowAsset.h"
#include "AddOns/FlowNodeAddOn.h"
#include "Nodes/FlowNode.h"

#include "EdGraph/EdGraph.h"
#include "EdGraphNode_Comment.h"
#include "Editor.h"
#include "ScopedTransaction.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowGraphSchema_Actions)

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

	if (NodeClass)
	{
		return CreateNode(ParentGraph, FromPin, NodeClass, Location, bSelectNewNode);
	}

	return nullptr;
}

UFlowGraphNode* FFlowGraphSchemaAction_NewNode::CreateNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const UClass* NodeClass, const FVector2D Location, const bool bSelectNewNode /*= true*/)
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

	// create new Flow Graph node
	const TSubclassOf<UFlowNodeBase> FlowNodeBaseClass = const_cast<UClass*>(NodeClass);
	const TSubclassOf<UEdGraphNode> GraphNodeClass = UFlowGraphSchema::GetAssignedGraphNodeClass(FlowNodeBaseClass);
	UFlowGraphNode* NewGraphNode = NewObject<UFlowGraphNode>(ParentGraph, GraphNodeClass, NAME_None, RF_Transactional);

	// register to the graph
	NewGraphNode->CreateNewGuid();
	ParentGraph->AddNode(NewGraphNode, false, bSelectNewNode);

	// link editor and runtime nodes together
	UFlowNode* FlowNode = FlowAsset->CreateNode(NodeClass, NewGraphNode);
	NewGraphNode->SetNodeTemplate(FlowNode);

	// create pins and connections
	NewGraphNode->ReconstructNode();
	NewGraphNode->AutowireNewNode(FromPin);

	// set position
	NewGraphNode->NodePosX = Location.X;
	NewGraphNode->NodePosY = Location.Y;

	// call notifies
	NewGraphNode->PostPlacedNewNode();
	ParentGraph->NotifyNodeChanged(NewGraphNode);

	FlowAsset->PostEditChange();

	// select in editor UI
	if (bSelectNewNode)
	{
		const TSharedPtr<SFlowGraphEditor> FlowGraphEditor = FFlowGraphUtils::GetFlowGraphEditor(ParentGraph);
		if (FlowGraphEditor.IsValid())
		{
			FlowGraphEditor->SelectSingleNode(NewGraphNode);
		}
	}

	return NewGraphNode;
}

UFlowGraphNode* FFlowGraphSchemaAction_NewNode::RecreateNode(UEdGraph* ParentGraph, UEdGraphNode* OldInstance, UFlowNode* FlowNode)
{
	check(FlowNode);

	ParentGraph->Modify();

	UFlowAsset* FlowAsset = CastChecked<UFlowGraph>(ParentGraph)->GetFlowAsset();
	FlowAsset->Modify();

	// create new Flow Graph node
	const TSubclassOf<UEdGraphNode> GraphNodeClass = UFlowGraphSchema::GetAssignedGraphNodeClass(FlowNode->GetClass());
	UFlowGraphNode* NewGraphNode = NewObject<UFlowGraphNode>(ParentGraph, GraphNodeClass, NAME_None, RF_Transactional);

	// register to the graph
	NewGraphNode->NodeGuid = FlowNode->GetGuid();
	ParentGraph->AddNode(NewGraphNode, false, false);

	// link editor and runtime nodes together
	FlowNode->SetGraphNode(NewGraphNode);
	NewGraphNode->SetNodeTemplate(FlowNode);

	// move links from the old node
	NewGraphNode->AllocateDefaultPins();
	if (OldInstance)
	{
		for (UEdGraphPin* OldPin : OldInstance->Pins)
		{
			if (OldPin->LinkedTo.Num() == 0)
			{
				continue;
			}

			for (UEdGraphPin* NewPin : NewGraphNode->Pins)
			{
				if (NewPin->Direction == OldPin->Direction && NewPin->PinName == OldPin->PinName)
				{
					TArray<UEdGraphPin*> Connections = OldPin->LinkedTo;
					for (UEdGraphPin* ConnectedPin : Connections)
					{
						ConnectedPin->BreakLinkTo(OldPin);
						ConnectedPin->MakeLinkTo(NewPin);
					}
				}
			}
		}
	}

	// keep old position
	NewGraphNode->NodePosX = OldInstance ? OldInstance->NodePosX : 0;
	NewGraphNode->NodePosY = OldInstance ? OldInstance->NodePosY : 0;

	// remove leftover
	if (OldInstance)
	{
		OldInstance->DestroyNode();
	}

	// call notifies
	NewGraphNode->PostPlacedNewNode();
	ParentGraph->NotifyGraphChanged();

	return NewGraphNode;
}

UFlowGraphNode* FFlowGraphSchemaAction_NewNode::ImportNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const UClass* NodeClass, const FGuid& NodeGuid, const FVector2D Location)
{
	check(NodeClass);

	ParentGraph->Modify();
	if (FromPin)
	{
		FromPin->Modify();
	}

	UFlowAsset* FlowAsset = CastChecked<UFlowGraph>(ParentGraph)->GetFlowAsset();
	FlowAsset->Modify();

	// create new Flow Graph node
	TSubclassOf<UFlowNodeBase> FlowNodeBaseClass = const_cast<UClass*>(NodeClass);
	const TSubclassOf<UEdGraphNode> GraphNodeClass = UFlowGraphSchema::GetAssignedGraphNodeClass(FlowNodeBaseClass);
	UFlowGraphNode* NewGraphNode = NewObject<UFlowGraphNode>(ParentGraph, GraphNodeClass, NAME_None, RF_Transactional);

	// register to the graph
	NewGraphNode->NodeGuid = NodeGuid;
	ParentGraph->AddNode(NewGraphNode, false, false);

	// link editor and runtime nodes together
	UFlowNode* FlowNode = FlowAsset->CreateNode(NodeClass, NewGraphNode);
	NewGraphNode->SetNodeTemplate(FlowNode);

	// create pins and connections
	NewGraphNode->AllocateDefaultPins();
	NewGraphNode->AutowireNewNode(FromPin);

	// set position
	NewGraphNode->NodePosX = Location.X;
	NewGraphNode->NodePosY = Location.Y;

	// call notifies
	NewGraphNode->PostPlacedNewNode();
	ParentGraph->NotifyGraphChanged();

	return NewGraphNode;
}

FText FFlowGraphSchemaAction_NewNode::GetNodeCategory(const UFlowNodeBase* Node, const UFlowGraphSettings& GraphSettings)
{
	const FString* CategoryOverridenByUser = GraphSettings.OverridenNodeCategories.Find(Node->GetClass());
	if (CategoryOverridenByUser && !CategoryOverridenByUser->IsEmpty())
	{
		return FText::FromString(*CategoryOverridenByUser);
	}

	return FText::FromString(Node->GetNodeCategory());
}

/////////////////////////////////////////////////////
// New SubNode (AddOn)

UEdGraphNode* FFlowSchemaAction_NewSubNode::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	ParentNode->AddSubNode(NodeTemplate, ParentGraph);
	return nullptr;
}

UEdGraphNode* FFlowSchemaAction_NewSubNode::PerformAction(class UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, const FVector2D Location, bool bSelectNewNode)
{
	return PerformAction(ParentGraph, nullptr, Location, bSelectNewNode);
}

void FFlowSchemaAction_NewSubNode::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdGraphSchemaAction::AddReferencedObjects(Collector);

	// These don't get saved to disk, but we want to make sure the objects don't get GC'd while the action array is around
	Collector.AddReferencedObject(NodeTemplate);
	Collector.AddReferencedObject(ParentNode);
}

UFlowGraphNode* FFlowSchemaAction_NewSubNode::RecreateNode(UEdGraph* ParentGraph, UEdGraphNode* OldInstance, UFlowGraphNode* ParentFlowGraphNode, UFlowNodeAddOn* FlowNodeAddOn)
{
	check(FlowNodeAddOn);

	UFlowAsset* FlowAsset = CastChecked<UFlowGraph>(ParentGraph)->GetFlowAsset();
	FlowAsset->Modify();

	// create new Flow Graph node
	const TSubclassOf<UEdGraphNode> GraphNodeClass = UFlowGraphSchema::GetAssignedGraphNodeClass(FlowNodeAddOn->GetClass());
	UFlowGraphNode* NewGraphNode = NewObject<UFlowGraphNode>(ParentGraph, GraphNodeClass, NAME_None, RF_Transactional);

	// link editor and runtime nodes together
	FlowNodeAddOn->SetGraphNode(NewGraphNode);
	NewGraphNode->SetNodeTemplate(FlowNodeAddOn);

	// remove leftover
	if (OldInstance)
	{
		OldInstance->DestroyNode();
	}

	ParentFlowGraphNode->AddSubNode(NewGraphNode, ParentGraph);

	// call notifies
	ParentGraph->NotifyGraphChanged();
	NewGraphNode->PostPlacedNewNode();

	return NewGraphNode;
}

TSharedPtr<FFlowSchemaAction_NewSubNode> FFlowSchemaAction_NewSubNode::AddNewSubNodeAction(FGraphActionListBuilderBase& ContextMenuBuilder, const FText& Category, const FText& MenuDesc, const FText& Tooltip)
{
	TSharedPtr<FFlowSchemaAction_NewSubNode> NewAction = MakeShared<FFlowSchemaAction_NewSubNode>(Category, MenuDesc, Tooltip, 0);
	ContextMenuBuilder.AddAction(NewAction);
	return NewAction;
}

/////////////////////////////////////////////////////
// Paste

UEdGraphNode* FFlowGraphSchemaAction_Paste::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, const bool bSelectNewNode/* = true*/)
{
	// prevent adding new nodes while playing
	if (GEditor->PlayWorld == nullptr)
	{
		FFlowGraphUtils::GetFlowGraphEditor(ParentGraph)->PasteNodesHere(Location);
	}

	return nullptr;
}

/////////////////////////////////////////////////////
// New Comment

UEdGraphNode* FFlowGraphSchemaAction_NewComment::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, const bool bSelectNewNode/* = true*/)
{
	// prevent adding new nodes while playing
	if (GEditor->PlayWorld != nullptr)
	{
		return nullptr;
	}

	UEdGraphNode_Comment* CommentTemplate = NewObject<UEdGraphNode_Comment>();
	FVector2D SpawnLocation = Location;

	const TSharedPtr<SFlowGraphEditor> FlowGraphEditor = FFlowGraphUtils::GetFlowGraphEditor(ParentGraph);
	if (FlowGraphEditor.IsValid())
	{
		FSlateRect Bounds;
		if (FlowGraphEditor->GetBoundsForSelectedNodes(Bounds, 50.0f))
		{
			CommentTemplate->SetBounds(Bounds);
			SpawnLocation.X = CommentTemplate->NodePosX;
			SpawnLocation.Y = CommentTemplate->NodePosY;
		}
	}

	return FEdGraphSchemaAction_NewNode::SpawnNodeFromTemplate<UEdGraphNode_Comment>(ParentGraph, CommentTemplate, SpawnLocation);
}

#undef LOCTEXT_NAMESPACE
