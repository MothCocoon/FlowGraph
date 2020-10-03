#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_CustomOutput.generated.h"

/**
 * Custom Output
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Custom Output"))
class FLOW_API UFlowNode_CustomOutput final : public UFlowNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FName EventName;
	
protected:
	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
	virtual FString GetNodeDescription() const override;
#endif
};
