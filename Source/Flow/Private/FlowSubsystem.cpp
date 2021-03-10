#include "FlowSubsystem.h"
#include "FlowAsset.h"
#include "FlowComponent.h"
#include "FlowModule.h"
#include "Nodes/Route/FlowNode_SubGraph.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
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
	for (int32 i = InstancedTemplates.Num() - 1; i >= 0; i--)
	{
		if (InstancedTemplates[i])
		{
			InstancedTemplates[i]->ClearInstances();
		}
	}

	InstancedTemplates.Empty();
	InstancedSubFlows.Empty();
}

void UFlowSubsystem::StartRootFlow(UObject* Owner, UFlowAsset* FlowAsset)
{
	if (RootInstances.Contains(Owner))
	{
		UE_LOG(LogFlow, Warning, TEXT("Attempted to start Root Flow again. Owner: %s. Flow Asset: %s."), *Owner->GetName(), *FlowAsset->GetName());
		return;
	}

	UFlowAsset* NewFlow = CreateFlowInstance(FlowAsset);
	RootInstances.Add(Owner, NewFlow);

	NewFlow->StartFlow();
}

void UFlowSubsystem::FinishRootFlow(UObject* Owner, UFlowAsset* FlowAsset)
{
	if (UFlowAsset* Instance = RootInstances.FindRef(Owner))
	{
		RootInstances.Remove(FlowAsset);
		Instance->FinishFlow(false);
	}
}

void UFlowSubsystem::PreloadSubFlow(UFlowNode_SubGraph* SubFlow)
{
	if (!InstancedSubFlows.Contains(SubFlow))
	{
		UFlowAsset* NewFlow = CreateFlowInstance(SubFlow->Asset);
		InstancedSubFlows.Add(SubFlow, NewFlow);

		NewFlow->PreloadNodes();
	}
}

void UFlowSubsystem::StartSubFlow(UFlowNode_SubGraph* SubFlow)
{
	if (!InstancedSubFlows.Contains(SubFlow))
	{
		UFlowAsset* NewFlow = CreateFlowInstance(SubFlow->Asset);
		InstancedSubFlows.Add(SubFlow, NewFlow);
	}

	// get instanced asset from map - in case it was already instanced by PreloadSubFlow()
	InstancedSubFlows[SubFlow]->StartAsSubFlow(SubFlow);
}

void UFlowSubsystem::FinishSubFlow(UFlowNode_SubGraph* SubFlow)
{
	if (UFlowAsset* Instance = InstancedSubFlows.FindRef(SubFlow))
	{
		InstancedSubFlows.Remove(SubFlow);
		Instance->FinishFlow(false);
	}
}

void UFlowSubsystem::RemoveInstancedTemplate(UFlowAsset* Template)
{
	InstancedTemplates.Remove(Template);
}

UFlowAsset* UFlowSubsystem::CreateFlowInstance(TSoftObjectPtr<UFlowAsset> FlowAsset)
{
	check(!FlowAsset.IsNull());

	if (FlowAsset.IsPending())
	{
		const FSoftObjectPath& AssetRef = FlowAsset.ToSoftObjectPath();
		FlowAsset = Cast<UFlowAsset>(Streamable.LoadSynchronous(AssetRef, false));
	}

	InstancedTemplates.Add(FlowAsset.Get());

#if WITH_EDITOR
	if (GetWorld()->WorldType != EWorldType::Game)
	{
		// Fix connections - even in packaged game if assets haven't been re-saved in the editor after changing node's definition
		FlowAsset.Get()->HarvestNodeConnections();
	}
#endif

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

void UFlowSubsystem::RegisterComponent(UFlowComponent* Component)
{
	for (const FGameplayTag& Tag : Component->IdentityTags)
	{
		FlowComponents.Emplace(Tag, Component);
	}

	OnComponentRegistered.Broadcast(Component);
}

void UFlowSubsystem::UnregisterComponent(UFlowComponent* Component)
{
	for (const FGameplayTag& Tag : Component->IdentityTags)
	{
		FlowComponents.Remove(Tag, Component);
	}

	OnComponentUnregistered.Broadcast(Component);
}

TArray<UFlowComponent*> UFlowSubsystem::GetFlowComponents(const FGameplayTag& Tag) const
{
	TArray<TWeakObjectPtr<UFlowComponent>> FoundComponents;
	FlowComponents.MultiFind(Tag, FoundComponents);

	TArray<UFlowComponent*> ResultComponents;
	for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
	{
		if (Component.IsValid())
		{
			ResultComponents.Emplace(Component.Get());
		}
	}

	return ResultComponents;
}

TArray<AActor*> UFlowSubsystem::GetFlowActors(const FGameplayTag& Tag) const
{
	TArray<TWeakObjectPtr<UFlowComponent>> FoundComponents;
	FlowComponents.MultiFind(Tag, FoundComponents);

	TArray<AActor*> ResultActors;
	for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
	{
		if (Component.IsValid())
		{
			ResultActors.Emplace(Component->GetOwner());
		}
	}

	return ResultActors;
}

TMap<AActor*, UFlowComponent*> UFlowSubsystem::GetFlowActorsAndComponents(const FGameplayTag& Tag) const
{
	TArray<TWeakObjectPtr<UFlowComponent>> FoundComponents;
	FlowComponents.MultiFind(Tag, FoundComponents);

	TMap<AActor*, UFlowComponent*> ResultActors;
	for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
	{
		if (Component.IsValid())
		{
			ResultActors.Emplace(Component->GetOwner(), Component.Get());
		}
	}

	return ResultActors;
}
