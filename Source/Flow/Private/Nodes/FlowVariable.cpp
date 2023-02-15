// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/FlowVariable.h"
#include "Nodes/FlowNode.h"

FFlowPropertyBag::FFlowPropertyBag(const FProperty& Property)
	: PropertyName(Property.GetFName())
	, PinTooltip(Property.GetToolTipText(true).ToString())
{
	Properties.AddProperty(PropertyName, &Property);
}

FFlowInputOutputPin::FFlowInputOutputPin(const FName& InputPinName, const FName& OutputPinName, const FGuid& InputNodeGuid, const FGuid& OutputNodeGuid)
	: InputPinName(InputPinName)
	, OutputPinName(OutputPinName)
	, InputNodeGuid(InputNodeGuid)
	, OutputNodeGuid(OutputNodeGuid)
{
}

bool FFlowInputOutputPin::IsPinValid() const
{
	return !InputPinName.IsNone() || !OutputPinName.IsNone();
}

void FFlowInputOutputPin::SetInputValue(const UFlowNode* FromOutputNode, const FProperty* InProperty, const FProperty* OutProperty)
{
	check(InProperty);
	check(OutProperty);

	const FName PropName = InProperty->GetFName();

	if (PropertyBag.FindPropertyDescByName(PropName) == nullptr)
	{
		PropertyBag.AddProperty(PropName, EPropertyBagPropertyType::Struct, FFlowPropertyBag::StaticStruct());
	}

	const auto StructProperty = CastField<FStructProperty>(OutProperty);
	check(StructProperty);
	const auto FlowBag = *StructProperty->ContainerPtrToValuePtr<FFlowPropertyBag>(FromOutputNode);
	
	PropertyBag.SetValueStruct(PropName, FlowBag);
}
