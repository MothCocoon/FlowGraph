#include "Nodes/Route/FlowNode_CustomOutput.h"

#include "FlowAsset.h"
#include "Nodes/Route/FlowNode_SubGraph.h"

UFlowNode_CustomOutput::UFlowNode_CustomOutput(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::InOut;
#endif
	
	OutputNames.Empty();
}

void UFlowNode_CustomOutput::ExecuteInput(const FName& PinName)
{
	if (!EventName.IsNone() && GetFlowAsset()->GetCustomOutputs().Contains(EventName) && GetFlowAsset()->GetNodeOwningThisAssetInstance())
	{
		GetFlowAsset()->TriggerCustomOutput(EventName);
	}
}

#if WITH_EDITOR
FString UFlowNode_CustomOutput::GetNodeDescription() const
{
	return EventName.ToString();
}
#endif
