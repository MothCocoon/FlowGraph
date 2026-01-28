// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowNamedDataPinPropertyCustomization.h"
#include "Types/FlowNamedDataPinProperty.h"

FText FFlowNamedDataPinPropertyCustomization::BuildHeaderText() const
{
	if (const FFlowNamedDataPinProperty* FlowNamedDataPinProperty = IFlowExtendedPropertyTypeCustomization::TryGetTypedStructValue<FFlowNamedDataPinProperty>(StructPropertyHandle))
	{
		return FlowNamedDataPinProperty->BuildHeaderText();
	}

	return Super::BuildHeaderText();
}
