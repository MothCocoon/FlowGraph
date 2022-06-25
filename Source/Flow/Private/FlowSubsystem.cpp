// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowSubsystem.h"

#include "FlowAsset.h"
#include "FlowComponent.h"
#include "FlowModule.h"
#include "FlowSave.h"
#include "FlowSettings.h"
#include "Nodes/Route/FlowNode_SubGraph.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectHash.h"

UFlowSubsystem::UFlowSubsystem()
	: UGameInstanceSubsystem()
{
}

bool UFlowSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Only create an instance if there is no override implementation defined elsewhere
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);
	if (ChildClasses.Num() > 0)
	{
		return false;
	}

	// in this case, we simply create subsystem for every instance of the game
	if (UFlowSettings::Get()->bCreateFlowSubsystemOnClients)
	{
		return true;
	}

	return Outer->GetWorld()->GetNetMode() < NM_Client;
}

void UFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
}

void UFlowSubsystem::Deinitialize()
{
	AbortActiveFlows();
}

void UFlowSubsystem::AbortActiveFlows()
{
	if (InstancedTemplates.Num() > 0)
	{
		for (int32 i = InstancedTemplates.Num() - 1; i >= 0; i--)
		{
			if (InstancedTemplates.IsValidIndex(i) && InstancedTemplates[i])
			{
				InstancedTemplates[i]->ClearInstances();
			}
		}
	}

	InstancedTemplates.Empty();
	InstancedSubFlows.Empty();

	RootInstances.Empty();
}

void UFlowSubsystem::StartRootFlow(UObject* Owner, UFlowAsset* FlowAsset, const bool bAllowMultipleInstances /* = true */)
{
	UFlowAsset* NewFlow = CreateRootFlow(Owner, FlowAsset, bAllowMultipleInstances);
	if (NewFlow)
	{
		NewFlow->StartFlow();
	}
}

UFlowAsset* UFlowSubsystem::CreateRootFlow(UObject* Owner, UFlowAsset* FlowAsset, const bool bAllowMultipleInstances)
{
	if (RootInstances.Contains(Owner))
	{
		UE_LOG(LogFlow, Warning, TEXT("Attempted to start Root Flow for the same Owner again. Owner: %s. Flow Asset: %s."), *Owner->GetName(), *FlowAsset->GetName());
		return nullptr;
	}

	if (!bAllowMultipleInstances && InstancedTemplates.Contains(FlowAsset))
	{
		UE_LOG(LogFlow, Warning, TEXT("Attempted to start Root Flow, although there can be only a single instance. Owner: %s. Flow Asset: %s."), *Owner->GetName(), *FlowAsset->GetName());
		return nullptr;
	}

	UFlowAsset* NewFlow = CreateFlowInstance(Owner, FlowAsset);
	RootInstances.Add(Owner, NewFlow);

	return NewFlow;
}

void UFlowSubsystem::FinishRootFlow(UObject* Owner, const EFlowFinishPolicy FinishPolicy)
{
	if (UFlowAsset* Instance = RootInstances.FindRef(Owner))
	{
		RootInstances.Remove(Owner);
		Instance->FinishFlow(FinishPolicy);
	}
}

UFlowAsset* UFlowSubsystem::CreateSubFlow(UFlowNode_SubGraph* SubGraphNode, const FString SavedInstanceName, const bool bPreloading /* = false */)
{
	UFlowAsset* NewInstance = nullptr;

	if (!InstancedSubFlows.Contains(SubGraphNode))
	{
		const TWeakObjectPtr<UObject> Owner = SubGraphNode->GetFlowAsset() ? SubGraphNode->GetFlowAsset()->GetOwner() : nullptr;
		NewInstance = CreateFlowInstance(Owner, SubGraphNode->Asset, SavedInstanceName);
		InstancedSubFlows.Add(SubGraphNode, NewInstance);

		if (bPreloading)
		{
			NewInstance->PreloadNodes();
		}
	}

	if (!bPreloading)
	{
		// get instanced asset from map - in case it was already instanced by calling CreateSubFlow() with bPreloading == true
		UFlowAsset* AssetInstance = InstancedSubFlows[SubGraphNode];

		AssetInstance->NodeOwningThisAssetInstance = SubGraphNode;
		SubGraphNode->GetFlowAsset()->ActiveSubGraphs.Add(SubGraphNode, AssetInstance);

		// don't activate Start Node if we're loading Sub Graph from SaveGame
		if (SavedInstanceName.IsEmpty())
		{
			AssetInstance->StartFlow();
		}
	}

	return NewInstance;
}

