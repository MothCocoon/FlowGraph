// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"
#include "FlowNode_DelayUntilNextTick.generated.h"

/**
 * Triggers output next frame
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Delay until next tick", Keywords = "frame"))
class FLOW_API UFlowNode_DelayUntilNextTick : public UFlowNode
{
	GENERATED_BODY()

public:

	UFlowNode_DelayUntilNextTick();

protected:

	virtual void ExecuteInput(const FName& PinName) override;

private:

	UFUNCTION()
	void OnCompletion();
};
