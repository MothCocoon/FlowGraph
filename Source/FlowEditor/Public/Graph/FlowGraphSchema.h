#pragma once

#include "EdGraph/EdGraphSchema.h"
#include "FlowGraphSchema.generated.h"

class UEdGraph;

UCLASS(MinimalAPI)
class UFlowGraphSchema : public UEdGraphSchema
{
	GENERATED_UCLASS_BODY()

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

	static void InitFlowNodes();

	static TArray<UClass*> FlowNodeClasses;
	static TArray<TSharedPtr<FString>> FlowNodeCategories;

	static bool bFlowNodesInitialized;
};
