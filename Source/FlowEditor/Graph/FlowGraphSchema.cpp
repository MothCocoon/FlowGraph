#include "FlowGraphSchema.h"
#include "FlowAssetGraph.h"
#include "FlowGraphUtilities.h"
#include "Nodes/FlowGraphNode.h"
#include "Nodes/FlowGraphNode_In.h"

#include "Flow/Graph/FlowAsset.h"
#include "Flow/Graph/Nodes/FlowNode.h"
#include "Flow/Graph/Nodes/FlowNodeIn.h"

#include "Developer/ToolMenus/Public/ToolMenus.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphNode_Comment.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GraphEditor.h"
#include "GraphEditorActions.h"
#include "Layout/SlateRect.h"
#include "ScopedTransaction.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"

#define LOCTEXT_NAMESPACE "FlowGraphSchema"

TArray<UClass*> UFlowGraphSchema::FlowNodeClasses;
TArray<TSharedPtr<FString>> UFlowGraphSchema::FlowNodeCategories;
bool UFlowGraphSchema::bFlowNodesInitialized = false;

/////////////////////////////////////////////////////
// FFlowGraphSchemaAction_NewNode

UEdGraphNode* FFlowGraphSchemaAction_NewNode::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode/* = true*/)
{
	check(FlowNodeClass);

	UFlowAsset* FlowAsset = CastChecked<UFlowAssetGraph>(ParentGraph)->GetFlowAsset();
	const FScopedTransaction Transaction(LOCTEXT("FlowEditorNewFlowNode", "Flow Editor: New Flow Node"));
	ParentGraph->Modify();
	FlowAsset->Modify();

	UFlowNode* NewNode = FlowAsset->CreateNode<UFlowNode>(FlowNodeClass, bSelectNewNode);

	NewNode->GetGraphNode()->NodePosX = Location.X;
	NewNode->GetGraphNode()->NodePosY = Location.Y;

	NewNode->GetGraphNode()->AutowireNewNode(FromPin);

	FlowAsset->PostEditChange();
	FlowAsset->MarkPackageDirty();

	return NewNode->GetGraphNode();
}

void FFlowGraphSchemaAction_NewNode::ConnectToSelectedNodes(UFlowNode* NewNode, class UEdGraph* ParentGraph) const
{
	const FGraphPanelSelectionSet SelectedNodes = FFlowGraphUtilities::GetSelectedNodes(ParentGraph);

	TArray<UFlowNode*> SortedNodes;
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt);

		if (SelectedNode)
		{
			// Sort the nodes by y position
			bool bInserted = false;
			for (int32 Index = 0; Index < SortedNodes.Num(); ++Index)
			{
				if (SortedNodes[Index]->GetGraphNode()->NodePosY > SelectedNode->NodePosY)
				{
					SortedNodes.Insert(SelectedNode->GetFlowNode(), Index);
					bInserted = true;
					break;
				}
			}
			if (!bInserted)
			{
				SortedNodes.Add(SelectedNode->GetFlowNode());
			}
		}
	}
	if (SortedNodes.Num() > 1)
	{
		CastChecked<UFlowGraphSchema>(NewNode->GetGraphNode()->GetSchema())->TryConnectNodes(SortedNodes, NewNode);
	}
}

/////////////////////////////////////////////////////
// FFlowGraphSchemaAction_NewFromSelected

