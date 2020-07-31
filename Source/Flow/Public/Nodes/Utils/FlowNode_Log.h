#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_Log.generated.h"

/**
 * Log
 */
UCLASS(meta = (DisplayName = "Log"))
class FLOW_API UFlowNode_Log : public UFlowNode
{
	GENERATED_UCLASS_BODY()
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "Flow")
	FString Message;

	UPROPERTY(EditDefaultsOnly, Category = "Flow")
	bool bPrintToScreen;

	UPROPERTY(EditDefaultsOnly, Category = "Flow", meta = (EditCondition = "bPrintToScreen"))
	float Duration;

	UPROPERTY(EditDefaultsOnly, Category = "Flow", meta = (EditCondition = "bPrintToScreen"))
	FColor TextColor;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
#endif
};
