#pragma once

#include "FlowGraphNode.h"
#include "FlowGraphNode_Reroute.generated.h"

UCLASS()
class FLOWEDITOR_API UFlowGraphNode_Reroute : public UFlowGraphNode
{
	GENERATED_UCLASS_BODY()

public:
	// UEdGraphNode
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	virtual bool ShouldDrawNodeAsControlPointOnly(int32& OutInputPinIndex, int32& OutOutputPinIndex) const override;
	// --
};