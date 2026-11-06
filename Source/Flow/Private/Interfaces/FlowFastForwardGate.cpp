// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Interfaces/FlowFastForwardGate.h"

IFlowFastForwardGate* FFlowFastForwardGate::Gate = nullptr;

void FFlowFastForwardGate::SetGate(IFlowFastForwardGate* InGate)
{
	Gate = InGate;
}

IFlowFastForwardGate* FFlowFastForwardGate::GetGate()
{
	return Gate;
}

bool FFlowFastForwardGate::ShouldFastForwardInput(const UFlowNode& Node, const FName& PinName, FName& OutOutputPinName)
{
	return Gate && Gate->ShouldFastForwardInput(Node, PinName, OutOutputPinName);
}

void FFlowFastForwardGate::ReportForcedOutput(const UFlowNode& Node, const FName& OutputPinName)
{
	if (Gate)
	{
		Gate->OnOutputForced(Node, OutputPinName);
	}
}

void FFlowFastForwardGate::ReportBrokenRoute(const UFlowNode& Node, const FName& OutputPinName)
{
	if (Gate)
	{
		Gate->OnRouteBroken(Node, OutputPinName);
	}
}
