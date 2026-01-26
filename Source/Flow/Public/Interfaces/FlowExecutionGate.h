// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "CoreMinimal.h"

class UFlowAsset;

/**
 * Implemented by a debugger/runtime system (in another module) that can halt Flow execution.
 * Flow runtime queries this through FFlowExecutionGate without depending on the debugger module.
 */
class FLOW_API IFlowExecutionGate
{
public:
	virtual ~IFlowExecutionGate() = default;

	/** Return true when Flow execution should be halted globally. */
	virtual bool IsFlowExecutionHalted() const = 0;
};

/**
 * Global registry + minimal deferred-execution queue for Flow runtime.
 */
class FLOW_API FFlowExecutionGate
{
public:
	static void SetGate(IFlowExecutionGate* InGate);
	static IFlowExecutionGate* GetGate();

	/** True if a gate exists and it currently wants Flow execution halted. */
	static bool IsHalted();

	/** If halted, queues the trigger for later. Returns true if queued (caller should early-out). */
	static bool EnqueueDeferredTriggerInput(UFlowAsset* FlowAssetInstance, const FGuid& NodeGuid, const FName& PinName, const struct FConnectedPin& FromPin);

	/**
	 * Flushes queued trigger inputs (FIFO).
	 * Safe to call even if nothing is queued.
	 */
	static void FlushDeferredTriggerInputs();

private:
	static IFlowExecutionGate* Gate;
};