UEdGraphNode* FFlowGraphSchemaAction_NewFromSelected::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode/* = true*/)
{
	UFlowAsset* FlowAsset = CastChecked<UFlowAssetGraph>(ParentGraph)->GetFlowAsset();
	const FScopedTransaction Transaction(LOCTEXT("FlowEditorNewFromSelection", "Flow Editor: New From Selection"));
	ParentGraph->Modify();
	FlowAsset->Modify();

	UEdGraphNode* CreatedNode = nullptr;
	FVector2D WaveStartLocation = Location;

	if (FlowNodeClass)
	{
		// If we will create another node, move wave nodes out of the way.
		WaveStartLocation.X -= 200;
	}

	TArray<UFlowNode*> CreatedNodes;

	if (FlowNodeClass)
	{
		UFlowNode* NewNode = FlowAsset->CreateNode<UFlowNode>(FlowNodeClass, bSelectNewNode);

		const UFlowGraphSchema* NewSchema = CastChecked<UFlowGraphSchema>(NewNode->GetGraphNode()->GetSchema());
		NewSchema->TryConnectNodes(CreatedNodes, NewNode);

		NewNode->GetGraphNode()->NodePosX = Location.X;
		NewNode->GetGraphNode()->NodePosY = Location.Y;

		CreatedNode = NewNode->GetGraphNode();
	}
	else
	{
		if (CreatedNodes.Num() > 0)
		{
			CreatedNode = CreatedNodes[0]->GetGraphNode();
		}
	}

	if (CreatedNode)
	{
		CreatedNode->AutowireNewNode(FromPin);
	}

	FlowAsset->PostEditChange();
	FlowAsset->MarkPackageDirty();

	return CreatedNode;
}

/////////////////////////////////////////////////////
// FFlowGraphSchemaAction_NewComment

UEdGraphNode* FFlowGraphSchemaAction_NewComment::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode/* = true*/)
{
	// Add menu item for creating comment boxes
	UEdGraphNode_Comment* CommentTemplate = NewObject<UEdGraphNode_Comment>();

	FVector2D SpawnLocation = Location;

	FSlateRect Bounds;
	if (FFlowGraphUtilities::GetBoundsForSelectedNodes(ParentGraph, Bounds, 50.0f))
	{
		CommentTemplate->SetBounds(Bounds);
		SpawnLocation.X = CommentTemplate->NodePosX;
		SpawnLocation.Y = CommentTemplate->NodePosY;
	}

	return FEdGraphSchemaAction_NewNode::SpawnNodeFromTemplate<UEdGraphNode_Comment>(ParentGraph, CommentTemplate, SpawnLocation);
}

/////////////////////////////////////////////////////
// FFlowGraphSchemaAction_Paste

UEdGraphNode* FFlowGraphSchemaAction_Paste::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode/* = true*/)
{
	FFlowGraphUtilities::PasteNodesHere(ParentGraph, Location);
	return nullptr;
}

/////////////////////////////////////////////////////
// UFlowGraphSchema

UFlowGraphSchema::UFlowGraphSchema(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFlowGraphSchema::GetPaletteActions(FGraphActionMenuBuilder& ActionMenuBuilder, const FString& CategoryName) const
{
	GetFlowNodeActions(ActionMenuBuilder, CategoryName);
	GetCommentAction(ActionMenuBuilder);
}

void UFlowGraphSchema::TryConnectNodes(const TArray<UFlowNode*>& OutputNodes, UFlowNode* InputNode) const
{
	for (int32 Index = 0; Index < OutputNodes.Num(); Index++)
	{
		UFlowGraphNode* GraphNode = CastChecked<UFlowGraphNode>(InputNode->GetGraphNode());
		if (Index >= GraphNode->GetInputCount())
		{
			GraphNode->CreateInputPin();
		}
		TryCreateConnection(GraphNode->GetInputPin(Index), CastChecked<UFlowGraphNode>(OutputNodes[Index]->GetGraphNode())->GetOutputPin(0));
	}
}

void UFlowGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	GetFlowNodeActions(ContextMenuBuilder, FString());
	GetCommentAction(ContextMenuBuilder, ContextMenuBuilder.CurrentGraph);

	if (!ContextMenuBuilder.FromPin && FFlowGraphUtilities::CanPasteNodes(ContextMenuBuilder.CurrentGraph))
	{
		TSharedPtr<FFlowGraphSchemaAction_Paste> NewAction(new FFlowGraphSchemaAction_Paste(FText::GetEmpty(), LOCTEXT("PasteHereAction", "Paste here"), FText::GetEmpty(), 0));
		ContextMenuBuilder.AddAction(NewAction);
	}
}

