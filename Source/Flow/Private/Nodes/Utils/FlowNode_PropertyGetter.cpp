// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Utils/FlowNode_PropertyGetter.h"

UFlowNode_PropertyGetter::UFlowNode_PropertyGetter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputPins = {};
	OutputPins = {};
}

const TArray<FFlowPropertyPin> UFlowNode_PropertyGetter::GetOutputProperties()
{
	LoadProperty();

	TArray<FFlowPropertyPin> Properties;
	if (!Property)
	{
		return Properties;
	}

	return ConvertProperties({{"",Property }});
}
