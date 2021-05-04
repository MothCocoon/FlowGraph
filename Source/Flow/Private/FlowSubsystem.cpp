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
#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

UFlowSubsystem::UFlowSubsystem()
	: UGameInstanceSubsystem()
{
}

bool UFlowSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// in this case, we simply create subsystem for every instance of the game
	if (UFlowSettings::Get()->bCreateFlowSubsystemOnClients)
	{
		return true;
	}

	return Outer->GetWorld()->GetNetMode() < NM_Client && Outer->GetWorld()->IsServer();
}

void UFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// temp for testing, game should control this
	//LoadGame();
}

void UFlowSubsystem::Deinitialize()
{
	if (InstancedTemplates.Num() > 0)
	{
		for (int32 i = InstancedTemplates.Num() - 1; i >= 0; i--)
		{
			if (InstancedTemplates.IsValidIndex(i))
			{
				InstancedTemplates[i]->ClearInstances();
			}
		}

		InstancedTemplates.Empty();
		InstancedSubFlows.Empty();
	}
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

void UFlowSubsystem::FinishRootFlow(UObject* Owner)
{
	if (UFlowAsset* Instance = RootInstances.FindRef(Owner))
	{
		RootInstances.Remove(Owner);
		Instance->FinishFlow(false);
	}
}

UFlowAsset* UFlowSubsystem::StartSubFlow(UFlowNode_SubGraph* SubGraphNode, const FString NewInstanceName, const bool bPreloading /* = false */)
{
	UFlowAsset* NewFlow = nullptr;

	if (!InstancedSubFlows.Contains(SubGraphNode))
	{
		const TWeakObjectPtr<UObject> Owner = SubGraphNode->GetFlowAsset() ? SubGraphNode->GetFlowAsset()->GetOwner() : nullptr;
		NewFlow = CreateFlowInstance(Owner, SubGraphNode->Asset, NewInstanceName);
		InstancedSubFlows.Add(SubGraphNode, NewFlow);

		if (bPreloading)
		{
			NewFlow->PreloadNodes();
		}
	}

	if (!bPreloading)
	{
		// get instanced asset from map - in case it was already instanced by calling StartSubFlow() with bPreloading == true
		InstancedSubFlows[SubGraphNode]->StartAsSubFlow(SubGraphNode);
	}

	return NewFlow;
}

void UFlowSubsystem::FinishSubFlow(UFlowNode_SubGraph* SubGraphNode)
{
	if (UFlowAsset* Instance = InstancedSubFlows.FindRef(SubGraphNode))
	{
		InstancedSubFlows.Remove(SubGraphNode);
		Instance->FinishFlow(false);
	}
}

UFlowAsset* UFlowSubsystem::CreateFlowInstance(const TWeakObjectPtr<UObject> Owner, TSoftObjectPtr<UFlowAsset> FlowAsset, FString NewInstanceName)
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

void UFlowSubsystem::SaveGame()
{
	FFlowSaveData SaveGameData;
	SaveGameData.Timestamp = FDateTime::Now();

	for (const TPair<TWeakObjectPtr<UObject>, UFlowAsset*>& Pair : RootInstances)
	{
		if (Pair.Key.IsValid() && Pair.Value)
		{
			if (UFlowComponent* FlowComponent = Cast<UFlowComponent>(Pair.Key))
			{
				SaveGameData.SavedRootFlowInstances.Emplace(FlowComponent->SaveRootFlow());
			}
			else
			{
				SaveGameData.SavedRootFlowInstances.Emplace(Pair.Value->SaveInstance());
			}
		}
	}

	TArray<TWeakObjectPtr<UFlowComponent>> ComponentsArray;
	FlowComponentRegistry.GenerateValueArray(ComponentsArray);
	const TSet<TWeakObjectPtr<UFlowComponent>> FlowComponents = TSet<TWeakObjectPtr<UFlowComponent>>(ComponentsArray);
	for (TWeakObjectPtr<UFlowComponent> FlowComponent : FlowComponents)
	{
		SaveGameData.SavedFlowComponents.Emplace(FlowComponent->SaveInstance());
	}

	FBufferArchive BinaryData;
	BinaryData << SaveGameData;

	const FString SaveFolder = GetSaveFolderDir() + FString("TestSave.sav");
	if (FFileHelper::SaveArrayToFile(BinaryData, *SaveFolder))
	{
		UE_LOG(LogFlow, Warning, TEXT("Save Success! %s"), FPlatformProcess::BaseDir());
	}
	else
	{
		UE_LOG(LogFlow, Warning, TEXT("Save Failed!"));
	}

	BinaryData.FlushCache();
	BinaryData.Empty();
}

void UFlowSubsystem::LoadGame()
{
	const FString SaveFolder = GetSaveFolderDir() + FString("TestSave.sav");
	TArray<uint8> BinaryData;

	if (FFileHelper::LoadFileToArray(BinaryData, *SaveFolder))
	{
		UE_LOG(LogFlow, Warning, TEXT("Load Succeeded!"));

		FMemoryReader FromBinary = FMemoryReader(BinaryData, true);
		FromBinary.Seek(0);
		FromBinary << LoadedSaveGame;;

		FromBinary.FlushCache();
		BinaryData.Empty();
		FromBinary.Close();
	}
	else
	{
		UE_LOG(LogFlow, Warning, TEXT("Load Failed!"));
	}
}

void UFlowSubsystem::LoadRootFlow(UObject* Owner, UFlowAsset* FlowAsset, const FString& SavedAssetInstanceName)
{
	if (SavedAssetInstanceName.IsEmpty())
	{
		return;
	}
	
	for (const FFlowAssetSaveData& AssetRecord : LoadedSaveGame.SavedRootFlowInstances)
	{
		if (AssetRecord.InstanceName == SavedAssetInstanceName)
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

void UFlowSubsystem::LoadSubFlow(UFlowNode_SubGraph* SubGraphNode, const FFlowAssetSaveData AssetRecord)
{
	UFlowAsset* LoadedInstance = StartSubFlow(SubGraphNode, AssetRecord.InstanceName);
	if (LoadedInstance)
	{
		LoadedInstance->LoadInstance(AssetRecord);
	}
}

FString UFlowSubsystem::GetSaveFolderDir()
{
	return FPaths::ProjectSavedDir() + UFlowSettings::Get()->SaveFolderName + TEXT("/");
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
	FlowComponentRegistry.Emplace(RemovedTag, Component);

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

TSet<UFlowComponent*> UFlowSubsystem::GetFlowComponentsByTag(const FGameplayTag Tag, const TSubclassOf<UFlowComponent> ComponentClass) const
{
	TArray<TWeakObjectPtr<UFlowComponent>> FoundComponents;
	FlowComponentRegistry.MultiFind(Tag, FoundComponents);

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

TSet<UFlowComponent*> UFlowSubsystem::GetFlowComponentsByTags(const FGameplayTagContainer Tags, const EGameplayContainerMatchType MatchType, const TSubclassOf<UFlowComponent> ComponentClass) const
{
	TSet<TWeakObjectPtr<UFlowComponent>> FoundComponents;
	FindComponents(Tags, FoundComponents, MatchType);

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

TSet<AActor*> UFlowSubsystem::GetFlowActorsByTag(const FGameplayTag Tag, const TSubclassOf<AActor> ActorClass) const
{
	TArray<TWeakObjectPtr<UFlowComponent>> FoundComponents;
	FlowComponentRegistry.MultiFind(Tag, FoundComponents);

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

TSet<AActor*> UFlowSubsystem::GetFlowActorsByTags(const FGameplayTagContainer Tags, const EGameplayContainerMatchType MatchType, const TSubclassOf<AActor> ActorClass) const
{
	TSet<TWeakObjectPtr<UFlowComponent>> FoundComponents;
	FindComponents(Tags, FoundComponents, MatchType);

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

TMap<AActor*, UFlowComponent*> UFlowSubsystem::GetFlowActorsAndComponentsByTag(const FGameplayTag Tag, const TSubclassOf<AActor> ActorClass) const
{
	TArray<TWeakObjectPtr<UFlowComponent>> FoundComponents;
	FlowComponentRegistry.MultiFind(Tag, FoundComponents);

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

TMap<AActor*, UFlowComponent*> UFlowSubsystem::GetFlowActorsAndComponentsByTags(const FGameplayTagContainer Tags, const EGameplayContainerMatchType MatchType, const TSubclassOf<AActor> ActorClass) const
{
	TSet<TWeakObjectPtr<UFlowComponent>> FoundComponents;
	FindComponents(Tags, FoundComponents, MatchType);

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

void UFlowSubsystem::FindComponents(const FGameplayTagContainer& Tags, TSet<TWeakObjectPtr<UFlowComponent>>& OutComponents, const EGameplayContainerMatchType MatchType) const
{
	if (MatchType == EGameplayContainerMatchType::Any)
	{
		for (const FGameplayTag& Tag : Tags)
		{
			TArray<TWeakObjectPtr<UFlowComponent>> ComponentsPerTag;
			FlowComponentRegistry.MultiFind(Tag, ComponentsPerTag);
			OutComponents.Append(ComponentsPerTag);
		}
	}
	else // EGameplayContainerMatchType::All
	{
		TSet<TWeakObjectPtr<UFlowComponent>> ComponentsWithAnyTag;
		for (const FGameplayTag& Tag : Tags)
		{
			TArray<TWeakObjectPtr<UFlowComponent>> ComponentsPerTag;
			FlowComponentRegistry.MultiFind(Tag, ComponentsPerTag);
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
