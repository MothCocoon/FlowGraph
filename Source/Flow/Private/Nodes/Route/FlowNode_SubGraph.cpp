#include "Nodes/Route/FlowNode_SubGraph.h"

#include "FlowAsset.h"
#include "FlowSubsystem.h"

UFlowNode_SubGraph::UFlowNode_SubGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::SubGraph;
#endif
}

void UFlowNode_SubGraph::PreloadContent()
{
	if (!Asset.IsNull())
	{
		GetFlowAsset()->GetFlowSubsystem()->PreloadSubFlow(this);
	}
}

void UFlowNode_SubGraph::FlushContent()
{
	if (!Asset.IsNull())
	{
		GetFlowAsset()->GetFlowSubsystem()->FlushPreload(this);
	}
}

void UFlowNode_SubGraph::ExecuteInput(const FName& PinName)
{
	if (Asset.IsNull())
	{
		Finish();
	}
	else
	{
		GetFlowAsset()->GetFlowSubsystem()->StartSubFlow(this);
	}
}

void UFlowNode_SubGraph::OnForceFinished()
{
	TriggerFirstOutput(true);
}

#if WITH_EDITOR
FString UFlowNode_SubGraph::GetNodeDescription() const
{
	return Asset.IsNull() ? FString() : Asset.ToSoftObjectPath().GetAssetName();
}

UObject* UFlowNode_SubGraph::GetAssetToOpen()
{
	return Asset.IsNull() ? nullptr : LoadAsset<UObject>(Asset);
}
#endif
