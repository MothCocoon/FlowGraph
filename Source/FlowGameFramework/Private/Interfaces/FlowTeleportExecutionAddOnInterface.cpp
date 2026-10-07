// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Interfaces/FlowTeleportExecutionAddOnInterface.h"

#include "AddOns/FlowNodeAddOn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowTeleportExecutionAddOnInterface)

bool IFlowTeleportExecutionAddOnInterface::ImplementsInterfaceSafe(const UFlowNodeAddOn* AddOnTemplate)
{
	return IsValid(AddOnTemplate) && AddOnTemplate->Implements<UFlowTeleportExecutionAddOnInterface>();
}
