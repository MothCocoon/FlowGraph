// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Policies/FlowPreloadHelper.h"

#include "AddOns/FlowNodeAddOn.h"
#include "Interfaces/FlowPreloadableInterface.h"
#include "FlowAsset.h"
#include "Policies/FlowPreloadPolicy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowPreloadHelper)

const FFlowPin FFlowPreloadHelper::OUTPIN_AllPreloadsComplete(TEXT("All Preloads Complete"));

const FFlowPin FFlowPreloadHelper_Standard::INPIN_PreloadContent(TEXT("Preload Content"));
const FFlowPin FFlowPreloadHelper_Standard::INPIN_FlushContent(TEXT("Flush Content"));

void FFlowPreloadHelper_Standard::TriggerPreload(UFlowNode& Node)
{
	FLOW_ASSERT_ENUM_MAX(EFlowPreloadResult, 2);

	if (bContentPreloaded || PendingPreloadCount > 0)
	{
		return;
	}

	// Count all preloadable participants (node + addons) before calling any PreloadContent.
	// PendingPreloadCount must be fully set before the first call so that re-entrant
	// NotifyPreloadComplete() (e.g. sync FStreamableManager) sees the correct total.
	const bool bNodePreloadable = Cast<IFlowPreloadableInterface>(&Node) != nullptr;
	if (bNodePreloadable)
	{
		++PendingPreloadCount;
	}

	Node.ForEachAddOnForClass<UFlowPreloadableInterface>([this](UFlowNodeAddOn& /*AddOn*/)
	{
		++PendingPreloadCount;
		return EFlowForEachAddOnFunctionReturnValue::Continue;
	});

	if (PendingPreloadCount == 0)
	{
		return;
	}

	// Trigger the node itself.
	if (bNodePreloadable)
	{
		if (Cast<IFlowPreloadableInterface>(&Node)->PreloadContent() == EFlowPreloadResult::Completed)
		{
			Node.NotifyPreloadComplete();
		}
	}

	// Trigger each preloadable addon.
	// PreloadInProgress addons must call NotifyPreloadComplete() on themselves when done.
	Node.ForEachAddOnForClass<UFlowPreloadableInterface>([&Node](UFlowNodeAddOn& AddOn)
	{
		IFlowPreloadableInterface* Preloadable = CastChecked<IFlowPreloadableInterface>(&AddOn);
		if (Preloadable->PreloadContent() == EFlowPreloadResult::Completed)
		{
			Node.NotifyPreloadComplete();
		}
		return EFlowForEachAddOnFunctionReturnValue::Continue;
	});
}

void FFlowPreloadHelper_Standard::TriggerFlush(UFlowNode& Node)
{
	// Reset pending count first. Any late-arriving PreloadInProgress NotifyPreloadComplete()
	// will be rejected by the PendingPreloadCount <= 0 guard in OnPreloadComplete.
	PendingPreloadCount = 0;

	if (bContentPreloaded)
	{
		bContentPreloaded = false;

		if (IFlowPreloadableInterface* Preloadable = Cast<IFlowPreloadableInterface>(&Node))
		{
			Preloadable->FlushContent();
		}

		Node.ForEachAddOnForClass<UFlowPreloadableInterface>([](UFlowNodeAddOn& AddOn)
		{
			CastChecked<IFlowPreloadableInterface>(&AddOn)->FlushContent();
			return EFlowForEachAddOnFunctionReturnValue::Continue;
		});
	}
}

EFlowPreloadResult FFlowPreloadHelper_Standard::OnPreloadComplete(UFlowNode& /*Node*/)
{
	FLOW_ASSERT_ENUM_MAX(EFlowPreloadResult, 2);

	if (PendingPreloadCount <= 0)
	{
		// Guard: TriggerFlush was called, or this is a spurious/duplicate call. Discard.
		return EFlowPreloadResult::PreloadInProgress;
	}

	--PendingPreloadCount;

	if (PendingPreloadCount > 0)
	{
		// Still waiting on other participants (addons or the node itself).
		return EFlowPreloadResult::PreloadInProgress;
	}

	bContentPreloaded = true;
	return EFlowPreloadResult::Completed;
}

void FFlowPreloadHelper_Standard::OnNodeActivate(UFlowNode& Node)
{
	FLOW_ASSERT_ENUM_MAX(EFlowPreloadTiming, 3);

	if (const UFlowAsset* FlowAsset = Node.GetFlowAsset())
	{
		const FFlowPreloadPolicy& Policy = FlowAsset->GetPreloadPolicy();
		if (Policy.GetPreloadTimingForNode(Node) == EFlowPreloadTiming::OnActivate)
		{
			TriggerPreload(Node);
		}
	}
}

void FFlowPreloadHelper_Standard::OnNodeInitializeInstance(UFlowNode& Node)
{
	FLOW_ASSERT_ENUM_MAX(EFlowPreloadTiming, 3);

	if (const UFlowAsset* FlowAsset = Node.GetFlowAsset())
	{
		const FFlowPreloadPolicy& Policy = FlowAsset->GetPreloadPolicy();
		if (Policy.GetPreloadTimingForNode(Node) == EFlowPreloadTiming::OnGraphInitialize)
		{
			TriggerPreload(Node);
		}
	}
}

void FFlowPreloadHelper_Standard::OnNodeCleanup(UFlowNode& Node)
{
	FLOW_ASSERT_ENUM_MAX(EFlowFlushTiming, 3);

	if (const UFlowAsset* FlowAsset = Node.GetFlowAsset())
	{
		const FFlowPreloadPolicy& Policy = FlowAsset->GetPreloadPolicy();
		if (Policy.GetFlushTimingForNode(Node) == EFlowFlushTiming::OnNodeFinish)
		{
			TriggerFlush(Node);
		}
	}
}

void FFlowPreloadHelper_Standard::OnNodeDeinitializeInstance(UFlowNode& Node)
{
	FLOW_ASSERT_ENUM_MAX(EFlowFlushTiming, 3);

	if (const UFlowAsset* FlowAsset = Node.GetFlowAsset())
	{
		const FFlowPreloadPolicy& Policy = FlowAsset->GetPreloadPolicy();
		if (Policy.GetFlushTimingForNode(Node) != EFlowFlushTiming::ManualOnly)
		{
			// Flush regardless of specific timing (safety net for OnNodeFinish
			// where content may still be loaded at graph teardown). TriggerFlush is idempotent.
			TriggerFlush(Node);
		}
	}
}

EFlowPreloadInputResult FFlowPreloadHelper_Standard::OnNodeExecuteInput(UFlowNode& Node, const FName& PinName)
{
	FLOW_ASSERT_ENUM_MAX(EFlowPreloadInputResult, 2);

	if (PinName == INPIN_PreloadContent.PinName)
	{
		TriggerPreload(Node);
		return EFlowPreloadInputResult::Handled;
	}
	else if (PinName == INPIN_FlushContent.PinName)
	{
		TriggerFlush(Node);
		return EFlowPreloadInputResult::Handled;
	}

	return EFlowPreloadInputResult::Unhandled;
}

#if WITH_EDITOR
void FFlowPreloadHelper_Standard::GetContextInputs(TArray<FFlowPin>& OutInputPins) const
{
	OutInputPins.AddUnique(INPIN_PreloadContent);
	OutInputPins.AddUnique(INPIN_FlushContent);
}

void FFlowPreloadHelper::GetContextOutputs(TArray<FFlowPin>& OutOutputPins) const
{
	OutOutputPins.AddUnique(OUTPIN_AllPreloadsComplete);
}
#endif
