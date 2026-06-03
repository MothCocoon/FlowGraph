// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/FlowPin.h"
#include "Policies/FlowPreloadPolicyEnums.h"

#include "FlowPreloadHelper.generated.h"

class UFlowNode;

/**
 * Base preload helper struct, which establishes the interface for preload helpers. 
 *
 * - Nodes and/or Nodes with AddOns that implement IFlowPreloadableInterface allocate SUBCLASS of this struct.
 * - Non-preloadable nodes (with no preloadable addons) leave PreloadHelper uninitialized (invalid).
 * - The base implementation is a pure virtual. *
 * - The concrete instance type is determined by FFlowPreloadPolicy::GetPreloadHelperStructType(),
 *   typically FFlowPreloadHelper_Standard. Projects may supply their own subclass via a custom
 *   FFlowPreloadPolicy subclass.
 */
USTRUCT()
struct FLOW_API FFlowPreloadHelper
{
	GENERATED_BODY()

public:	
	/* Exec output pin fired when all preloads for this node are complete. */
	static const FFlowPin OUTPIN_AllPreloadsComplete;
	
public:	
	virtual ~FFlowPreloadHelper() = default;

	// IFlowCoreExecutableInterface
	virtual void OnNodeInitializeInstance(UFlowNode& Node) PURE_VIRTUAL(OnNodeInitializeInstance);
	virtual void OnNodeActivate(UFlowNode& Node) PURE_VIRTUAL(OnNodeActivate);
	virtual void OnNodeCleanup(UFlowNode& Node) PURE_VIRTUAL(OnNodeCleanup);
	virtual void OnNodeDeinitializeInstance(UFlowNode& Node) PURE_VIRTUAL(OnNodeDeinitializeInstance);
	virtual EFlowPreloadInputResult OnNodeExecuteInput(UFlowNode& Node, const FName& PinName) PURE_VIRTUAL(OnNodeExecuteInput, return EFlowPreloadInputResult::Invalid;);
	// --

	/* Returns true if this node's content is fully preloaded (if async, the async load(s) must be complete). */
	virtual bool IsContentPreloaded() const PURE_VIRTUAL(IsContentPreloaded, return false;);

	/* These Trigger functions are safe to be called when already preloaded, or already flushed. */
	virtual void TriggerPreload(UFlowNode& Node) PURE_VIRTUAL(TriggerPreload);
	virtual void TriggerFlush(UFlowNode& Node) PURE_VIRTUAL(TriggerFlush);

	/* Called by UFlowNode::NotifyPreloadComplete() when async preloading finishes.
	 * Possible results:
	 * - Completed			- all participants finished, AllPreloadsComplete should fire.
	 * - PreloadInProgress  - call arrived after flush/cancel, or other participants are still in progress. */
	virtual EFlowPreloadResult OnPreloadComplete(UFlowNode& Node) PURE_VIRTUAL(OnPreloadComplete, return EFlowPreloadResult::Invalid;);

#if WITH_EDITOR
	/* Provide Preload-specific pins to the FlowNode. */
	virtual void GetContextInputs(TArray<FFlowPin>& OutInputPins) const {}
	virtual void GetContextOutputs(TArray<FFlowPin>& OutOutputPins) const;
#endif
};

/**
 * Standard preload helper. 
 * 
 * Calls TriggerPreload/TriggerFlush on the owning node at the
 * timing specified by the asset's FFlowPreloadPolicy. 
 * 
 * Also adds the Preload and Flush exec input pins for manual triggering.
 */
USTRUCT()
struct FLOW_API FFlowPreloadHelper_Standard : public FFlowPreloadHelper
{
	GENERATED_BODY()

protected:
	/* Exec input pin triggered to manually preload this node's content. */
	static const FFlowPin INPIN_PreloadContent;

	/* Exec input pin triggered to manually flush this node's content. */
	static const FFlowPin INPIN_FlushContent;
	
	/* True if the content completed its preload (and hasn't been flushed). */
	bool bContentPreloaded = false;

	/* Number of outstanding async completions (node + addons) between TriggerPreload and full completion.
	 * Counts up before any PreloadContent calls so re-entrant NotifyPreloadComplete() is safe.
	 * TriggerFlush resets to 0; OnPreloadComplete decrements; AllPreloadsComplete fires when it reaches 0. */
	int32 PendingPreloadCount = 0;

public:	
	// IFlowCoreExecutableInterface
	virtual void OnNodeInitializeInstance(UFlowNode& Node) override;
	virtual void OnNodeActivate(UFlowNode& Node) override;
	virtual void OnNodeCleanup(UFlowNode& Node) override;
	virtual void OnNodeDeinitializeInstance(UFlowNode& Node) override;
	virtual EFlowPreloadInputResult OnNodeExecuteInput(UFlowNode& Node, const FName& PinName) override;
	// --

	virtual bool IsContentPreloaded() const override { return bContentPreloaded; }

	virtual void TriggerPreload(UFlowNode& Node) override;
	virtual void TriggerFlush(UFlowNode& Node) override;

	/* Called by UFlowNode::NotifyPreloadComplete() to update async state before the output pin fires. */
	virtual EFlowPreloadResult OnPreloadComplete(UFlowNode& Node) override;

protected:
#if WITH_EDITOR
	virtual void GetContextInputs(TArray<FFlowPin>& OutInputPins) const override;
#endif
};
