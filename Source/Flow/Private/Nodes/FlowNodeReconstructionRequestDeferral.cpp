// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Nodes/FlowNodeReconstructionRequestDeferral.h"

#if WITH_EDITORONLY_DATA
FScopedFlowNodeReconstructionRequestDeferral::FScopedFlowNodeReconstructionRequestDeferral(UFlowNodeBase& InFlowNodeBase)
	: FlowNodeBase(InFlowNodeBase)
	, SavedReconstructionRequested(FlowNodeBase.OnReconstructionRequested)
	, SavedAddOnRequestedParentReconstruction(FlowNodeBase.OnAddOnRequestedParentReconstruction)
{
	FlowNodeBase.OnReconstructionRequested.Unbind();
	FlowNodeBase.OnAddOnRequestedParentReconstruction.Unbind();
}

FScopedFlowNodeReconstructionRequestDeferral::~FScopedFlowNodeReconstructionRequestDeferral()
{
	FlowNodeBase.OnReconstructionRequested = MoveTemp(SavedReconstructionRequested);
	FlowNodeBase.OnAddOnRequestedParentReconstruction = MoveTemp(SavedAddOnRequestedParentReconstruction);
}
#endif // WITH_EDITORONLY_DATA
