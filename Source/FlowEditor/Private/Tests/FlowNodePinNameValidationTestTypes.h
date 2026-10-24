// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Misc/DataValidation.h"
#include "Nodes/FlowNode.h"

#include "FlowNodePinNameValidationTestTypes.generated.h"

UCLASS(Hidden, NotBlueprintable, NotPlaceable, meta = (ExcludeFromFlowCatalog))
class UFlowNodePinNameValidationTestNode : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowNodePinNameValidationTestNode()
	{
		InputPins = {FFlowPin(TEXT("StartLeft"))};
		OutputPins = {FFlowPin(TEXT("StartLeft"))};
	}

	void SetPinsForTest(const TArray<FFlowPin>& InInputPins, const TArray<FFlowPin>& InOutputPins)
	{
		InputPins = InInputPins;
		OutputPins = InOutputPins;
	}

	EDataValidationResult ValidatePinNamesForTest()
	{
		return ValidateNode();
	}
};
