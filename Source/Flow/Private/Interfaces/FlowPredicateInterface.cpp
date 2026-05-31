// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Interfaces/FlowPredicateInterface.h"
#include "AddOns/FlowNodeAddOn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowPredicateInterface)

bool IFlowPredicateInterface::ImplementsInterfaceSafe(const UFlowNodeAddOn* AddOnTemplate)
{
	if (!IsValid(AddOnTemplate))
	{
		return false;
	}

	const UClass* AddOnClass = AddOnTemplate->GetClass();
	if (AddOnClass->ImplementsInterface(UFlowPredicateInterface::StaticClass()))
	{
		return true;
	}

	return false;
}
