// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Interfaces/FlowSwitchCaseInterface.h"
#include "AddOns/FlowNodeAddOn.h"

bool IFlowSwitchCaseInterface::ImplementsInterfaceSafe(const UFlowNodeAddOn* AddOnTemplate)
{
	if (!IsValid(AddOnTemplate))
	{
		return false;
	}

	UClass* AddOnClass = AddOnTemplate->GetClass();
	if (AddOnClass->ImplementsInterface(UFlowSwitchCaseInterface::StaticClass()))
	{
		return true;
	}

	return false;
}
