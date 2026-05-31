// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Policies/FlowPolicy.h"
#include "Policies/FlowPreloadPolicyEnums.h"
#include "FlowPreloadPolicy.generated.h"

class UFlowNode;

/* 
 * Policy governing how preloading and flushing of node content is managed for a Flow Asset.
 * Configure the default policy project-wide via UFlowSettings, and override per-domain via UFlowAsset subclasses.
 */
USTRUCT(BlueprintType)
struct FLOW_API FFlowPreloadPolicy : public FFlowPolicy
{
	GENERATED_BODY()

	/* Returns the resolved preload timing for the given node, checking per-class overrides first.
	 * Override in subclasses for code-driven per-node logic. */
	virtual EFlowPreloadTiming GetPreloadTimingForNode(const UFlowNode& Node) const PURE_VIRTUAL(FFlowPreloadPolicy::GetPreloadTimingForNode, return EFlowPreloadTiming::Invalid;);

	/* Returns the resolved flush timing for the given node, checking per-class overrides first.
	 * Override in subclasses for code-driven per-node logic. */
	virtual EFlowFlushTiming GetFlushTimingForNode(const UFlowNode& Node) const PURE_VIRTUAL(FFlowPreloadPolicy::GetFlushTimingForNode, return EFlowFlushTiming::Invalid;);

	/* Returns the UScriptStruct type to instantiate as the FFlowPreloadHelper for a given preloadable node.
	 * Default returns FFlowPreloadHelper_Standard. Override to supply project-specific helper types. */
	virtual UScriptStruct* GetPreloadHelperStructType(const UFlowNode& Node) const PURE_VIRTUAL(FFlowPreloadPolicy::GetPreloadHelperStructType, return nullptr;);
};
