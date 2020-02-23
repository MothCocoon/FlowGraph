#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "AssetData.h"
#include "EdGraph/EdGraphSchema.h"
#include "FlowGraphSchema.generated.h"

class UEdGraph;
class UFlowNode;

/** Action to add a node to the graph */
USTRUCT()
struct FLOWEDITOR_API FFlowGraphSchemaAction_NewNode : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();

	/** Class of node we want to create */
	UPROPERTY()
	class UClass* NodeClass;

	// Simple type info
	static FName StaticGetTypeId() { static FName Type("FFlowGraphSchemaAction_NewNode"); return Type; }
	virtual FName GetTypeId() const override { return StaticGetTypeId(); }

	FFlowGraphSchemaAction_NewNode() 
		: FEdGraphSchemaAction()
		, NodeClass(nullptr)
	{}

	FFlowGraphSchemaAction_NewNode(FText InNodeCategory, FText InMenuDesc, FText InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping)
		, NodeClass(nullptr)
	{}

	// FEdGraphSchemaAction
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	// --
};

/** Action to add nodes to the graph based on selected objects*/
USTRUCT()
struct FLOWEDITOR_API FFlowGraphSchemaAction_NewFromSelected : public FFlowGraphSchemaAction_NewNode
{
	GENERATED_USTRUCT_BODY();

	FFlowGraphSchemaAction_NewFromSelected() 
		: FFlowGraphSchemaAction_NewNode()
	{}

	FFlowGraphSchemaAction_NewFromSelected(FText InNodeCategory, FText InMenuDesc, FText InToolTip, const int32 InGrouping)
		: FFlowGraphSchemaAction_NewNode(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping) 
	{}

	// FEdGraphSchemaAction
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	// --
};

/** Action to create new comment */
USTRUCT()
struct FLOWEDITOR_API FFlowGraphSchemaAction_NewComment : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();

	// Simple type info
	static FName StaticGetTypeId() { static FName Type("FFlowGraphSchemaAction_NewComment"); return Type; }
	virtual FName GetTypeId() const override { return StaticGetTypeId(); }
	
	FFlowGraphSchemaAction_NewComment() 
		: FEdGraphSchemaAction()
	{}

	FFlowGraphSchemaAction_NewComment(FText InNodeCategory, FText InMenuDesc, FText InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping)
	{}

	// FEdGraphSchemaAction
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	// --
};

/** Action to paste clipboard contents into the graph */
USTRUCT()
struct FLOWEDITOR_API FFlowGraphSchemaAction_Paste : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();

	FFlowGraphSchemaAction_Paste() 
		: FEdGraphSchemaAction()
	{}

	FFlowGraphSchemaAction_Paste(FText InNodeCategory, FText InMenuDesc, FText InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping)
	{}

	// FEdGraphSchemaAction
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	// ==
};

UCLASS(MinimalAPI)
class UFlowGraphSchema : public UEdGraphSchema
{
	GENERATED_UCLASS_BODY()

public:
	FLOWEDITOR_API void GetPaletteActions(FGraphActionMenuBuilder& ActionMenuBuilder, const FString& CategoryName) const;

	// EdGraphSchema
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const override;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;
	virtual bool ShouldHidePinDefaultValue(UEdGraphPin* Pin) const override;
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;
	virtual void BreakNodeLinks(UEdGraphNode& TargetNode) const override;
	virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const override;
	virtual int32 GetNodeSelectionCount(const UEdGraph* Graph) const override;
	virtual TSharedPtr<FEdGraphSchemaAction> GetCreateCommentAction() const override;
	// --

	static TArray<TSharedPtr<FString>> GetFlowNodeCategories();

private:
	void GetFlowNodeActions(FGraphActionMenuBuilder& ActionMenuBuilder, const FString& CategoryName) const;
	void GetCommentAction(FGraphActionMenuBuilder& ActionMenuBuilder, const UEdGraph* CurrentGraph = nullptr) const;

private:
	static void InitFlowNodes();

	static TArray<UClass*> FlowNodeClasses;
	static TArray<TSharedPtr<FString>> FlowNodeCategories;

	static bool bFlowNodesInitialized;
};