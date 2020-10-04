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
		GetFlowSubsystem()->PreloadSubFlow(this);
	}
}

void UFlowNode_SubGraph::FlushContent()
{
	if (!Asset.IsNull())
	{
		GetFlowSubsystem()->FlushPreload(this);
	}
}

void UFlowNode_SubGraph::ExecuteInput(const FName& PinName)
{
	if (Asset.IsNull())
	{
		LogError(TEXT("Missing Flow Asset"));
		Finish();
	}
	else
	{
		if (PinName == DefaultInputName)
		{
			GetFlowSubsystem()->StartSubFlow(this);
		}
		else
		{
			GetFlowAsset()->TriggerCustomEvent(this, PinName);
		}
	}
}

void UFlowNode_SubGraph::ForceFinishNode()
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

TArray<FName> UFlowNode_SubGraph::GetContextInputs()
{
	TArray<FName> EventNames;

	if (!Asset.IsNull())
	{
		LoadAsset<UFlowAsset>(Asset);
		EventNames = Asset.Get()->GetCustomEvents();
	}

	return EventNames;
}

TArray<FName> UFlowNode_SubGraph::GetContextOutputs()
{
	TArray<FName> EventNames;

	if (!Asset.IsNull())
	{
		LoadAsset<UFlowAsset>(Asset);
		EventNames = Asset.Get()->GetCustomOutputs();
	}

	return EventNames;
}
#endif
