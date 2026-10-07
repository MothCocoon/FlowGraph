// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "AddOns/FlowNodeAddOn.h"
#include "Interfaces/FlowTeleportExecutionAddOnInterface.h"
#include "Types/FlowTeleportTypes.h"

#include "FlowNodeAddOn_TeleportExecution.generated.h"

class AActor;
class UFlowNode_TeleportActorsV2;

/** Optional extension point for replacing or augmenting Teleport Actors execution. */
UCLASS(Abstract, BlueprintType, EditInlineNew, meta = (DisplayName = "Teleport Execution"))
class FLOWGAMEFRAMEWORK_API UFlowNodeAddOn_TeleportExecution : public UFlowNodeAddOn, public IFlowTeleportExecutionAddOnInterface
{
	GENERATED_BODY()

public:
	UFlowNodeAddOn_TeleportExecution();

	virtual EFlowAddOnAcceptResult AcceptFlowNodeAddOnParent_Implementation(
		const UFlowNodeBase* ParentTemplate,
		const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const override;

	virtual EFlowTeleportExecutionResult ExecuteTeleport(
		UFlowNode_TeleportActorsV2& Node,
		AActor& Actor,
		const FTransform& Destination,
		const FGuid& OperationGuid) override;

	virtual void CancelTeleport(UFlowNode_TeleportActorsV2& Node) override;
};

