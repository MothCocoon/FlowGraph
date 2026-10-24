// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Interfaces/FlowExternalExecutableInterface.h"

void IFlowExternalExecutableInterface::PreActivateExternalFlowExecutable(UFlowNodeBase& FlowNodeBase)
{
	Execute_K2_PreActivateExternalFlowExecutable(Cast<UObject>(this), &FlowNodeBase);
}
