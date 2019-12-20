#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "FlowGraphNode.h"
#include "FlowGraphNode_In.generated.h"

UCLASS(MinimalAPI)
class UFlowGraphNode_In : public UFlowGraphNode
{
	GENERATED_UCLASS_BODY()

public:
	// UEdGraphNode
	virtual FText GetTooltipText() const override;
	virtual bool CanUserDeleteNode() const override { return false; }
	virtual bool CanDuplicateNode() const override { return false; }
	// --
};
