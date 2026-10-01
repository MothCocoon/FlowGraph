// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "UObject/NameTypes.h"

class UFlowNode;

/* The point in a node's execution that a break was requested from. */
enum class EFlowBreakSite : uint8
{
	/* An input pin was triggered, but the node has not executed that input yet. */
	InputTriggered,

	/* An output pin was triggered, but the signal has not reached the connected node yet. */
	OutputTriggered,
};

/* How halted Flow execution should proceed once it is released. */
enum class EFlowBreakAction : uint8
{
	/* Resume inline, exactly where the break happened. */
	Continue,

	/* Unwind without executing the input or propagating the output. */
	Abort,
};

/* Identifies where execution wants to break. Cheap to copy; valid only for the duration of a break. */
struct FFlowBreakContext
{
	/* The node whose pin was triggered. */
	const UFlowNode* Node = nullptr;

	/* The pin that was triggered. */
	FName PinName;

	EFlowBreakSite Site = EFlowBreakSite::InputTriggered;
};

/**
 * Implemented by a debugger system (in another module) that can halt Flow execution in place.
 * Flow runtime reaches this through FFlowExecutionGate without depending on the debugger module.
 */
class FLOW_API IFlowExecutionGate
{
public:
	virtual ~IFlowExecutionGate() = default;

	/* Tests whether execution should break for this context. Must be cheap and free of side effects. */
	virtual bool WantsFlowBreak(const FFlowBreakContext& Context) const = 0;

	/* Halts the calling thread until the user releases it, then reports how execution should proceed.
	 * Implementations must keep the process responsive while halted rather than spinning. */
	virtual EFlowBreakAction BreakFlowExecution(const FFlowBreakContext& Context) = 0;
};

/**
 * Global registry for the Flow execution gate, and the single entry point Flow runtime breaks through.
 */
class FLOW_API FFlowExecutionGate
{
public:
	static void SetGate(IFlowExecutionGate* InGate);
	static IFlowExecutionGate* GetGate();

	/* Halts inline if a gate is registered and wants to break here, otherwise returns immediately.
	 * Blocks for as long as the user leaves execution halted, keeping the calling stack alive so that
	 * execution resumes at the break rather than being replayed from a later frame. */
	static EFlowBreakAction BreakIfRequested(const FFlowBreakContext& Context);

	/* True while a break is halting execution somewhere below the current call stack. */
	static bool IsBreaking() { return bIsBreaking; }

private:
	static IFlowExecutionGate* Gate;

	static bool bIsBreaking;
};
