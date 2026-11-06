// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Engine/TimerHandle.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "FastForward/FlowFastForwardRoute.h"
#include "Interfaces/FlowFastForwardGate.h"

#include "FlowFastForwardSubsystem.generated.h"

class UFlowNode;

/**
 * Pushes the signal along a route found by FFlowFastForwardRouteFinder.
 * Nodes on the route receive FastForwardInput instead of ExecuteInput.
 */
UCLASS()
class FLOWDEBUGGER_API UFlowFastForwardSubsystem : public UGameInstanceSubsystem, public IFlowFastForwardGate
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/* Replaces the Fast Forward in progress. */
	void StartFastForward(const FFlowFastForwardRoute& InRoute);
	void StopFastForward();

	bool IsFastForwarding() const { return !Route.IsEmpty(); }

	// IFlowFastForwardGate
	virtual bool ShouldFastForwardInput(const UFlowNode& Node, const FName& PinName, FName& OutOutputPinName) override;
	virtual void OnOutputForced(const UFlowNode& Node, const FName& OutputPinName) override;
	virtual void OnRouteBroken(const UFlowNode& Node, const FName& OutputPinName) override;
	// --

protected:
	FFlowFastForwardRoute Route;

	/* Graph instance of every segment reached so far. */
	TArray<TWeakObjectPtr<UFlowAsset>> SegmentInstances;

	/* Number of forced outputs per node class during this game session. */
	TMap<TWeakObjectPtr<const UClass>, int32> ForcedOutputCounts;

	/* Step expected to receive the signal next. */
	int32 SegmentIndex = INDEX_NONE;
	int32 StepIndex = INDEX_NONE;

	/* Seconds without reaching the next step, after which the stalled Fast Forward is reported. */
	float StallTimeout = 5.0f;

	FTimerHandle StallTimerHandle;

	bool IsExpectedStep(const UFlowNode& Node, const FName& PinName) const;
	bool IsRouteNode(const UFlowNode& Node, const int32 InSegmentIndex, const int32 InStepIndex) const;

	bool IsSegmentInstance(const UFlowAsset* Instance, const int32 InSegmentIndex) const;
	void AdvanceStep();

	/* Reports the node of the remaining route receiving the signal from outside the route. */
	void ReportSignalOutsideRoute(const UFlowNode& Node, const FName& PinName) const;

	void OnStalled();
};
