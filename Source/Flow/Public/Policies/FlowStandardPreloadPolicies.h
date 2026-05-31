// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Policies/FlowPreloadPolicy.h"
#include "FlowStandardPreloadPolicies.generated.h"

/* The "standard" preload implementation, this may be updated in subclasses of this class or of FFlowPreloadPolicy directly. */
USTRUCT(BlueprintType)
struct FLOW_API FFlowPreloadPolicy_Standard : public FFlowPreloadPolicy
{
	GENERATED_BODY()

public:
	/* Default preload timing applied to all preloadable nodes in the graph. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preload")
	EFlowPreloadTiming DefaultPreloadTiming = EFlowPreloadTiming::OnGraphInitialize;

	/* Default flush timing applied to all preloadable nodes in the graph. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preload")
	EFlowFlushTiming DefaultFlushTiming = EFlowFlushTiming::OnGraphDeinitialize;

	/* Per-node-class preload timing overrides (key = GetFName(), e.g. "FlowNode_SubGraph"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preload")
	TMap<FName, EFlowPreloadTiming> NodePreloadTimingOverrides;

	/* Per-node-class flush timing overrides (key = GetFName(), e.g. "FlowNode_SubGraph"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preload")
	TMap<FName, EFlowFlushTiming> NodeFlushTimingOverrides;

public:
	/* Returns the resolved preload timing for the given node, checking per-class overrides first.
	 * Override in subclasses for code-driven per-node logic. */
	virtual EFlowPreloadTiming GetPreloadTimingForNode(const UFlowNode& Node) const override;

	/* Returns the resolved flush timing for the given node, checking per-class overrides first.
	 * Override in subclasses for code-driven per-node logic. */
	virtual EFlowFlushTiming GetFlushTimingForNode(const UFlowNode& Node) const override;

	/* Returns the UScriptStruct type to instantiate as the FFlowPreloadHelper for a given preloadable node.
	 * Default returns FFlowPreloadHelper_Standard. Override to supply project-specific helper types. */
	virtual UScriptStruct* GetPreloadHelperStructType(const UFlowNode& Node) const override;
};
