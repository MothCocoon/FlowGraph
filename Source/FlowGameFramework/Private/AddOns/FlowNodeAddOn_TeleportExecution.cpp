// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "AddOns/FlowNodeAddOn_TeleportExecution.h"

#include "Nodes/Actor/FlowNode_TeleportActorsV2.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNodeAddOn_TeleportExecution)

UFlowNodeAddOn_TeleportExecution::UFlowNodeAddOn_TeleportExecution()
{
#if WITH_EDITOR
	Category = TEXT("Flow|Actors");
#endif
}

EFlowAddOnAcceptResult UFlowNodeAddOn_TeleportExecution::AcceptFlowNodeAddOnParent_Implementation(
	const UFlowNodeBase* ParentTemplate,
	const TArray<UFlowNodeAddOn*>&) const
{
	const UFlowNode* ParentNode = Cast<UFlowNode>(ParentTemplate);
	if (!IsValid(ParentNode) || !ParentNode->IsA<UFlowNode_TeleportActorsV2>())
	{
		return EFlowAddOnAcceptResult::Reject;
	}

	return EFlowAddOnAcceptResult::TentativeAccept;
}

EFlowTeleportExecutionResult UFlowNodeAddOn_TeleportExecution::ExecuteTeleport(
	UFlowNode_TeleportActorsV2& Node,
	AActor& Actor,
	const FTransform& Destination,
	const FGuid& OperationGuid)
{
	return EFlowTeleportExecutionResult::UseDefault;
}

void UFlowNodeAddOn_TeleportExecution::CancelTeleport(UFlowNode_TeleportActorsV2& Node)
{
}
