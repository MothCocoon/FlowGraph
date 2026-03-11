// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "UObject/Interface.h"

class UFlowAsset;

/**
 * Implemented by a debugger/runtime system (in another module) that can halt Flow execution.
 * Flow runtime queries this through FFlowExecutionGate without depending on the debugger module.
 */
class FLOW_API IFlowExecutionGate
{
public:
	virtual ~IFlowExecutionGate() = default;

	/* Return true when Flow execution should be halted globally. */
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

	/* True if a gate exists and it currently wants Flow execution halted. */
	static bool IsHalted();

private:
	static IFlowExecutionGate* Gate;
};