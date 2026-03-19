// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "UObject/Interface.h"
#include "Policies/FlowPreloadPolicyEnums.h"

#include "FlowPreloadableInterface.generated.h"

/**
 * Implemented by Flow Nodes that have content which can be asynchronously preloaded.
 * Implementing this interface opts the node into the preload system: the node will have
 * a FFlowPreloadHelper allocated during InitializeInstance (as determined by the asset's
 * FFlowPreloadPolicy), which drives when PreloadContent and FlushContent are called.
 */
UINTERFACE(MinimalAPI, Blueprintable, DisplayName = "Flow Preloadable Interface")
class UFlowPreloadableInterface : public UInterface
{
	GENERATED_BODY()
};

class FLOW_API IFlowPreloadableInterface
{
	GENERATED_BODY()

public:
	/* Called by the preload helper to start loading this node's content.
	 *
	 * Return EFlowPreloadResult::Completed  if loading finished synchronously.
	 * Return EFlowPreloadResult::PreloadInProgress if loading started but is not yet done.
	 *   - In the PreloadInProgress case you MUST call NotifyPreloadComplete() on this node
	 *     (game thread) when loading finishes. AllPreloadsComplete fires at that point.
	 *   - If NotifyPreloadComplete() is called from within PreloadContent() itself
	 *     (e.g. FStreamableManager fires synchronously for an already-cached asset),
	 *     that is safe — state guards prevent double-fire.
	 *
	 * The default implementation calls K2_PreloadContent (Blueprint event) and returns
	 * Completed, so Blueprint nodes and existing sync C++ overrides work unchanged.
	 * Async C++ nodes override PreloadContent(); async Blueprint nodes override
	 * K2_PreloadContent and return PreloadInProgress, then call NotifyPreloadComplete() when done. */
	UFUNCTION(BlueprintNativeEvent, Category = FlowPreloadableInterface, DisplayName = "Preload Content")
	EFlowPreloadResult K2_PreloadContent();
	virtual EFlowPreloadResult K2_PreloadContent_Implementation() { return EFlowPreloadResult::Completed; }
	virtual EFlowPreloadResult PreloadContent() { return Execute_K2_PreloadContent(Cast<UObject>(this)); }

	/* Called by the preload helper to release this node's preloaded content. */
	UFUNCTION(BlueprintImplementableEvent, Category = FlowPreloadableInterface, DisplayName = "Flush Content")
	void K2_FlushContent();
	virtual void FlushContent() { Execute_K2_FlushContent(Cast<UObject>(this)); }

	static bool ImplementsInterfaceSafe(const UObject* Object);
};
