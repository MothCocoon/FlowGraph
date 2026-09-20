// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Engine/EngineTypes.h"
#include "Nodes/FlowNode.h"
#include "FlowNode_Timer.generated.h"

/**
 * Triggers outputs after time elapsed.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Timer", Keywords = "delay, step, tick"))
class FLOW_API UFlowNode_Timer : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowNode_Timer();

protected:
	/* If the value is closer to 0, Timer will complete in next tick. */
	UPROPERTY(EditAnywhere, Category = "Timer", meta = (ClampMin = 0.0f, DefaultForInputFlowPin, FlowPinType = Float))
	float CompletionTime = 1.0f;

	/* This allows to trigger other nodes multiple times before completing the Timer. */
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
#endif
};
