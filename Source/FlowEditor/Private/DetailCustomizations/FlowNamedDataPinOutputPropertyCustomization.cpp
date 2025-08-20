// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowNamedDataPinOutputPropertyCustomization.h"

FText FFlowNamedDataPinPropertyCustomization::BuildHeaderText() const
{
	if (const FFlowNamedDataPinProperty* FlowNamedDataPinProperty = IFlowExtendedPropertyTypeCustomization::TryGetTypedStructValue<FFlowNamedDataPinProperty>(StructPropertyHandle))
	{
		return FlowNamedDataPinProperty->BuildHeaderText();
	}

	return Super::BuildHeaderText();
}