void UFlowGraphSchema::GetContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	if (Context->Pin)
	{
		if (Context->Pin->LinkedTo.Num() > 0)
		{
			FToolMenuSection& Section = Menu->AddSection("FlowGraphSchemaPinActions", LOCTEXT("PinActionsMenuHeader", "Pin Actions"));
			Section.AddMenuEntry(FGraphEditorCommands::Get().BreakPinLinks);
		}
	}
	else if (Context->Node)
	{
		const UFlowGraphNode* FlowGraphNode = Cast<const UFlowGraphNode>(Context->Node);

		FToolMenuSection& Section = Menu->AddSection("FlowGraphSchemaNodeActions", LOCTEXT("NodeActionsMenuHeader", "Node Actions"));
		Section.AddMenuEntry(FGraphEditorCommands::Get().BreakNodeLinks);
	}

	Super::GetContextMenuActions(Menu, Context);
}

void UFlowGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	UFlowAsset* FlowAsset = CastChecked<UFlowAssetGraph>(&Graph)->GetFlowAsset();
	Graph.Modify();
	FlowAsset->Modify();

	UFlowNodeIn* NewNode = FlowAsset->CreateNode<UFlowNodeIn>(UFlowNodeIn::StaticClass());
	NewNode->GetGraphNode()->NodePosX = 0;
	NewNode->GetGraphNode()->NodePosY = 0;

	SetNodeMetaData(NewNode->GetGraphNode(), FNodeMetadata::DefaultGraphNode);

	FlowAsset->PostEditChange();
	FlowAsset->MarkPackageDirty();
}

const FPinConnectionResponse UFlowGraphSchema::CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
	// Make sure the pins are not on the same node
	if (PinA->GetOwningNode() == PinB->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnectionSameNode", "Both are on the same node"));
	}

	// Compare the directions
	const UEdGraphPin* InputPin = nullptr;
	const UEdGraphPin* OutputPin = nullptr;

	if (!CategorizePinsByDirection(PinA, PinB, /*out*/ InputPin, /*out*/ OutputPin))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnectionIncompatible", "Directions are not compatible"));
	}

	// Break existing connections on outputs only - multiple input connections are acceptable
	if (OutputPin->LinkedTo.Num() > 0)
	{
		ECanCreateConnectionResponse ReplyBreakInputs = (OutputPin == PinA ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B);
		return FPinConnectionResponse(ReplyBreakInputs, LOCTEXT("ConnectionReplace", "Replace existing connections"));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
}

bool UFlowGraphSchema::TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	bool bModified = UEdGraphSchema::TryCreateConnection(PinA, PinB);

	if (bModified)
	{
		CastChecked<UFlowAssetGraph>(PinA->GetOwningNode()->GetGraph())->GetFlowAsset()->CompileNodeConnections();
	}

	return bModified;
}

bool UFlowGraphSchema::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const
{
	return true;
}

FLinearColor UFlowGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FLinearColor::White;
}

void UFlowGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	Super::BreakNodeLinks(TargetNode);

	CastChecked<UFlowAssetGraph>(TargetNode.GetGraph())->GetFlowAsset()->CompileNodeConnections();
}

void UFlowGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakPinLinks", "Break Pin Links"));

	Super::BreakPinLinks(TargetPin, bSendsNodeNotifcation);

	// if this would notify the node then we need to compile the FlowAsset
	if (bSendsNodeNotifcation)
	{
		CastChecked<UFlowAssetGraph>(TargetPin.GetOwningNode()->GetGraph())->GetFlowAsset()->CompileNodeConnections();
	}
}

TArray<TSharedPtr<FString>> UFlowGraphSchema::GetFlowNodeCategories()
{
	InitFlowNodes();
	return FlowNodeCategories;
}

