// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/FlowNodeBase.h"

#if WITH_EDITORONLY_DATA
/**
 * Temporarily suspends reconstruction notifications while a caller applies a batch of properties.
 *
 * AddOn property callbacks can request reconstruction immediately. During a multi-AddOn update,
 * that would rebuild the parent against a partially updated tree and stale pin caches. Batch
 * updates use this scope around callback dispatch after attachment is complete, then reconstruct
 * parent once the final pin state is ready. The scope restores both original delegate bindings
 * when it ends and does not request reconstruction itself.
 */
class FLOW_API FScopedFlowNodeReconstructionRequestDeferral
{
public:
	explicit FScopedFlowNodeReconstructionRequestDeferral(UFlowNodeBase& InFlowNodeBase);
	~FScopedFlowNodeReconstructionRequestDeferral();

	FScopedFlowNodeReconstructionRequestDeferral(const FScopedFlowNodeReconstructionRequestDeferral&) = delete;
	FScopedFlowNodeReconstructionRequestDeferral& operator=(const FScopedFlowNodeReconstructionRequestDeferral&) = delete;

private:
	// Flow object whose editor listeners are suspended for this property's callback batch.
	UFlowNodeBase& FlowNodeBase;

	// Direct reconstruction listener saved so the exact existing binding is restored on scope exit.
	FFlowNodeEvent SavedReconstructionRequested;

	// AddOn-to-parent reconstruction listener restored alongside the direct listener.
	FFlowNodeEvent SavedAddOnRequestedParentReconstruction;
};
#endif // WITH_EDITORONLY_DATA
