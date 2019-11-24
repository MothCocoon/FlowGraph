#include "FlowSubsystem.h"
#include "Graph/FlowAsset.h"
#include "Graph/Nodes/FlowNodeSubFlow.h"

UFlowSubsystem::UFlowSubsystem()
	: UGameInstanceSubsystem()
{
}

void UFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
}

void UFlowSubsystem::Deinitialize()
{
	for (int32 i = ActiveFlows.Num() - 1; i >= 0; --i)
	{
		if (ActiveFlows[i] != nullptr)
		{
			ActiveFlows[i]->ClearInstances();
		}
	}
	ActiveFlows.Empty();
}

UFlowAsset* UFlowSubsystem::StartFlow(TSoftObjectPtr<UFlowAsset> FlowAsset)
{
	UFlowAsset* NewFlow = CreateFlowInstance(FlowAsset);
	NewFlow->StartFlow(this);

	return FlowAsset.Get();
}

void UFlowSubsystem::StartSubFlow(UFlowNodeSubFlow* SubFlowNode, TSoftObjectPtr<UFlowAsset> ChildAsset, UFlowAsset* ParentFlow /*= nullptr*/)
{
	UFlowAsset* NewFlow = CreateFlowInstance(ChildAsset);
	NewFlow->StartSubFlow(this, SubFlowNode, ParentFlow);
}

UFlowAsset* UFlowSubsystem::CreateFlowInstance(TSoftObjectPtr<UFlowAsset> FlowAsset)
{
	check(!FlowAsset.IsNull());

	if (FlowAsset.IsPending())
	{
		const FSoftObjectPath& AssetRef = FlowAsset.ToSoftObjectPath();
		FlowAsset = Cast<UFlowAsset>(Streamable.LoadSynchronous(AssetRef, false));
	}

	ActiveFlows.AddUnique(FlowAsset.Get());

	UFlowAsset* NewInstance = NewObject<UFlowAsset>(this, FlowAsset->GetClass(), NAME_None, RF_Transient, FlowAsset.Get(), false, nullptr);
	NewInstance->CreateNodeInstances();
	FlowAsset.Get()->AddInstance(NewInstance);

	return NewInstance;
}
