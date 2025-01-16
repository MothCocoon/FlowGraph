// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "EdGraph/EdGraphSchema.h"

#include "Nodes/FlowGraphNode.h"
#include "Nodes/FlowNode.h"
#include "FlowGraphSchema_Actions.generated.h"

class UFlowGraphSettings;

/** Action to add a node to the graph */
USTRUCT()
struct FLOWEDITOR_API FFlowGraphSchemaAction_NewNode : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TObjectPtr<class UClass> NodeClass;

	static FName StaticGetTypeId()
	{
		static FName Type("FFlowGraphSchemaAction_NewNode");
		return Type;
	}

	virtual FName GetTypeId() const override { return StaticGetTypeId(); }

	FFlowGraphSchemaAction_NewNode()
		: NodeClass(nullptr)
	{
	}

	explicit FFlowGraphSchemaAction_NewNode(UClass* InNodeClass)
		: NodeClass(InNodeClass)
	{
	}

	explicit FFlowGraphSchemaAction_NewNode(const UFlowNodeBase* Node, const UFlowGraphSettings& GraphSettings)
		: FEdGraphSchemaAction(GetNodeCategory(Node, GraphSettings), Node->GetNodeTitle(), Node->GetNodeToolTip(), 0, FText::FromString(Node->GetClass()->GetMetaData("Keywords")))
		, NodeClass(Node->GetClass())
	{
	}

	// FEdGraphSchemaAction
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	// --

	static UFlowGraphNode* CreateNode(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const UClass* NodeClass, const FVector2D Location, const bool bSelectNewNode = true);
	static UFlowGraphNode* RecreateNode(UEdGraph* ParentGraph, UEdGraphNode* OldInstance, UFlowNode* FlowNode);
	static UFlowGraphNode* ImportNode(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const UClass* NodeClass, const FGuid& NodeGuid, const FVector2D Location);

private:
	static FText GetNodeCategory(const UFlowNodeBase* Node, const UFlowGraphSettings& GraphSettings);
};

/** Action to add a subnode to the selected node */
USTRUCT()
struct FLOWEDITOR_API FFlowSchemaAction_NewSubNode : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();

	/** Template of node we want to create */
	UPROPERTY()
	TObjectPtr<UFlowGraphNode> NodeTemplate;

	/** parent node */
	UPROPERTY()
	TObjectPtr<UFlowGraphNode> ParentNode;

	FFlowSchemaAction_NewSubNode()
		: FEdGraphSchemaAction()
		, NodeTemplate(nullptr)
		, ParentNode(nullptr)
	{
	}

	FFlowSchemaAction_NewSubNode(FText InNodeCategory, FText InMenuDesc, FText InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping)
		, NodeTemplate(nullptr)
		, ParentNode(nullptr)
	{
	}

	// FEdGraphSchemaAction
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, const FVector2D Location, bool bSelectNewNode = true) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	// --

	static UFlowGraphNode* RecreateNode(UEdGraph* ParentGraph, UEdGraphNode* OldInstance, UFlowGraphNode* ParentFlowGraphNode, UFlowNodeAddOn* FlowNodeAddOn);

	static TSharedPtr<FFlowSchemaAction_NewSubNode> AddNewSubNodeAction(FGraphActionListBuilderBase& ContextMenuBuilder, const FText& Category, const FText& MenuDesc, const FText& Tooltip);
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
