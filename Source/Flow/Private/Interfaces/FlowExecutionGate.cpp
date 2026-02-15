// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Interfaces/FlowExecutionGate.h"

#include "FlowAsset.h"
#include "Nodes/FlowPin.h"

IFlowExecutionGate* FFlowExecutionGate::Gate = nullptr;

void FFlowExecutionGate::SetGate(IFlowExecutionGate* InGate)
{
	Gate = InGate;
}

IFlowExecutionGate* FFlowExecutionGate::GetGate()
{
	return Gate;
}

bool FFlowExecutionGate::IsHalted()
{
	return (Gate != nullptr) && Gate->IsFlowExecutionHalted();
}