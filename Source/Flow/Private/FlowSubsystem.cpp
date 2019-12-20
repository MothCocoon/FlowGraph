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
	for (int32 i = InstancedAssets.Num() - 1; i >= 0; --i)
	{
		if (InstancedAssets[i] != nullptr)
		{
			InstancedAssets[i]->ClearInstances();
		}
	}
	InstancedAssets.Empty();
}

void UFlowSubsystem::StartFlow(UFlowAsset* FlowAsset)
{
	UFlowAsset* NewFlow = CreateFlowInstance(FlowAsset);
	NewFlow->StartFlow();
}

void UFlowSubsystem::PreloadSubFlow(UFlowNodeSubFlow* SubFlow)
{
	if (!InstancedSubFlows.Contains(SubFlow))
	{
		UFlowAsset* NewFlow = CreateFlowInstance(SubFlow->FlowAsset);
		InstancedSubFlows.Add(SubFlow, NewFlow);

		NewFlow->PreloadNodes();
	}
}

void UFlowSubsystem::StartSubFlow(UFlowNodeSubFlow* SubFlow)
{
	if (!InstancedSubFlows.Contains(SubFlow))
	{
		UFlowAsset* NewFlow = CreateFlowInstance(SubFlow->FlowAsset);
		InstancedSubFlows.Add(SubFlow, NewFlow);
	}

	// get instanced asset from map - in case it was already instanced by PreloadSubFlow()
	InstancedSubFlows[SubFlow]->StartSubFlow(SubFlow);
}

UFlowAsset* UFlowSubsystem::CreateFlowInstance(TSoftObjectPtr<UFlowAsset> FlowAsset)
{
	check(!FlowAsset.IsNull());

	if (FlowAsset.IsPending())
	{
		const FSoftObjectPath& AssetRef = FlowAsset.ToSoftObjectPath();
		FlowAsset = Cast<UFlowAsset>(Streamable.LoadSynchronous(AssetRef, false));
	}

	InstancedAssets.Add(FlowAsset.Get());

	UFlowAsset* NewInstance = NewObject<UFlowAsset>(this, FlowAsset->GetClass(), NAME_None, RF_Transient, FlowAsset.Get(), false, nullptr);
	NewInstance->SetSubsystem(this);

	NewInstance->CreateNodeInstances();
	FlowAsset.Get()->AddInstance(NewInstance);

	return NewInstance;
}

UWorld* UFlowSubsystem::GetWorld() const
{
	return GetGameInstance()->GetWorld();
}
