// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Misc/Guid.h"

#include "Nodes/FlowPin.h"

class UFlowAsset;

struct FFlowDeferredTriggerInput
{
	FGuid NodeGuid;
	FName PinName;
	FConnectedPin FromPin;
};

struct FLOW_API FFlowDeferredTransitionScope
{
public:
	void EnqueueDeferredTrigger(const FFlowDeferredTriggerInput& Entry);
	bool TryFlushDeferredTriggers(UFlowAsset& OwningFlowAsset);

	void CloseScope() { bIsOpen = false; }
	bool IsOpen() const { return bIsOpen; }

	const TArray<FFlowDeferredTriggerInput>& GetDeferredTriggers() const { return DeferredTriggers; }

protected:
	/* Deferred triggers for this scope. */
	TArray<FFlowDeferredTriggerInput> DeferredTriggers;

	/* Is currently accepting new deferred triggers. */
	bool bIsOpen = true;
};
