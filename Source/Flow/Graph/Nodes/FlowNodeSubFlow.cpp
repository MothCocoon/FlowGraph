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

#if WITH_EDITOR
FString UFlowNodeSubFlow::GetDescription() const
{
	return FlowAsset.IsNull() ? FString() : FlowAsset.ToSoftObjectPath().GetAssetName();
}

UObject* UFlowNodeSubFlow::GetAssetToOpen()
{
	return FlowAsset.IsNull() ? nullptr : LoadAsset<UObject>(FlowAsset);
}
#endif
