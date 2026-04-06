// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/FlowPin.h"
#include "Policies/FlowPreloadPolicyEnums.h"

#include "FlowPreloadHelper.generated.h"

class UFlowNode;

/**
 * Base preload helper struct. Nodes implementing IFlowPreloadableInterface receive one,
 * as do nodes whose AddOns implement IFlowPreloadableInterface.
 * Non-preloadable nodes (with no preloadable addons) leave PreloadHelper uninitialized (invalid).
 * The base implementation is a no-op: it answers all API calls but performs no preloading.
 *
 * The concrete instance type is determined by FFlowPreloadPolicy::GetPreloadHelperStructType(),
 * typically FFlowPreloadHelper_Standard. Projects may supply their own subclass via a custom
 * FFlowPreloadPolicy subclass.
 */
USTRUCT()
struct FLOW_API FFlowPreloadHelper
{
	GENERATED_BODY()

	virtual ~FFlowPreloadHelper() = default;

	// IFlowCoreExecutableInterface pass-thrus
	virtual void OnNodeInitializeInstance(UFlowNode& Node) PURE_VIRTUAL(OnNodeInitializeInstance);
	virtual void OnNodeActivate(UFlowNode& Node) PURE_VIRTUAL(OnNodeActivate);
	virtual void OnNodeCleanup(UFlowNode& Node) PURE_VIRTUAL(OnNodeCleanup);
	virtual void OnNodeDeinitializeInstance(UFlowNode& Node) PURE_VIRTUAL(OnNodeDeinitializeInstance);
	virtual EFlowPreloadInputResult OnNodeExecuteInput(UFlowNode& Node, const FName& PinName) PURE_VIRTUAL(OnNodeExecuteInput, return EFlowPreloadInputResult::Unhandled; );

	// Returns true if this node's content is fully preloaded (async completion has fired).
	virtual bool IsPreloaded() const PURE_VIRTUAL(IsPreloaded, return false; );

	// Triggers preloading on the given node. The base is a no-op; Standard tracks bPreloaded internally.
	virtual void TriggerPreload(UFlowNode& Node) PURE_VIRTUAL(TriggerPreload);
	virtual void TriggerFlush(UFlowNode& Node) PURE_VIRTUAL(TriggerFlush);

	// Called by UFlowNode::NotifyPreloadComplete() when async preloading finishes.
	// Returns AllComplete if all participants finished and AllPreloadsComplete should fire;
	// returns Pending if the call arrived after flush/cancel or other participants are still in progress.
	virtual EFlowPreloadCompleteResult OnPreloadComplete(UFlowNode& Node) PURE_VIRTUAL(OnPreloadComplete, return EFlowPreloadCompleteResult::Pending; );

	// Exec output pin fired when all preloads for this node are complete.
	static const FFlowPin OUTPIN_AllPreloadsComplete;

#if WITH_EDITOR
	virtual void GetContextInputs(TArray<FFlowPin>& OutInputPins) const {}
	virtual void GetContextOutputs(TArray<FFlowPin>& OutOutputPins) const {}
#endif
};

/**
 * Standard preload helper. Calls TriggerPreload/TriggerFlush on the owning node at the
 * timing specified by the asset's FFlowPreloadPolicy. Also recognises the Preload and Flush
 * exec input pins for manual (ManualOnly) triggering.
 */
USTRUCT()
struct FLOW_API FFlowPreloadHelper_Standard : public FFlowPreloadHelper
{
	GENERATED_BODY()

	// IFlowCoreExecutableInterface pass-thrus
	virtual void OnNodeInitializeInstance(UFlowNode& Node) override;
	virtual void OnNodeActivate(UFlowNode& Node) override;
	virtual void OnNodeCleanup(UFlowNode& Node) override;
	virtual void OnNodeDeinitializeInstance(UFlowNode& Node) override;
	virtual EFlowPreloadInputResult OnNodeExecuteInput(UFlowNode& Node, const FName& PinName) override;

	virtual bool IsPreloaded() const override { return bPreloaded; }
	virtual void TriggerPreload(UFlowNode& Node) override;
	virtual void TriggerFlush(UFlowNode& Node) override;

	// Called by UFlowNode::NotifyPreloadComplete() to update async state before the output pin fires.
	virtual EFlowPreloadCompleteResult OnPreloadComplete(UFlowNode& Node) override;

private:
	bool bPreloaded = false;

	// Number of outstanding async completions (node + addons) between TriggerPreload and full completion.
	// Counts up before any PreloadContent calls so re-entrant NotifyPreloadComplete() is safe.
	// TriggerFlush resets to 0; OnPreloadComplete decrements; AllPreloadsComplete fires when it reaches 0.
	int32 PendingPreloadCount = 0;

#if WITH_EDITOR
	virtual void GetContextInputs(TArray<FFlowPin>& OutInputPins) const override;
	virtual void GetContextOutputs(TArray<FFlowPin>& OutOutputPins) const override;
#endif

	// Exec input pin triggered to manually preload this node's content.
	static const FFlowPin INPIN_PreloadContent;

	// Exec input pin triggered to manually flush this node's content.
	static const FFlowPin INPIN_FlushContent;
};
