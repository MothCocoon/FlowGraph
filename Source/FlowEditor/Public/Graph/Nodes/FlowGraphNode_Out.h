#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "FlowGraphNode.h"
#include "FlowGraphNode_Out.generated.h"

UCLASS(MinimalAPI)
class UFlowGraphNode_Out : public UFlowGraphNode
{
	GENERATED_UCLASS_BODY()

public:
	// UEdGraphNode
	virtual FText GetTooltipText() const override;
	// --
};
