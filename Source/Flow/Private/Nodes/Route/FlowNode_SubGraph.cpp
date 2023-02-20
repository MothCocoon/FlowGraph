// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_SubGraph.h"

#include "FlowAsset.h"
#include "FlowMessageLog.h"
#include "FlowSubsystem.h"

FFlowPin UFlowNode_SubGraph::StartPin(TEXT("Start"));
FFlowPin UFlowNode_SubGraph::FinishPin(TEXT("Finish"));

UFlowNode_SubGraph::UFlowNode_SubGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bCanInstanceIdenticalAsset(false)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::SubGraph;
#endif

	InputPins = {StartPin};
	OutputPins = {FinishPin};
}

bool UFlowNode_SubGraph::CanBeAssetInstanced() const
{
	return !Asset.IsNull() && (bCanInstanceIdenticalAsset || Asset->GetPathName() != GetFlowAsset()->GetTemplateAsset()->GetPathName());
}

void UFlowNode_SubGraph::PreloadContent()
{
	if (CanBeAssetInstanced() && GetFlowSubsystem())
	{
		GetFlowSubsystem()->CreateSubFlow(this, FString(), true);
	}
}

void UFlowNode_SubGraph::FlushContent()
{
	if (CanBeAssetInstanced() && GetFlowSubsystem())
	{
		GetFlowSubsystem()->RemoveSubFlow(this, EFlowFinishPolicy::Abort);
	}
}

void UFlowNode_SubGraph::ExecuteInput(const FName& PinName)
{
	if (CanBeAssetInstanced() == false)
	{
		if (Asset.IsNull())
		{
			LogRuntimeError(TEXT("Missing Flow Asset"));
		}
		else
		{
			LogRuntimeError(FString::Printf(TEXT("Asset %s cannot be instance, probably is the same as the asset owning this SubGraph node."), *Asset->GetPathName()));
		}
		
		Finish();
		return;
	}
	
	if (PinName == TEXT("Start"))
	{
		if (GetFlowSubsystem())
		{
			GetFlowSubsystem()->CreateSubFlow(this);
		}
	}
	else if (!PinName.IsNone())
	{
		GetFlowAsset()->TriggerCustomEvent(this, PinName);
	}
}

void UFlowNode_SubGraph::Cleanup()
{
	if (CanBeAssetInstanced() && GetFlowSubsystem())
	{
		GetFlowSubsystem()->RemoveSubFlow(this, EFlowFinishPolicy::Keep);
	}
}

void UFlowNode_SubGraph::ForceFinishNode()
{
	TriggerFirstOutput(true);
}

void UFlowNode_SubGraph::OnLoad_Implementation()
{
	if (!SavedAssetInstanceName.IsEmpty() && !Asset.IsNull())
	{
		GetFlowSubsystem()->LoadSubFlow(this, SavedAssetInstanceName);
		SavedAssetInstanceName = FString();
	}
}

#if WITH_EDITOR
FString UFlowNode_SubGraph::GetNodeDescription() const
{
	return Asset.IsNull() ? FString() : Asset.ToSoftObjectPath().GetAssetName();
}

UObject* UFlowNode_SubGraph::GetAssetToEdit()
{
	return Asset.IsNull() ? nullptr : Asset.LoadSynchronous();
}

EDataValidationResult UFlowNode_SubGraph::ValidateNode()
{
	if (Asset.IsNull())
	{
		ValidationLog.Error<UFlowNode>(TEXT("Flow Asset not assigned or invalid!"), this);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}

TArray<FFlowPin> UFlowNode_SubGraph::GetContextInputs()
{
	TArray<FFlowPin> EventNames;

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

TArray<FFlowPin> UFlowNode_SubGraph::GetContextOutputs()
{
	TArray<FFlowPin> Pins;

	if (!Asset.IsNull())
	{
		Asset.LoadSynchronous();
		for (const FName& PinName : Asset.Get()->GetCustomOutputs())
		{
			if (!PinName.IsNone())
			{
				Pins.Emplace(PinName);
			}
		}
	}

	return Pins;
}

void UFlowNode_SubGraph::PostLoad()
{
	Super::PostLoad();

	SubscribeToAssetChanges();
}

void UFlowNode_SubGraph::PreEditChange(FProperty* PropertyAboutToChange)
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
