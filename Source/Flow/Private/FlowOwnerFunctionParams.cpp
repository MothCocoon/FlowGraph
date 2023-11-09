// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowOwnerFunctionParams.h"
#include "Nodes/FlowNode.h"
#include "Nodes/World/FlowNode_CallOwnerFunction.h"


// UFlowOwnerFunctionParams Implementation

UFlowOwnerFunctionParams::UFlowOwnerFunctionParams()
	: Super()
{
#if WITH_EDITOR
	InputNames.Add(UFlowNode::DefaultInputPin.PinName);
	OutputNames.Add(UFlowNode::DefaultOutputPin.PinName);
#endif //WITH_EDITOR
}

void UFlowOwnerFunctionParams::PreExecute(UFlowNode_CallOwnerFunction& InSourceNode, const FName& InputPinName)
{
	SourceNode = &InSourceNode;
	ExecutedInputPinName = InputPinName;

	BP_PreExecute();
}

void UFlowOwnerFunctionParams::PostExecute()
{
	BP_PostExecute();

	SourceNode = nullptr;
	ExecutedInputPinName = NAME_None;
}

bool UFlowOwnerFunctionParams::ShouldFinishForOutputName_Implementation(const FName& OutputName) const
{
	// Blueprint subclasses may want to declare certain outputs as "non-finishing"
	//  but by default, all outputs are 'finishing'
	return true;
}

TArray<FName> UFlowOwnerFunctionParams::BP_GetInputNames() const
{
	if (IsValid(SourceNode))
	{
		return SourceNode->GetInputNames();
	}

	return TArray<FName>();
}

TArray<FName> UFlowOwnerFunctionParams::BP_GetOutputNames() const
{
	if (IsValid(SourceNode))
	{
		return SourceNode->GetOutputNames();
	}

	return TArray<FName>();
}
