// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "UObject/Interface.h"
#include "Types/FlowTeleportTypes.h"

#include "FlowTeleportExecutionAddOnInterface.generated.h"

class AActor;
class UFlowNodeAddOn;
class UFlowNode_TeleportActorsV2;
enum class EFlowTeleportExecutionResult : uint8;
struct FGuid;

/** Interface for addons that replace or augment Teleport Actors execution. */
UINTERFACE(MinimalAPI)
class UFlowTeleportExecutionAddOnInterface : public UInterface
{
	GENERATED_BODY()
};

class FLOWGAMEFRAMEWORK_API IFlowTeleportExecutionAddOnInterface
{
	GENERATED_BODY()

public:
	static bool ImplementsInterfaceSafe(const UFlowNodeAddOn* AddOnTemplate);

	virtual EFlowTeleportExecutionResult ExecuteTeleport(
		UFlowNode_TeleportActorsV2& Node,
		AActor& Actor,
		const FTransform& Destination,
		const FGuid& OperationGuid) = 0;

	virtual void CancelTeleport(UFlowNode_TeleportActorsV2& Node) {}
};
