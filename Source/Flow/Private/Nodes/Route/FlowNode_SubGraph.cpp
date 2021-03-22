#include "Nodes/Route/FlowNode_SubGraph.h"

#include "FlowAsset.h"
#include "FlowSubsystem.h"

FName UFlowNode_SubGraph::StartPinName(TEXT("Start"));
FName UFlowNode_SubGraph::FinishPinName(TEXT("Finish"));

UFlowNode_SubGraph::UFlowNode_SubGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::SubGraph;
#endif

	InputNames = {StartPinName};
	OutputNames = {FinishPinName};
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
		GetFlowSubsystem()->FinishSubFlow(this);
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
		if (PinName == TEXT("Start"))
		{
			GetFlowSubsystem()->StartSubFlow(this);
		}
		else
		{
			GetFlowAsset()->TriggerCustomEvent(this, PinName);
		}
	}
}

void UFlowNode_SubGraph::Cleanup()
{
	if (!Asset.IsNull())
	{
		GetFlowSubsystem()->FinishSubFlow(this);
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

UObject* UFlowNode_SubGraph::GetAssetToEdit()
{
	return Asset.IsNull() ? nullptr : LoadAsset<UObject>(Asset);
}

TArray<FName> UFlowNode_SubGraph::GetContextInputs()
{
	TArray<FName> EventNames;

	if (!Asset.IsNull())
	{
		Asset.LoadSynchronous();
		for (const FName& PinName : Asset.Get()->GetCustomInputs())
		{
			if (!PinName.IsNone())
			{
				EventNames.Emplace(PinName);
			}
		}
	}

	return EventNames;
}

TArray<FName> UFlowNode_SubGraph::GetContextOutputs()
{
	TArray<FName> EventNames;

	if (!Asset.IsNull())
	{
		Asset.LoadSynchronous();
		for (const FName& PinName : Asset.Get()->GetCustomOutputs())
		{
			if (!PinName.IsNone())
			{
				EventNames.Emplace(PinName);
			}
		}
	}

	return EventNames;
}

void UFlowNode_SubGraph::PostLoad()
{
	Super::PostLoad();
	
	SubscribeToAssetChanges();
}

void UFlowNode_SubGraph::PreEditChange(UProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);

	if (PropertyAboutToChange->GetFName() == GET_MEMBER_NAME_CHECKED(UFlowNode_SubGraph, Asset))
	{
		if (Asset)
		{
			Asset->OnSubGraphReconstructionRequested.Unbind();
		}
	}
}

void UFlowNode_SubGraph::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property && PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UFlowNode_SubGraph, Asset))
	{
		OnReconstructionRequested.ExecuteIfBound();
		SubscribeToAssetChanges();
	}
}

void UFlowNode_SubGraph::SubscribeToAssetChanges()
{
	if (Asset)
	{
		TWeakObjectPtr<UFlowNode_SubGraph> SelfWeakPtr(this);
		Asset->OnSubGraphReconstructionRequested.BindLambda([SelfWeakPtr]()
		{
			if (SelfWeakPtr.IsValid())
			{
				SelfWeakPtr->OnReconstructionRequested.ExecuteIfBound();
			}
		});
	}
}
#endif
