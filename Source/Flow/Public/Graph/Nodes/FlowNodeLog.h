#pragma once

#include "FlowNode.h"
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

protected:
	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
public:
	virtual FString GetShortDescription() const override;
#endif
};
