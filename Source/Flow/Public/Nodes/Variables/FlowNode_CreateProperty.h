// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"
#include "FlowNode_CreateProperty.generated.h"

/**
 * 
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Create Property", Keywords = "make, variable"))
class FLOW_API UFlowNode_CreateProperty : public UFlowNode
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn))
	FFlowPropertyBag Property;

public:

	UFlowNode_CreateProperty();

protected:
	
	virtual void ExecuteInput(const FName& PinName) override;
};
