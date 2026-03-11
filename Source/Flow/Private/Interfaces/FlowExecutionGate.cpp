// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Interfaces/FlowExecutionGate.h"

#include "FlowAsset.h"
#include "Nodes/FlowPin.h"

namespace FlowExecutionGate_Private
{
	struct FDeferredTriggerInput
	{
		TWeakObjectPtr<UFlowAsset> FlowAssetInstance;
		FGuid NodeGuid;
		FName PinName;
		FConnectedPin FromPin;
	};

	static TArray<FDeferredTriggerInput> DeferredTriggerInputs;
	static bool bIsFlushing = false;
}

IFlowExecutionGate* FFlowExecutionGate::Gate = nullptr;

void FFlowExecutionGate::SetGate(IFlowExecutionGate* InGate)
{
	Gate = InGate;
}

IFlowExecutionGate* FFlowExecutionGate::GetGate()
{
	return Gate;
}

bool FFlowExecutionGate::IsHalted()
{
	return (Gate != nullptr) && Gate->IsFlowExecutionHalted();
}

bool FFlowExecutionGate::EnqueueDeferredTriggerInput(UFlowAsset* FlowAssetInstance, const FGuid& NodeGuid, const FName& PinName, const FConnectedPin& FromPin)
{
	using namespace FlowExecutionGate_Private;

	// If we're halted, always enqueue (even during flushing). The whole point is to stop propagation.
	if (IsHalted())
	{
		if (!IsValid(FlowAssetInstance))
		{
			return true; // treat as handled while halted
		}

		FDeferredTriggerInput& Entry = DeferredTriggerInputs.AddDefaulted_GetRef();
		Entry.FlowAssetInstance = FlowAssetInstance;
		Entry.NodeGuid = NodeGuid;
		Entry.PinName = PinName;
		Entry.FromPin = FromPin;

		return true;
	}

	// Not halted:
	// During flush we must not enqueue "normal" triggers (we want them to execute now),
	// otherwise we can get infinite deferral.
	if (bIsFlushing)
	{
		return false;
	}

	return false;
}

void FFlowExecutionGate::FlushDeferredTriggerInputs()
{
	using namespace FlowExecutionGate_Private;

	if (bIsFlushing)
	{
		return;
	}

	// Do not flush while halted; callers should clear the halt first.
	if (IsHalted())
	{
		return;
	}

	if (DeferredTriggerInputs.IsEmpty())
	{
		return;
	}

	bIsFlushing = true;

	// Move into a local array so new deferred triggers can be added while we flush.
	TArray<FDeferredTriggerInput> Local = MoveTemp(DeferredTriggerInputs);
	DeferredTriggerInputs.Reset();

	for (int32 Index = 0; Index < Local.Num(); ++Index)
	{
		// If a breakpoint was hit during this flush, stop immediately and re-queue remaining work.
		if (IsHalted())
		{
			const int32 Remaining = Local.Num() - Index;
			if (Remaining > 0)
			{
				TArray<FDeferredTriggerInput> RemainingItems;
				RemainingItems.Reserve(Remaining);

				for (int32 j = Index; j < Local.Num(); ++j)
				{
					RemainingItems.Add(Local[j]);
				}

				// RemainingItems should run before any items that may already be queued.
				if (!DeferredTriggerInputs.IsEmpty())
				{
					RemainingItems.Append(MoveTemp(DeferredTriggerInputs));
				}

				DeferredTriggerInputs = MoveTemp(RemainingItems);
			}

			bIsFlushing = false;
			return;
		}

		const FDeferredTriggerInput& Entry = Local[Index];
		if (UFlowAsset* Asset = Entry.FlowAssetInstance.Get())
		{
			Asset->TriggerDeferredInputFromDebugger(Entry.NodeGuid, Entry.PinName, Entry.FromPin);
		}
	}

	bIsFlushing = false;
}