void UFlowSubsystem::RemoveSubFlow(UFlowNode_SubGraph* SubGraphNode, const EFlowFinishPolicy FinishPolicy)
{
	if (InstancedSubFlows.Contains(SubGraphNode))
	{
		UFlowAsset* AssetInstance = InstancedSubFlows[SubGraphNode];
		AssetInstance->NodeOwningThisAssetInstance = nullptr;

		SubGraphNode->GetFlowAsset()->ActiveSubGraphs.Remove(SubGraphNode);
		InstancedSubFlows.Remove(SubGraphNode);

		AssetInstance->FinishFlow(FinishPolicy);
	}
}

UFlowAsset* UFlowSubsystem::CreateFlowInstance(const TWeakObjectPtr<UObject> Owner, TSoftObjectPtr<UFlowAsset> FlowAsset, FString NewInstanceName)
{
	check(!FlowAsset.IsNull());

	if (FlowAsset.IsPending() || !FlowAsset.IsValid())
	{
		FlowAsset = Cast<UFlowAsset>(Streamable.LoadSynchronous(FlowAsset.ToSoftObjectPath(), false));
	}

	InstancedTemplates.Add(FlowAsset.Get());

#if WITH_EDITOR
	if (GetWorld()->WorldType != EWorldType::Game)
	{
		// Fix connections - even in packaged game if assets haven't been re-saved in the editor after changing node's definition
		FlowAsset.Get()->HarvestNodeConnections();
	}
#endif

	// it won't be empty, if we're restoring Flow Asset instance from the SaveGame
	if (NewInstanceName.IsEmpty())
	{
		NewInstanceName = FPaths::GetBaseFilename(FlowAsset.Get()->GetPathName()) + TEXT("_") + FString::FromInt(FlowAsset.Get()->GetInstancesNum());
	}

	UFlowAsset* NewInstance = NewObject<UFlowAsset>(this, FlowAsset->GetClass(), *NewInstanceName, RF_Transient, FlowAsset.Get(), false, nullptr);
	NewInstance->InitializeInstance(Owner, FlowAsset.Get());

	FlowAsset.Get()->AddInstance(NewInstance);

	return NewInstance;
}

void UFlowSubsystem::RemoveInstancedTemplate(UFlowAsset* Template)
{
	InstancedTemplates.Remove(Template);
}

TMap<UObject*, UFlowAsset*> UFlowSubsystem::GetRootInstances() const
{
	TMap<UObject*, UFlowAsset*> Result;
	for (const TPair<TWeakObjectPtr<UObject>, UFlowAsset*>& Pair : RootInstances)
	{
		Result.Emplace(Pair.Key.Get(), Pair.Value);
	}
	return Result;
}

UWorld* UFlowSubsystem::GetWorld() const
{
	return GetGameInstance()->GetWorld();
}

