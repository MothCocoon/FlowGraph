#include "FlowNode.h"
#include "FlowSubsystem.h"
#include "Graph/FlowAsset.h"
#include "Graph/FlowAssetTypes.h"

#include "Misc/App.h"

UFlowNode::UFlowNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Flow");
	NodeStyle = EFlowNodeStyle::Default;
#endif

	InputNames = { FName() };
	OutputNames = { FName() };
}

#if WITH_EDITOR
void UFlowNode::SetGraphNode(UEdGraphNode* NewGraph)
{
	GraphNode = NewGraph;
}
#endif

UFlowAsset* UFlowNode::GetFlowAsset() const
{
	return Cast<UFlowAsset>(GetOuter());
}

bool UFlowNode::HasUserCreatedInputs() const
{
	if (CanUserAddInput())
	{
		return (CreatedInputs.Num() > 2); // 2 is only reasonable number of default inputs for such node
	}

	return false;
}

bool UFlowNode::HasUserCreatedOutputs() const
{
	if (CanUserAddOutput())
	{
		return (CreatedOutputs.Num() > 2); // 2 is only reasonable number of default inputs for such node
	}

	return false;
}

void UFlowNode::SetNumericalOutputs(const uint8 FirstNumber /*= 0*/, const uint8 LastNumber /*= 1*/)
{
	OutputNames.Empty();

	for (uint8 i = FirstNumber; i <= LastNumber; i++)
	{
		OutputNames.Emplace(FName(*FString::FromInt(i)));
	}
}

void UFlowNode::AddCreatedInput(const uint8 PinIndex, const FName& PinName)
{
	CreatedInputs.Add(PinName, PinIndex);
}

void UFlowNode::AddCreatedOutput(const uint8 PinIndex, const FName& PinName)
{
	CreatedOutputs.Add(PinName, PinIndex);
}

void UFlowNode::RemoveCreatedInput(const FName& PinName)
{
	InputNames.Remove(PinName);
	CreatedInputs.Remove(PinName);
}

void UFlowNode::RemoveCreatedOutput(const FName& PinName)
{
	OutputNames.Remove(PinName);
	CreatedOutputs.Remove(PinName);
}

FName UFlowNode::GetInputName(const uint8 PinIndex) const
{
	if (const FName* Name = CreatedInputs.FindKey(PinIndex))
	{
		return *Name;
	}

	return FName();
}

FName UFlowNode::GetOutputName(const uint8 PinIndex) const
{
	if (const FName* Name = CreatedOutputs.FindKey(PinIndex))
	{
		return *Name;
	}

	return FName();
}

TSet<UFlowNode*> UFlowNode::GetConnectedNodes() const
{
	TSet<UFlowNode*> Result;
	for (const TPair<FName, FConnectedPin>& Connection : Connections)
	{
		Result.Emplace(GetFlowAsset()->GetNode(Connection.Value.NodeGuid));
	}
	return Result;
}

UFlowSubsystem* UFlowNode::GetFlowSubsystem() const
{
	return GetFlowAsset()->GetFlowSubsystem();
}

void UFlowNode::TriggerInput(const FName& PinName)
{
	// record for debugging
	TArray<double>& Records = InputRecords.FindOrAdd(PinName);
	Records.Add(FApp::GetCurrentTime());

	ExecuteInput(PinName);
}

void UFlowNode::ExecuteInput(const FName& PinName)
{
	TriggerDefaultOutput(true);
}

void UFlowNode::TriggerDefaultOutput(const bool bFinish)
{
	if (const FName* Name = CreatedOutputs.FindKey(0))
	{
		TriggerOutput(*Name, bFinish);
	}
}

void UFlowNode::TriggerOutput(const FName& PinName, const bool bFinish /*= false*/)
{
	// record for debugging, even if nothing is connected to this pin
	TArray<double>& Records = OutputRecords.FindOrAdd(PinName);
	Records.Add(FApp::GetCurrentTime());

	// clean up node, if needed
	if (bFinish)
	{
		Finish();
	}

	// call the next node
	if (Connections.Contains(PinName))
	{
		const FConnectedPin FlowPin = GetConnection(PinName);
		GetFlowAsset()->TriggerInput(FlowPin.NodeGuid, FlowPin.PinName);
	}
}

void UFlowNode::Finish()
{
	Cleanup();
	GetFlowAsset()->FinishNode(this);
}

void UFlowNode::ForceFinishNode()
{
	OnForceFinished();
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
	for (const TPair<FName, TArray<double>>& Record : OutputRecords)
	{
		Result.Add(CreatedOutputs[Record.Key], Record.Value.Last());
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
