// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Engine/EngineTypes.h"
#include "Nodes/FlowNode.h"
#include "FlowNode_Timer.generated.h"

/**
 * Waits for CompletionTime seconds then fires the Completed output.
 * Optionally fires the Step output repeatedly every StepTime seconds during the wait
 * CompletionTime can be overridden via a data pin at runtime.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Timer", Keywords = "delay step tick"))
class FLOW_API UFlowNode_Timer : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowNode_Timer();

protected:
	/* Total duration in seconds before the Completed output fires.
	 * Values near 0 complete on the next tick.
	 * Overridable via input data pin. */
	UPROPERTY(EditAnywhere, Category = "Timer", meta = (ClampMin = 0.0f, DefaultForInputFlowPin, FlowPinType = Float))
	float CompletionTime = 1.0f;

	/* If > 0, fires the Step output every StepTime seconds while waiting.
	 * Set to 0 to disable periodic steps. */
	UPROPERTY(EditAnywhere, Category = "Timer", meta = (ClampMin = 0.0f))
	float StepTime = 0.0f;

	static FName INPIN_CompletionTime;

private:
	FTimerHandle CompletionTimerHandle;
	FTimerHandle StepTimerHandle;

	UPROPERTY(SaveGame)
	float ResolvedCompletionTime = 0.0f;

	UPROPERTY(SaveGame)
	float SumOfSteps = 0.0f;

	UPROPERTY(SaveGame)
	float RemainingCompletionTime = 0.0f;

	UPROPERTY(SaveGame)
	float RemainingStepTime = 0.0f;

public:
	virtual void InitializeInstance() override;
	virtual void ExecuteInput(const FName& PinName) override;

protected:
	virtual void SetTimer();
	virtual void Restart();

	float ResolveCompletionTime() const;

private:
	UFUNCTION()
	void OnStep();

	UFUNCTION()
	void OnCompletion();

public:
	virtual void Cleanup() override;

	virtual void OnSave_Implementation() override;
	virtual void OnLoad_Implementation() override;

#if WITH_EDITOR
public:
	virtual FString GetStatusString() const override;
	virtual void UpdateNodeConfigText_Implementation() override;
	virtual const FFlowAgentDoc& GetAgentDoc() const override;
#endif
};
