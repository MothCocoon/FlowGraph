#include "FlowNodeSubFlow.h"
#include "../FlowAsset.h"
#include "../../FlowSubsystem.h"

UFlowNodeSubFlow::UFlowNodeSubFlow(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFlowNodeSubFlow::ExecuteInput(const uint8 Pin)
{
	if (FlowAsset.IsNull())
	{
		Finish();
	}
	else
	{
		GetFlowAsset()->GetFlowSubsystem()->StartSubFlow(this, FlowAsset, GetFlowAsset());
	}
}

FString UFlowNodeSubFlow::GetDesc()
{
	return FlowAsset.IsNull() ? FString() : FlowAsset.ToSoftObjectPath().GetAssetName();
}

#if WITH_EDITOR
UObject* UFlowNodeSubFlow::GetAssetToOpen()
{
	return FlowAsset.IsNull() ? nullptr : LoadAsset<UObject>(FlowAsset);
}
#endif
