#include "FlowNode.h"
#include "../FlowAsset.h"

#include "Misc/App.h"

UFlowNode::UFlowNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Flow");
#endif
}

#if WITH_EDITOR
void UFlowNode::SetGraphNode(UEdGraphNode* NewGraph)
{
	GraphNode = NewGraph;
}
#endif

UFlowAsset* UFlowNode::GetFlowAsset() const
{
	return GetOuter() ? Cast<UFlowAsset>(GetOuter()) : nullptr;
}

void UFlowNode::ExecuteInput(const uint8 Pin)
{
	ActivateOutput(0);
	Finish();
}

void UFlowNode::ExecuteOutput(const uint8 Pin)
{
	ActivateOutput(0);
	Finish();
}

void UFlowNode::ActivateOutput(const uint8 Pin)
{
	RecordOutput(Pin);

	if (Connections.Contains(Pin))
	{
		const FFlowPin FlowPin = GetConnection(Pin);
		GetFlowAsset()->ActivateInput(FlowPin.NodeGuid, FlowPin.PinIndex);
	}
}

void UFlowNode::Finish()
{
	Cleanup();
	GetFlowAsset()->FinishNode(this);
}

void UFlowNode::RecordInput(const uint8 PinIndex)
{
	TArray<double>& Records = InputRecords.FindOrAdd(PinIndex);
	Records.Add(FApp::GetCurrentTime());
}

void UFlowNode::RecordOutput(const uint8 PinIndex)
{
	TArray<double>& Records = OutputRecords.FindOrAdd(PinIndex);
	Records.Add(FApp::GetCurrentTime());
}

void UFlowNode::ResetRecords()
{
	InputRecords.Empty();
	OutputRecords.Empty();
}

#if WITH_EDITOR
TMap<uint8, double> UFlowNode::GetWireRecords() const
{
	TMap<uint8, double> Result;
	for (const TPair<uint8, TArray<double>>& Record : OutputRecords)
	{
		Result.Add(Record.Key, Record.Value.Last());
	}
	return Result;
}

UFlowNode* UFlowNode::GetInspectedInstance()
{
	if (const UFlowAsset* FlowInstance = GetFlowAsset()->GetInspectedInstance())
	{
		return FlowInstance->GetNodeInstance(GetGuid());
	}

	return nullptr;
}
#endif
