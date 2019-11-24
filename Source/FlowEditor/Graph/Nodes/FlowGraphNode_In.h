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
	// UEdGraphNode interface
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual bool CanUserDeleteNode() const override { return false; }
	virtual bool CanDuplicateNode() const override { return false; }
	// End of UEdGraphNode interface
};
