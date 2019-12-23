#include "FlowNodeSubFlow.h"
#include "FlowSubsystem.h"
#include "Graph/FlowAsset.h"

UFlowNodeSubFlow::UFlowNodeSubFlow(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	NodeStyle = EFlowNodeStyle::SubFlow;
#endif
}

void UFlowNodeSubFlow::PreloadContent()
{
	if (!FlowAsset.IsNull())
	{
		GetFlowAsset()->GetFlowSubsystem()->PreloadSubFlow(this);
	}
}

void UFlowNodeSubFlow::FlushContent()
{
	if (!FlowAsset.IsNull())
	{
		GetFlowAsset()->GetFlowSubsystem()->FlushPreload(this);
	}
}

void UFlowNodeSubFlow::ExecuteInput(const FName& PinName)
{
	if (FlowAsset.IsNull())
	{
		Finish();
	}
	else
	{
		GetFlowAsset()->GetFlowSubsystem()->StartSubFlow(this);
	}
}

void UFlowNodeSubFlow::OnForceFinished()
{
	TriggerDefaultOutput(true);
}

#if WITH_EDITOR
FString UFlowNodeSubFlow::GetNodeDescription() const
{
	return FlowAsset.IsNull() ? FString() : FlowAsset.ToSoftObjectPath().GetAssetName();
}

UObject* UFlowNodeSubFlow::GetAssetToOpen()
{
	return FlowAsset.IsNull() ? nullptr : LoadAsset<UObject>(FlowAsset);
}
#endif