// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "EdGraph/EdGraphSchema.h"

#include "Nodes/FlowGraphNode.h"
#include "Nodes/FlowNode.h"
#include "FlowGraphSchema_Actions.generated.h"

/** Action to add a node to the graph */
USTRUCT()
struct FLOWEDITOR_API FFlowGraphSchemaAction_NewNode : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	class UClass* NodeClass;

	static FName StaticGetTypeId()
	{
		static FName Type("FFlowGraphSchemaAction_NewNode");
		return Type;
	}

	virtual FName GetTypeId() const override { return StaticGetTypeId(); }

	FFlowGraphSchemaAction_NewNode()
		: FEdGraphSchemaAction()
		, NodeClass(nullptr)
	{
	}

	FFlowGraphSchemaAction_NewNode(UClass* Node)
		: FEdGraphSchemaAction()
		, NodeClass(Node)
	{
	}

	FFlowGraphSchemaAction_NewNode(const UFlowNode* Node)
		: FEdGraphSchemaAction(FText::FromString(Node->GetNodeCategory()), Node->GetNodeTitle(), Node->GetNodeToolTip(), 0)
		, NodeClass(Node->GetClass())
	{
	}

	// FEdGraphSchemaAction
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	// --

	static UFlowGraphNode* CreateNode(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, UClass* NodeClass, const FVector2D Location, const bool bSelectNewNode = true);
};

/** Action to paste clipboard contents into the graph */
USTRUCT()
struct FLOWEDITOR_API FFlowGraphSchemaAction_Paste : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY()

	FFlowGraphSchemaAction_Paste()
		: FEdGraphSchemaAction()
	{
	}

	FFlowGraphSchemaAction_Paste(FText InNodeCategory, FText InMenuDesc, FText InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping)
	{
	}

	// FEdGraphSchemaAction
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	// --
};

/** Action to create new comment */
USTRUCT()
struct FLOWEDITOR_API FFlowGraphSchemaAction_NewComment : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY()

	// Simple type info
	static FName StaticGetTypeId()
	{
		static FName Type("FFlowGraphSchemaAction_NewComment");
		return Type;
	}

	virtual FName GetTypeId() const override { return StaticGetTypeId(); }

	FFlowGraphSchemaAction_NewComment()
		: FEdGraphSchemaAction()
	{
	}

	FFlowGraphSchemaAction_NewComment(FText InNodeCategory, FText InMenuDesc, FText InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping)
	{
	}

	// FEdGraphSchemaAction
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	// --
};
