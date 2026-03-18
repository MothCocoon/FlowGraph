// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Policies/FlowStandardPreloadPolicies.h"
#include "Policies/FlowPreloadHelper.h"
#include "Nodes/FlowNode.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowStandardPreloadPolicies)

EFlowPreloadTiming FFlowPreloadPolicy_Standard::GetPreloadTimingForNode(const UFlowNode& Node) const
{
	if (const EFlowPreloadTiming* OverrideTiming = NodePreloadTimingOverrides.Find(Node.GetClass()->GetFName()))
	{
		return *OverrideTiming;
	}

	return DefaultPreloadTiming;
}

EFlowFlushTiming FFlowPreloadPolicy_Standard::GetFlushTimingForNode(const UFlowNode& Node) const
{
	if (const EFlowFlushTiming* OverrideTiming = NodeFlushTimingOverrides.Find(Node.GetClass()->GetFName()))
	{
		return *OverrideTiming;
	}

	return DefaultFlushTiming;
}

UScriptStruct* FFlowPreloadPolicy_Standard::GetPreloadHelperStructType(const UFlowNode& Node) const
{
	return FFlowPreloadHelper_Standard::StaticStruct();
}
