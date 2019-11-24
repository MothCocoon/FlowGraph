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
	// UEdGraphNode interface
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	// End of UEdGraphNode interface
};
