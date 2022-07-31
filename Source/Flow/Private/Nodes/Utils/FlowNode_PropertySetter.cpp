// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Utils/FlowNode_PropertySetter.h"

#include "FlowAsset.h"

UFlowNode_PropertySetter::UFlowNode_PropertySetter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFlowNode_PropertySetter::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}

const TArray<FFlowPropertyPin> UFlowNode_PropertySetter::GetInputProperties()
{
	LoadProperty();

	TArray<FFlowPropertyPin> Properties;
	if (!Property)
	{
		return Properties;
	}

	return ConvertProperties({{"",Property }});
}
