#include "FlowSubsystem.h"
#include "Graph/FlowAsset.h"
#include "Graph/Nodes/FlowNodeSubFlow.h"

#include "Engine/GameInstance.h"
#include "Misc/Paths.h"

UFlowSubsystem::UFlowSubsystem()
	: UGameInstanceSubsystem()
{
}

void UFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
}

void UFlowSubsystem::Deinitialize()
{
	for (TWeakObjectPtr<UFlowAsset> InstancedAsset : InstancedAssets)
	{
		if (InstancedAsset.IsValid())
		{
			InstancedAsset.Get()->ClearInstances();
		}
	}

	InstancedAssets.Empty();
	InstancedSubFlows.Empty();
}

void UFlowSubsystem::StartFlow(UFlowAsset* FlowAsset)
{
	UFlowAsset* NewFlow = CreateFlowInstance(FlowAsset);
	NewFlow->StartFlow();
}

void UFlowSubsystem::EndFlow(UFlowAsset* FlowAsset)
{
	// todo
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

void UFlowSubsystem::FlushPreload(UFlowNodeSubFlow* SubFlow)
{
	if (UFlowAsset* PreloadedAsset = InstancedSubFlows.FindRef(SubFlow))
	{
		PreloadedAsset->FlushPreload();
		InstancedSubFlows.Remove(SubFlow);

		const int32 ActiveInstancesLeft = SubFlow->FlowAsset.Get()->RemoveInstance(PreloadedAsset);
		if (ActiveInstancesLeft == 0)
		{
			InstancedAssets.Remove(SubFlow->FlowAsset.Get());
		}
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

	const FString NewInstanceName = FPaths::GetBaseFilename(FlowAsset.Get()->GetPathName()) + TEXT("_") + FString::FromInt(FlowAsset.Get()->GetInstancesNum());
	UFlowAsset* NewInstance = NewObject<UFlowAsset>(this, FlowAsset->GetClass(), *NewInstanceName, RF_Transient, FlowAsset.Get(), false, nullptr);
	NewInstance->InitInstance(FlowAsset.Get());

	FlowAsset.Get()->AddInstance(NewInstance);

	return NewInstance;
}

UWorld* UFlowSubsystem::GetWorld() const
{
	return GetGameInstance()->GetWorld();
}