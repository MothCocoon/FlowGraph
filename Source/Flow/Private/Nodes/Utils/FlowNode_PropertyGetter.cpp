// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Utils/FlowNode_PropertyGetter.h"

UFlowNode_PropertyGetter::UFlowNode_PropertyGetter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputPins = {};
	OutputPins = {};
}

const TMultiMap<TWeakObjectPtr<UObject>, FFlowInputOutputPin> UFlowNode_PropertyGetter::GetOutputProperties()
{
	LoadProperty();
	
	TMultiMap<TWeakObjectPtr<UObject>, FFlowInputOutputPin> Properties;
	if (!Property)
	{
		return Properties;
	}

	FFlowInputOutputPin PropertyToAdd = FFlowInputOutputPin(nullptr, Property);
	PropertyToAdd.OutputPinName = Property->GetFName();
	PropertyToAdd.PinTooltip = Property->GetToolTipText(true).ToString();
	Properties.Add(GetVariableHolder(), PropertyToAdd);
	return Properties;
}