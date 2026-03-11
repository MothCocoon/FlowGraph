// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowDeferredTransitionScope.h"
#include "FlowAsset.h"
#include "Interfaces/FlowExecutionGate.h"

void FFlowDeferredTransitionScope::EnqueueDeferredTrigger(const FFlowDeferredTriggerInput& Entry)
{
	check(bIsOpen);

	DeferredTriggers.Add(Entry);
}

bool FFlowDeferredTransitionScope::TryFlushDeferredTriggers(UFlowAsset& OwningFlowAsset)
{
	// Ensure the scope is closed before beginning flushing
	CloseScope();

	// Remove and trigger each deferred trigger input
	while (!DeferredTriggers.IsEmpty() && !FFlowExecutionGate::IsHalted())
	{
		const FFlowDeferredTriggerInput Entry = DeferredTriggers[0];
		DeferredTriggers.RemoveAt(0, 1, EAllowShrinking::No);

		OwningFlowAsset.TriggerInput(Entry.NodeGuid, Entry.PinName, Entry.FromPin);
	}

	check(DeferredTriggers.IsEmpty() || FFlowExecutionGate::IsHalted());

	// Return true if everything flushed without being interrupted by an ExecutionGate
	return DeferredTriggers.IsEmpty();
}