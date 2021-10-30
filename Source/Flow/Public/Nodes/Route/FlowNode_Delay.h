// Created by Satheesh (ryanjon2040)

#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_Delay.generated.h"

/**
* Delays execution for given seconds
*/
UCLASS(NotBlueprintable, meta = (DisplayName = "Delay"))
class FLOW_API UFlowNode_Delay final : public UFlowNode
{
	GENERATED_BODY()

private:

	FTimerHandle TimerHandle_Delay;

protected:
	/** Executes 'Finished' after given seconds. */
	UPROPERTY(EditAnywhere, Category = "Delay", meta = (UIMin = 0, ClampMin = 0, UIMax = 60))
	float DelayInSeconds;

public:

	UFlowNode_Delay();

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

#if WITH_EDITOR
	virtual FString GetNodeDescription() const override;
	virtual FString GetStatusString() const override;
#endif
};