void UFlowSubsystem::OnGameSaved(UFlowSaveGame* SaveGame)
{
	// clear existing data, in case we received reused SaveGame instance
	// we only remove data for the current world + global Flow Graph instances (i.e. not bound to any world if created by UGameInstanceSubsystem)
	// we keep data bound to other worlds
	if (GetWorld())
	{
		const FString& WorldName = GetWorld()->GetName();

		for (int32 i = SaveGame->FlowInstances.Num() - 1; i >= 0; i--)
		{
			if (SaveGame->FlowInstances[i].WorldName.IsEmpty() || SaveGame->FlowInstances[i].WorldName == WorldName)
			{
				SaveGame->FlowInstances.RemoveAt(i);
			}
		}

		for (int32 i = SaveGame->FlowComponents.Num() - 1; i >= 0; i--)
		{
			if (SaveGame->FlowComponents[i].WorldName.IsEmpty() || SaveGame->FlowComponents[i].WorldName == WorldName)
			{
				SaveGame->FlowComponents.RemoveAt(i);
			}
		}
	}

	// save Flow Graphs
	for (const TPair<TWeakObjectPtr<UObject>, UFlowAsset*>& Pair : RootInstances)
	{
		if (Pair.Key.IsValid() && Pair.Value)
		{
			if (UFlowComponent* FlowComponent = Cast<UFlowComponent>(Pair.Key))
			{
				FlowComponent->SaveRootFlow(SaveGame->FlowInstances);
			}
			else
			{
				Pair.Value->SaveInstance(SaveGame->FlowInstances);
			}
		}
	}

	// save Flow Components
	{
		// retrieve all registered components
		TArray<TWeakObjectPtr<UFlowComponent>> ComponentsArray;
		FlowComponentRegistry.GenerateValueArray(ComponentsArray);

		// ensure uniqueness of entries
		const TSet<TWeakObjectPtr<UFlowComponent>> RegisteredComponents = TSet<TWeakObjectPtr<UFlowComponent>>(ComponentsArray);

		// write archives to SaveGame
		for (const TWeakObjectPtr<UFlowComponent> RegisteredComponent : RegisteredComponents)
		{
			SaveGame->FlowComponents.Emplace(RegisteredComponent->SaveInstance());
		}
	}
}

void UFlowSubsystem::OnGameLoaded(UFlowSaveGame* SaveGame)
{
	LoadedSaveGame = SaveGame;
}

void UFlowSubsystem::LoadRootFlow(UObject* Owner, UFlowAsset* FlowAsset, const FString& SavedAssetInstanceName)
{
	if (FlowAsset == nullptr || SavedAssetInstanceName.IsEmpty())
	{
		return;
	}

	for (const FFlowAssetSaveData& AssetRecord : LoadedSaveGame->FlowInstances)
	{
		if (AssetRecord.InstanceName == SavedAssetInstanceName
			&& (FlowAsset->IsBoundToWorld() == false || AssetRecord.WorldName == GetWorld()->GetName()))
		{
			UFlowAsset* LoadedInstance = CreateRootFlow(Owner, FlowAsset, false);
			if (LoadedInstance)
			{
				LoadedInstance->LoadInstance(AssetRecord);
			}
			return;
		}
	}
}

void UFlowSubsystem::LoadSubFlow(UFlowNode_SubGraph* SubGraphNode, const FString& SavedAssetInstanceName)
{
	if (SubGraphNode->Asset.IsNull())
	{
		return;
	}

	if (SubGraphNode->Asset.IsPending())
	{
		const FSoftObjectPath& AssetRef = SubGraphNode->Asset.ToSoftObjectPath();
		Streamable.LoadSynchronous(AssetRef, false);
	}

	for (const FFlowAssetSaveData& AssetRecord : LoadedSaveGame->FlowInstances)
	{
		if (AssetRecord.InstanceName == SavedAssetInstanceName
			&& ((SubGraphNode->Asset && SubGraphNode->Asset->IsBoundToWorld() == false) || AssetRecord.WorldName == GetWorld()->GetName()))
		{
			UFlowAsset* LoadedInstance = CreateSubFlow(SubGraphNode, SavedAssetInstanceName);
			if (LoadedInstance)
			{
				LoadedInstance->LoadInstance(AssetRecord);
			}
			return;
		}
	}
}

void UFlowSubsystem::RegisterComponent(UFlowComponent* Component)
{
	for (const FGameplayTag& Tag : Component->IdentityTags)
	{
		if (Tag.IsValid())
		{
			FlowComponentRegistry.Emplace(Tag, Component);
		}
	}

	OnComponentRegistered.Broadcast(Component);
}