void UFlowGraphSchema::GetFlowNodeActions(FGraphActionMenuBuilder& ActionMenuBuilder, const FString& CategoryName) const
{
	InitFlowNodes();

	for (UClass* FlowNodeClass : FlowNodeClasses)
	{
		const UFlowNode* FlowNode = FlowNodeClass->GetDefaultObject<UFlowNode>();

		// filter out nodes that can't automatically wired to the selected node
		if (ActionMenuBuilder.FromPin)
		{
			switch (ActionMenuBuilder.FromPin->Direction)
			{
				case EEdGraphPinDirection::EGPD_Input:
					if (!FlowNode->HasOutputPins())
					{
						continue;
					}
					break;
				case EEdGraphPinDirection::EGPD_Output:
					if (!FlowNode->HasInputPins())
					{
						continue;
					}
					break;
			}
		}

		// filter by category
		if (CategoryName.IsEmpty() || CategoryName.Equals(FlowNode->GetCategory()))
		{
			const FText Name = FlowNode->GetTitle();

			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("Name"), Name);
			const FText AddToolTip = FText::Format(LOCTEXT("NewFlowNodeTooltip", "Adds {Name} node here"), Arguments);
			TSharedPtr<FFlowGraphSchemaAction_NewNode> NewNodeAction(new FFlowGraphSchemaAction_NewNode(FText::FromString(FlowNode->GetCategory()), Name, AddToolTip, 0));
			ActionMenuBuilder.AddAction(NewNodeAction);
			NewNodeAction->FlowNodeClass = FlowNodeClass;
		}
	}
}

void UFlowGraphSchema::GetCommentAction(FGraphActionMenuBuilder& ActionMenuBuilder, const UEdGraph* CurrentGraph /*= nullptr*/) const
{
	if (!ActionMenuBuilder.FromPin)
	{
		const bool bIsManyNodesSelected = CurrentGraph ? (FFlowGraphUtilities::GetNumberOfSelectedNodes(CurrentGraph) > 0) : false;
		const FText MenuDescription = bIsManyNodesSelected ? LOCTEXT("CreateCommentAction", "Create Comment from Selection") : LOCTEXT("AddCommentAction", "Add Comment...");
		const FText ToolTip = LOCTEXT("CreateCommentToolTip", "Creates a comment.");

		TSharedPtr<FFlowGraphSchemaAction_NewComment> NewAction(new FFlowGraphSchemaAction_NewComment(FText::GetEmpty(), MenuDescription, ToolTip, 0));
		ActionMenuBuilder.AddAction(NewAction);
	}
}

void UFlowGraphSchema::InitFlowNodes()
{
	if (bFlowNodesInitialized)
	{
		return;
	}

	TSet<FString> UnsortedCategories;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->IsChildOf(UFlowNode::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract) && !It->HasAnyClassFlags(CLASS_NotPlaceable))
		{
			UClass* FlowNodeClass = *It;
			FlowNodeClasses.Add(*It);

			const UFlowNode* DefaultObject = FlowNodeClass->GetDefaultObject<UFlowNode>();
			UnsortedCategories.Add(DefaultObject->GetCategory());
		}
	}

	FlowNodeClasses.Sort();

	TArray<FString> SortedCategories = UnsortedCategories.Array();
	SortedCategories.Sort();
	for (const FString& Category : SortedCategories)
	{
		FlowNodeCategories.Add(MakeShareable(new FString(Category)));
	}

	bFlowNodesInitialized = true;
}

int32 UFlowGraphSchema::GetNodeSelectionCount(const UEdGraph* Graph) const
{
	return FFlowGraphUtilities::GetNumberOfSelectedNodes(Graph);
}

TSharedPtr<FEdGraphSchemaAction> UFlowGraphSchema::GetCreateCommentAction() const
{
	return TSharedPtr<FEdGraphSchemaAction>(static_cast<FEdGraphSchemaAction*>(new FFlowGraphSchemaAction_NewComment));
}

#undef LOCTEXT_NAMESPACE
