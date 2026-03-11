// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Graph/FlowNode_Start.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_Start)

UFlowNode_Start::UFlowNode_Start()
{
#if WITH_EDITOR
	Category = TEXT("Graph");
	NodeDisplayStyle = FlowNodeStyle::InOut;
	bCanDelete = bCanDuplicate = false;
#endif

	OutputPins = { UFlowNode::DefaultOutputPin };
}

void UFlowNode_Start::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}

void UFlowNode_Start::SetDataPinValueSupplier(IFlowDataPinValueSupplierInterface* DataPinValueSupplier)
{
	FlowDataPinValueSupplierInterface = Cast<UObject>(DataPinValueSupplier);
}

#if WITH_EDITOR

bool UFlowNode_Start::TryAppendExternalInputPins(TArray<FFlowPin>& InOutPins) const
{
	// Add pins for all of the Flow DataPin Properties
	for (const FFlowNamedDataPinProperty& DataPinProperty : NamedProperties)
	{
		if (DataPinProperty.IsValid())
		{
			InOutPins.AddUnique(DataPinProperty.CreateFlowPin());
		}
	}
	
	return !NamedProperties.IsEmpty();
}

#endif // WITH_EDITOR

FFlowDataPinResult UFlowNode_Start::TrySupplyDataPin(FName PinName) const
{
	if (FlowDataPinValueSupplierInterface)
	{
		FFlowDataPinResult SuppliedResult = FlowDataPinValueSupplierInterface->TrySupplyDataPin(PinName);

		if (FlowPinType::IsSuccess(SuppliedResult.Result))
		{
			return SuppliedResult;
		}
	}

	return Super::TrySupplyDataPin(PinName);
}

