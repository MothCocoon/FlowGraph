#include "FlowGraphSchema.h"
#include "FlowAssetEditor.h"
#include "FlowAssetGraph.h"
#include "FlowGraphUtils.h"
#include "Nodes/FlowGraphNode.h"

#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "Nodes/FlowNodeIn.h"

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
	// prevent adding new nodes while playing
	if (GEditor->PlayWorld != nullptr)
	{
		return nullptr;
	}

	check(NodeClass);

	UFlowAsset* FlowAsset = CastChecked<UFlowAssetGraph>(ParentGraph)->GetFlowAsset();
	const FScopedTransaction Transaction(LOCTEXT("FlowEditorNewFlowNode", "Flow Editor: New Flow Node"));
	ParentGraph->Modify();
	FlowAsset->Modify();

	UFlowNode* NewNode = FlowAsset->CreateNode<UFlowNode>(NodeClass, bSelectNewNode);

	NewNode->GetGraphNode()->NodePosX = Location.X;
	NewNode->GetGraphNode()->NodePosY = Location.Y;
	NewNode->GetGraphNode()->AutowireNewNode(FromPin);

	FlowAsset->PostEditChange();
	FlowAsset->MarkPackageDirty();

	return NewNode->GetGraphNode();
}

/////////////////////////////////////////////////////
// FFlowGraphSchemaAction_NewFromSelected

UEdGraphNode* FFlowGraphSchemaAction_NewFromSelected::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode/* = true*/)
{
	// prevent adding new nodes while playing
	if (GEditor->PlayWorld != nullptr)
	{
		return nullptr;
	}

	const FScopedTransaction Transaction(LOCTEXT("FlowEditorNewFromSelection", "Flow Editor: New From Selection"));
	ParentGraph->Modify();

	UFlowAsset* FlowAsset = CastChecked<UFlowAssetGraph>(ParentGraph)->GetFlowAsset();
	FlowAsset->Modify();

	if (NodeClass)
	{
		UFlowNode* NewNode = FlowAsset->CreateNode<UFlowNode>(NodeClass, bSelectNewNode);

		NewNode->GetGraphNode()->NodePosX = Location.X;
		NewNode->GetGraphNode()->NodePosY = Location.Y;
		NewNode->GetGraphNode()->AutowireNewNode(FromPin);

		FlowAsset->PostEditChange();
		FlowAsset->MarkPackageDirty();

		return NewNode->GetGraphNode();
	}

	return nullptr;
}

/////////////////////////////////////////////////////
// FFlowGraphSchemaAction_NewComment

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

/////////////////////////////////////////////////////
// FFlowGraphSchemaAction_Paste

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

void UFlowGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	GetFlowNodeActions(ContextMenuBuilder, FString());
	GetCommentAction(ContextMenuBuilder, ContextMenuBuilder.CurrentGraph);

	if (!ContextMenuBuilder.FromPin && FFlowGraphUtils::GetFlowAssetEditor(ContextMenuBuilder.CurrentGraph)->CanPasteNodes())
	{
		const TSharedPtr<FFlowGraphSchemaAction_Paste> NewAction(new FFlowGraphSchemaAction_Paste(FText::GetEmpty(), LOCTEXT("PasteHereAction", "Paste here"), FText::GetEmpty(), 0));
		ContextMenuBuilder.AddAction(NewAction);
	}
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
		const ECanCreateConnectionResponse ReplyBreakInputs = (OutputPin == PinA ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B);
		return FPinConnectionResponse(ReplyBreakInputs, LOCTEXT("ConnectionReplace", "Replace existing connections"));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
}

bool UFlowGraphSchema::TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	const bool bModified = UEdGraphSchema::TryCreateConnection(PinA, PinB);

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
					if (FlowNode->OutputNames.Num() == 0)
					{
						continue;
					}
					break;
				case EEdGraphPinDirection::EGPD_Output:
					if (FlowNode->InputNames.Num() == 0)
					{
						continue;
					}
					break;
			}
		}

		// filter by category
		if (CategoryName.IsEmpty() || CategoryName.Equals(FlowNode->Category))
		{
			const FText Name = FlowNode->GetTitle();

			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("Name"), Name);
			const FText AddToolTip = FText::Format(LOCTEXT("NewFlowNodeTooltip", "Adds {Name} node here"), Arguments);
			TSharedPtr<FFlowGraphSchemaAction_NewNode> NewNodeAction(new FFlowGraphSchemaAction_NewNode(FText::FromString(FlowNode->Category), Name, AddToolTip, 0));
			ActionMenuBuilder.AddAction(NewNodeAction);
			NewNodeAction->NodeClass = FlowNodeClass;
		}
	}
}

void UFlowGraphSchema::GetCommentAction(FGraphActionMenuBuilder& ActionMenuBuilder, const UEdGraph* CurrentGraph /*= nullptr*/) const
{
	if (!ActionMenuBuilder.FromPin)
	{
		const bool bIsManyNodesSelected = CurrentGraph ? (FFlowGraphUtils::GetFlowAssetEditor(CurrentGraph)->GetNumberOfSelectedNodes() > 0) : false;
		const FText MenuDescription = bIsManyNodesSelected ? LOCTEXT("CreateCommentAction", "Create Comment from Selection") : LOCTEXT("AddCommentAction", "Add Comment...");
		const FText ToolTip = LOCTEXT("CreateCommentToolTip", "Creates a comment.");

		const TSharedPtr<FFlowGraphSchemaAction_NewComment> NewAction(new FFlowGraphSchemaAction_NewComment(FText::GetEmpty(), MenuDescription, ToolTip, 0));
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
			UnsortedCategories.Add(DefaultObject->Category);
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
	return FFlowGraphUtils::GetFlowAssetEditor(Graph)->GetNumberOfSelectedNodes();
}

TSharedPtr<FEdGraphSchemaAction> UFlowGraphSchema::GetCreateCommentAction() const
{
	return TSharedPtr<FEdGraphSchemaAction>(static_cast<FEdGraphSchemaAction*>(new FFlowGraphSchemaAction_NewComment));
}

#undef LOCTEXT_NAMESPACE