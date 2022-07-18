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

const TMultiMap<TWeakObjectPtr<UObject>, FFlowInputOutputPin> UFlowNode_PropertySetter::GetInputProperties()
{
	LoadProperty();

	TMultiMap<TWeakObjectPtr<UObject>, FFlowInputOutputPin> Properties;
	if (!Property)
	{
		return Properties;
	}

	FFlowInputOutputPin PropertyToAdd = FFlowInputOutputPin(Property, nullptr);
	PropertyToAdd.InputPinName = Property->GetFName();
	PropertyToAdd.PinTooltip = Property->GetToolTipText(true).ToString();
	Properties.Add(GetVariableHolder(), PropertyToAdd);
	return Properties;
}
