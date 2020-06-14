#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNodeLog.generated.h"

/**
 * Log
 */
UCLASS(meta = (DisplayName = "Log"))
class FLOW_API UFlowNodeLog : public UFlowNode
{
	GENERATED_UCLASS_BODY()
	
private:
	UPROPERTY(EditAnywhere, Category = "Flow")
	FString Message;

	UPROPERTY(EditAnywhere, Category = "Flow")
	bool bPrintToScreen;

	UPROPERTY(EditAnywhere, Category = "Flow", meta = (EditCondition = "bPrintToScreen"))
	float Duration;

	UPROPERTY(EditAnywhere, Category = "Flow", meta = (EditCondition = "bPrintToScreen"))
	FColor TextColor;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
#endif
};
