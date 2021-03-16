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
	class UClass* NativeNodeClass;

	UPROPERTY()
	class UBlueprint* Blueprint;

	static FName StaticGetTypeId()
	{
		static FName Type("FFlowGraphSchemaAction_NewNode");
		return Type;
	}

	virtual FName GetTypeId() const override { return StaticGetTypeId(); }

	FFlowGraphSchemaAction_NewNode()
		: FEdGraphSchemaAction()
		, NativeNodeClass(nullptr)
		, Blueprint(nullptr)
	{
	}

	FFlowGraphSchemaAction_NewNode(UClass* Node)
		: FEdGraphSchemaAction()
		, NativeNodeClass(Node)
		, Blueprint(nullptr)
	{
	}

	FFlowGraphSchemaAction_NewNode(const UFlowNode* Node)
		: FEdGraphSchemaAction(FText::FromString(Node->GetNativeCategory()), Node->GetTitle(), Node->GetClass()->GetToolTipText(), 0)
		, NativeNodeClass(Node->GetClass())
		, Blueprint(nullptr)
	{
	}

	FFlowGraphSchemaAction_NewNode(UBlueprint* InBlueprint)
        : FEdGraphSchemaAction(FText::FromString(InBlueprint->BlueprintCategory), FText::FromString(InBlueprint->BlueprintDisplayName), FText::FromString(InBlueprint->BlueprintDescription), 0)
		, NativeNodeClass(nullptr)
		, Blueprint(InBlueprint)
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
