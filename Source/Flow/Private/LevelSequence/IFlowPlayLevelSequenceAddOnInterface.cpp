// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "LevelSequence/IFlowPlayLevelSequenceAddOnInterface.h"

#include "AddOns/FlowNodeAddOn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(IFlowPlayLevelSequenceAddOnInterface)

bool IFlowPlayLevelSequenceAddOnInterface::ImplementsInterfaceSafe(const UFlowNodeAddOn* AddOnTemplate)
{
	return IsValid(AddOnTemplate) && AddOnTemplate->Implements<UFlowPlayLevelSequenceAddOnInterface>();
}