void UFlowSubsystem::OnIdentityTagAdded(UFlowComponent* Component, const FGameplayTag& AddedTag)
{
	FlowComponentRegistry.Emplace(AddedTag, Component);

	// broadcast OnComponentRegistered only if this component wasn't present in the registry previously
	if (Component->IdentityTags.Num() > 1)
	{
		OnComponentTagAdded.Broadcast(Component, FGameplayTagContainer(AddedTag));
	}
	else
	{
		OnComponentRegistered.Broadcast(Component);
	}
}

void UFlowSubsystem::OnIdentityTagsAdded(UFlowComponent* Component, const FGameplayTagContainer& AddedTags)
{
	for (const FGameplayTag& Tag : AddedTags)
	{
		FlowComponentRegistry.Emplace(Tag, Component);
	}

	// broadcast OnComponentRegistered only if this component wasn't present in the registry previously
	if (Component->IdentityTags.Num() > AddedTags.Num())
	{
		OnComponentTagAdded.Broadcast(Component, AddedTags);
	}
	else
	{
		OnComponentRegistered.Broadcast(Component);
	}
}

void UFlowSubsystem::UnregisterComponent(UFlowComponent* Component)
{
	for (const FGameplayTag& Tag : Component->IdentityTags)
	{
		if (Tag.IsValid())
		{
			FlowComponentRegistry.Remove(Tag, Component);
		}
	}

	OnComponentUnregistered.Broadcast(Component);
}

void UFlowSubsystem::OnIdentityTagRemoved(UFlowComponent* Component, const FGameplayTag& RemovedTag)
{
	FlowComponentRegistry.Remove(RemovedTag, Component);

	// broadcast OnComponentUnregistered only if this component isn't present in the registry anymore
	if (Component->IdentityTags.Num() > 0)
	{
		OnComponentTagRemoved.Broadcast(Component, FGameplayTagContainer(RemovedTag));
	}
	else
	{
		OnComponentUnregistered.Broadcast(Component);
	}
}

void UFlowSubsystem::OnIdentityTagsRemoved(UFlowComponent* Component, const FGameplayTagContainer& RemovedTags)
{
	for (const FGameplayTag& Tag : RemovedTags)
	{
		FlowComponentRegistry.Remove(Tag, Component);
	}

	// broadcast OnComponentUnregistered only if this component isn't present in the registry anymore
	if (Component->IdentityTags.Num() > 0)
	{
		OnComponentTagRemoved.Broadcast(Component, RemovedTags);
	}
	else
	{
		OnComponentUnregistered.Broadcast(Component);
	}
}

TSet<UFlowComponent*> UFlowSubsystem::GetFlowComponentsByTag(const FGameplayTag Tag, const TSubclassOf<UFlowComponent> ComponentClass, const bool bExactMatch) const
{
	TArray<TWeakObjectPtr<UFlowComponent>> FoundComponents;
	FindComponents(Tag, bExactMatch, FoundComponents);

	TSet<UFlowComponent*> Result;
	for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
	{
		if (Component.IsValid() && Component->GetClass()->IsChildOf(ComponentClass))
		{
			Result.Emplace(Component.Get());
		}
	}

	return Result;
}

TSet<UFlowComponent*> UFlowSubsystem::GetFlowComponentsByTags(const FGameplayTagContainer Tags, const EGameplayContainerMatchType MatchType, const TSubclassOf<UFlowComponent> ComponentClass, const bool bExactMatch) const
{
	TSet<TWeakObjectPtr<UFlowComponent>> FoundComponents;
	FindComponents(Tags, MatchType, bExactMatch, FoundComponents);

	TSet<UFlowComponent*> Result;
	for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
	{
		if (Component.IsValid() && Component->GetClass()->IsChildOf(ComponentClass))
		{
			Result.Emplace(Component.Get());
		}
	}

	return Result;
}

TSet<AActor*> UFlowSubsystem::GetFlowActorsByTag(const FGameplayTag Tag, const TSubclassOf<AActor> ActorClass, const bool bExactMatch) const
{
	TArray<TWeakObjectPtr<UFlowComponent>> FoundComponents;
	FindComponents(Tag, bExactMatch, FoundComponents);

	TSet<AActor*> Result;
	for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
	{
		if (Component.IsValid() && Component->GetOwner()->GetClass()->IsChildOf(ActorClass))
		{
			Result.Emplace(Component->GetOwner());
		}
	}

	return Result;
}

