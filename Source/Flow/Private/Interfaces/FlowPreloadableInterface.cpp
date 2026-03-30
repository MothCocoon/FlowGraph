// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Interfaces/FlowPreloadableInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowPreloadableInterface)

bool IFlowPreloadableInterface::ImplementsInterfaceSafe(const UObject* Object)
{
	return IsValid(Object) && Object->GetClass()->ImplementsInterface(UFlowPreloadableInterface::StaticClass());
}
