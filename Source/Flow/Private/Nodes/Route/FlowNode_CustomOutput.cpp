// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_CustomOutput.h"

#include "FlowAsset.h"
#include "Nodes/Route/FlowNode_SubGraph.h"

UFlowNode_CustomOutput::UFlowNode_CustomOutput(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bFinish(false)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::InOut;
#endif

	OutputPins.Empty();
}

void UFlowNode_CustomOutput::ExecuteInput(const FName& PinName)
{
	if (!EventName.IsNone() && GetFlowAsset()->GetCustomOutputs().Contains(EventName) && GetFlowAsset()->GetNodeOwningThisAssetInstance())
	{
		GetFlowAsset()->TriggerCustomOutput(EventName, bFinish);
	}
}

#if WITH_EDITOR
FString UFlowNode_CustomOutput::GetNodeDescription() const
{
	return EventName.ToString() + LINE_TERMINATOR + (bFinish ? TEXT("True") : TEXT("False"));
}
#endif
