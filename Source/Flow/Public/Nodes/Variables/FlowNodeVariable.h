// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"
#include "FlowNodeVariable.generated.h"

/**
 * A Flow Node Variable is similar to Flow Node with support for exposed Flow Property Bag variable.
 */
UCLASS(Abstract, Blueprintable)
class FLOW_API UFlowNodeVariable : public UFlowNode
{
	GENERATED_BODY()
	
protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn))
	FFlowPropertyBag Property;

public:

	UFlowNodeVariable();
};
