#include "Nodes/FlowNodeSubGraph.h"

#include "FlowAsset.h"
#include "FlowSubsystem.h"

UFlowNodeSubGraph::UFlowNodeSubGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	NodeStyle = EFlowNodeStyle::SubGraph;
#endif
}

void UFlowNodeSubGraph::PreloadContent()
{
	if (!Asset.IsNull())
	{
		GetFlowAsset()->GetFlowSubsystem()->PreloadSubFlow(this);
	}
}

void UFlowNodeSubGraph::FlushContent()
{
	if (!Asset.IsNull())
	{
		GetFlowAsset()->GetFlowSubsystem()->FlushPreload(this);
	}
}

void UFlowNodeSubGraph::ExecuteInput(const FName& PinName)
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

void UFlowNodeSubGraph::OnForceFinished()
{
	TriggerFirstOutput(true);
}

#if WITH_EDITOR
FString UFlowNodeSubGraph::GetNodeDescription() const
{
	return Asset.IsNull() ? FString() : Asset.ToSoftObjectPath().GetAssetName();
}

UObject* UFlowNodeSubGraph::GetAssetToOpen()
{
	return Asset.IsNull() ? nullptr : LoadAsset<UObject>(Asset);
}
#endif
