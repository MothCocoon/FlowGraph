// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Interfaces/FlowContextPinSupplierInterface.h"

bool IFlowContextPinSupplierInterface::K2_SupportsContextPins_Implementation() const
{
	return false;
}

TArray<FFlowPin> IFlowContextPinSupplierInterface::K2_GetContextInputs_Implementation() const
{
	return TArray<FFlowPin>();
}

TArray<FFlowPin> IFlowContextPinSupplierInterface::K2_GetContextOutputs_Implementation() const
{
	return TArray<FFlowPin>();
}
