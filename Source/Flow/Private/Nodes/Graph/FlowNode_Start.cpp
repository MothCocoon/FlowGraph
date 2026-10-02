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

	OutputPins = {UFlowNode::DefaultOutputPin};
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

const FFlowAgentDoc& UFlowNode_Start::GetAgentDoc() const
{
	static const FFlowAgentDoc Doc = MakeAgentDoc(
		/*Guidance*/ TEXT("Define named properties (inherited from Define Properties) to declare input data pins that receive values passed in by the parent SubGraph node or another external data pin supplier. There is exactly one Start node per graph and it cannot be deleted or duplicated."),
		/*Tags*/     { TEXT("graph"), TEXT("start"), TEXT("datapin") },
		/*Articles*/ {  });
	return Doc;
}
#endif

FFlowDataPinResult UFlowNode_Start::TrySupplyDataPin(const FName PinName) const
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
