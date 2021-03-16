#pragma once

#include "Engine/EngineTypes.h"
#include "Nodes/FlowNode.h"
#include "FlowNode_Timer.generated.h"

/**
 * Triggers outputs after time elapsed
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Timer"))
class FLOW_API UFlowNode_Timer final : public UFlowNode
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Timer", meta = (ClampMin = 0.0f))
	float CompletionTime;

	// this allows to trigger other nodes multiple times before completing the Timer
	UPROPERTY(EditDefaultsOnly, Category = "Timer", meta = (ClampMin = 0.0f))
	float StepTime;

private:
	FTimerHandle CompletionTimerHandle;

	FTimerHandle StepTimerHandle;
	float SumOfSteps;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

	UFUNCTION()
	void OnStep();

	UFUNCTION()
	void OnCompletion();

	virtual void Cleanup() override;

#if WITH_EDITOR
	virtual FString GetNodeDescription() const override;
	virtual FString GetStatusString() const override;
#endif
};