TSet<AActor*> UFlowSubsystem::GetFlowActorsByTags(const FGameplayTagContainer Tags, const EGameplayContainerMatchType MatchType, const TSubclassOf<AActor> ActorClass, const bool bExactMatch) const
{
	TSet<TWeakObjectPtr<UFlowComponent>> FoundComponents;
	FindComponents(Tags, MatchType, bExactMatch, FoundComponents);

	TSet<AActor*> Result;
	for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
	{
		if (Component.IsValid() && Component->GetOwner()->GetClass()->IsChildOf(ActorClass))
		{
			Result.Emplace(Component->GetOwner());
		}
	}

	return Result;
}

TMap<AActor*, UFlowComponent*> UFlowSubsystem::GetFlowActorsAndComponentsByTag(const FGameplayTag Tag, const TSubclassOf<AActor> ActorClass, const bool bExactMatch) const
{
	TArray<TWeakObjectPtr<UFlowComponent>> FoundComponents;
	FindComponents(Tag, bExactMatch, FoundComponents);

	TMap<AActor*, UFlowComponent*> Result;
	for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
	{
		if (Component.IsValid() && Component->GetOwner()->GetClass()->IsChildOf(ActorClass))
		{
			Result.Emplace(Component->GetOwner(), Component.Get());
		}
	}

	return Result;
}

TMap<AActor*, UFlowComponent*> UFlowSubsystem::GetFlowActorsAndComponentsByTags(const FGameplayTagContainer Tags, const EGameplayContainerMatchType MatchType, const TSubclassOf<AActor> ActorClass, const bool bExactMatch) const
{
	TSet<TWeakObjectPtr<UFlowComponent>> FoundComponents;
	FindComponents(Tags, MatchType, bExactMatch, FoundComponents);

	TMap<AActor*, UFlowComponent*> Result;
	for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
	{
		if (Component.IsValid() && Component->GetOwner()->GetClass()->IsChildOf(ActorClass))
		{
			Result.Emplace(Component->GetOwner(), Component.Get());
		}
	}

	return Result;
}

void UFlowSubsystem::FindComponents(const FGameplayTag& Tag, const bool bExactMatch, TArray<TWeakObjectPtr<UFlowComponent>>& OutComponents) const
{
	if (bExactMatch)
	{
		FlowComponentRegistry.MultiFind(Tag, OutComponents);
	}
	else
	{
		for (TMultiMap<FGameplayTag, TWeakObjectPtr<UFlowComponent>>::TConstIterator It(FlowComponentRegistry); It; ++It)
		{
			if (It.Key().MatchesTag(Tag))
			{
				OutComponents.Emplace(It.Value());
			}
		}
	}
}

void UFlowSubsystem::FindComponents(const FGameplayTagContainer& Tags, const EGameplayContainerMatchType MatchType, const bool bExactMatch, TSet<TWeakObjectPtr<UFlowComponent>>& OutComponents) const
{
	if (MatchType == EGameplayContainerMatchType::Any)
	{
		for (const FGameplayTag& Tag : Tags)
		{
			TArray<TWeakObjectPtr<UFlowComponent>> ComponentsPerTag;
			FindComponents(Tag, bExactMatch, ComponentsPerTag);
			OutComponents.Append(ComponentsPerTag);
		}
	}
	else // EGameplayContainerMatchType::All
	{
		TSet<TWeakObjectPtr<UFlowComponent>> ComponentsWithAnyTag;
		for (const FGameplayTag& Tag : Tags)
		{
			TArray<TWeakObjectPtr<UFlowComponent>> ComponentsPerTag;
			FindComponents(Tag, bExactMatch, ComponentsPerTag);
			ComponentsWithAnyTag.Append(ComponentsPerTag);
		}

		for (const TWeakObjectPtr<UFlowComponent>& Component : ComponentsWithAnyTag)
		{
			if (Component.IsValid() && Component->IdentityTags.HasAllExact(Tags))
			{
				OutComponents.Emplace(Component);
			}
		}
	}
}
