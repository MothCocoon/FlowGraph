// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "UObject/NameTypes.h"

class UFlowNode;

/**
 * Implemented by a system (in another module) that fast forwards the game along a route of the signal.
 * Flow runtime reaches this through FFlowFastForwardGate without depending on the module of this system.
 */
class FLOW_API IFlowFastForwardGate
{
public:
	virtual ~IFlowFastForwardGate() = default;

	/* Returns true if the node should fast forward the triggered input instead of executing it.
	 * OutOutputPinName is the output expected to pass the signal further. */
	virtual bool ShouldFastForwardInput(const UFlowNode& Node, const FName& PinName, FName& OutOutputPinName) = 0;

	/* Called when the node didn't trigger the expected output by itself. */
	virtual void OnOutputForced(const UFlowNode& Node, const FName& OutputPinName) = 0;

	/* Called when the node finished without triggering the expected output. */
	virtual void OnRouteBroken(const UFlowNode& Node, const FName& OutputPinName) = 0;
};

/**
 * Global registry for the Flow Fast Forward gate.
 */
class FLOW_API FFlowFastForwardGate
{
public:
	static void SetGate(IFlowFastForwardGate* InGate);
	static IFlowFastForwardGate* GetGate();

	/* False if no gate is registered. */
	static bool ShouldFastForwardInput(const UFlowNode& Node, const FName& PinName, FName& OutOutputPinName);

	static void ReportForcedOutput(const UFlowNode& Node, const FName& OutputPinName);
	static void ReportBrokenRoute(const UFlowNode& Node, const FName& OutputPinName);

private:
	static IFlowFastForwardGate* Gate;
};
