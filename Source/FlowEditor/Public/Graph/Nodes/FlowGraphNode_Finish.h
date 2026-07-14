// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Graph/Nodes/FlowGraphNode.h"
#include "FlowGraphNode_Finish.generated.h"

UCLASS()
class FLOWEDITOR_API UFlowGraphNode_Finish : public UFlowGraphNode
{
	GENERATED_BODY()

public:
	UFlowGraphNode_Finish();

	// UEdGraphNode
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	// --
};
