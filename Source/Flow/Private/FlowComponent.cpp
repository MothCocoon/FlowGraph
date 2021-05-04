#include "FlowComponent.h"

#include "FlowAsset.h"
#include "FlowSettings.h"
#include "FlowSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

UFlowComponent::UFlowComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, RootFlow(nullptr)
	, bAutoStartRootFlow(true)
	, RootFlowMode(EFlowNetMode::Authority)
	, bAllowMultipleInstances(true)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);
}

void UFlowComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UFlowComponent, AddedIdentityTags);
	DOREPLIFETIME(UFlowComponent, RemovedIdentityTags);

	DOREPLIFETIME(UFlowComponent, RecentlySentNotifyTags);
	DOREPLIFETIME(UFlowComponent, NotifyTagsFromGraph);
	DOREPLIFETIME(UFlowComponent, NotifyTagsFromAnotherComponent);
}

void UFlowComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
	{
		bool bComponentLoadedFromSaveGame = false;
		if (GetFlowSubsystem()->IsSaveGameLoaded())
		{
			// temp hack, checking if Flow Component is inside World Settings
			if (bAllowMultipleInstances == false)
			{
				bComponentLoadedFromSaveGame = LoadInstance();
			}
		}

		FlowSubsystem->RegisterComponent(this);

		if (RootFlow)
		{
			if (bComponentLoadedFromSaveGame)
			{
				LoadRootFlow();
			}
			else
			{
				if (bAutoStartRootFlow)
				{
					StartRootFlow();
				}
			}
		}
	}
}

void UFlowComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
	{
		FlowSubsystem->FinishRootFlow(this);
		FlowSubsystem->UnregisterComponent(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UFlowComponent::AddIdentityTag(const FGameplayTag Tag, const EFlowNetMode NetMode /* = EFlowNetMode::Authority*/)
{
	if (IsFlowNetMode(NetMode) && Tag.IsValid() && !IdentityTags.HasTagExact(Tag))
	{
		IdentityTags.AddTag(Tag);

		if (HasBegunPlay())
		{
			OnIdentityTagsAdded.Broadcast(this, FGameplayTagContainer(Tag));

			if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
			{
				FlowSubsystem->OnIdentityTagAdded(this, Tag);
			}

			if (IsNetMode(NM_DedicatedServer) || IsNetMode(NM_ListenServer))
			{
				AddedIdentityTags = FGameplayTagContainer(Tag);
			}
		}
	}
}

void UFlowComponent::AddIdentityTags(FGameplayTagContainer Tags, const EFlowNetMode NetMode /* = EFlowNetMode::Authority*/)
{
	if (IsFlowNetMode(NetMode) && Tags.IsValid())
	{
		FGameplayTagContainer ValidatedTags;

		for (const FGameplayTag& Tag : Tags)
		{
			if (Tag.IsValid() && !IdentityTags.HasTagExact(Tag))
			{
				IdentityTags.AddTag(Tag);
				ValidatedTags.AddTag(Tag);
			}
		}

		if (ValidatedTags.Num() > 0 && HasBegunPlay())
		{
			OnIdentityTagsAdded.Broadcast(this, ValidatedTags);

			if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
			{
				FlowSubsystem->OnIdentityTagsAdded(this, ValidatedTags);
			}

			if (IsNetMode(NM_DedicatedServer) || IsNetMode(NM_ListenServer))
			{
				AddedIdentityTags = ValidatedTags;
			}
		}
	}
}

void UFlowComponent::RemoveIdentityTag(const FGameplayTag Tag, const EFlowNetMode NetMode /* = EFlowNetMode::Authority*/)
{
	if (IsFlowNetMode(NetMode) && Tag.IsValid() && IdentityTags.HasTagExact(Tag))
	{
		IdentityTags.RemoveTag(Tag);

		if (HasBegunPlay())
		{
			OnIdentityTagsRemoved.Broadcast(this, FGameplayTagContainer(Tag));

			if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
			{
				FlowSubsystem->OnIdentityTagRemoved(this, Tag);
			}

			if (IsNetMode(NM_DedicatedServer) || IsNetMode(NM_ListenServer))
			{
				RemovedIdentityTags = FGameplayTagContainer(Tag);
			}
		}
	}
}

void UFlowComponent::RemoveIdentityTags(FGameplayTagContainer Tags, const EFlowNetMode NetMode /* = EFlowNetMode::Authority*/)
{
	if (IsFlowNetMode(NetMode) && Tags.IsValid())
	{
		FGameplayTagContainer ValidatedTags;

		for (const FGameplayTag& Tag : Tags)
		{
			if (Tag.IsValid() && IdentityTags.HasTagExact(Tag))
			{
				IdentityTags.RemoveTag(Tag);
				ValidatedTags.AddTag(Tag);
			}
		}

		if (ValidatedTags.Num() > 0 && HasBegunPlay())
		{
			OnIdentityTagsRemoved.Broadcast(this, ValidatedTags);

			if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
			{
				FlowSubsystem->OnIdentityTagsRemoved(this, ValidatedTags);
			}

			if (IsNetMode(NM_DedicatedServer) || IsNetMode(NM_ListenServer))
			{
				RemovedIdentityTags = ValidatedTags;
			}
		}
	}
}

void UFlowComponent::OnRep_AddedIdentityTags()
{
	IdentityTags.AppendTags(AddedIdentityTags);
	OnIdentityTagsAdded.Broadcast(this, AddedIdentityTags);

	if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
	{
		FlowSubsystem->OnIdentityTagsAdded(this, AddedIdentityTags);
	}
}

void UFlowComponent::OnRep_RemovedIdentityTags()
{
	IdentityTags.RemoveTags(RemovedIdentityTags);
	OnIdentityTagsRemoved.Broadcast(this, RemovedIdentityTags);

	if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
	{
		FlowSubsystem->OnIdentityTagsRemoved(this, RemovedIdentityTags);
	}
}

void UFlowComponent::NotifyGraph(const FGameplayTag NotifyTag, const EFlowNetMode NetMode /* = EFlowNetMode::Authority*/)
{
	if (IsFlowNetMode(NetMode) && NotifyTag.IsValid() && HasBegunPlay())
	{
		// save recently notify, this allow for the retroactive check in nodes
		// if retroactive check wouldn't be performed, this is only used by the network replication
		RecentlySentNotifyTags = FGameplayTagContainer(NotifyTag);

		OnRep_SentNotifyTags();
	}
}

void UFlowComponent::BulkNotifyGraph(const FGameplayTagContainer NotifyTags, const EFlowNetMode NetMode /* = EFlowNetMode::Authority*/)
{
	if (IsFlowNetMode(NetMode) && NotifyTags.IsValid() && HasBegunPlay())
	{
		FGameplayTagContainer ValidatedTags;
		for (const FGameplayTag& Tag : NotifyTags)
		{
			if (Tag.IsValid())
			{
				ValidatedTags.AddTag(Tag);
			}
		}

		if (ValidatedTags.Num() > 0)
		{
			// save recently notify, this allow for the retroactive check in nodes
			// if retroactive check wouldn't be performed, this is only used by the network replication
			RecentlySentNotifyTags = NotifyTags;

			OnRep_SentNotifyTags();
		}
	}
}

void UFlowComponent::OnRep_SentNotifyTags()
{
	for (const FGameplayTag& NotifyTag : RecentlySentNotifyTags)
	{
		OnNotifyFromComponent.Broadcast(this, NotifyTag);
	}
}

void UFlowComponent::NotifyFromGraph(const FGameplayTagContainer& NotifyTags, const EFlowNetMode NetMode /* = EFlowNetMode::Authority*/)
{
	if (IsFlowNetMode(NetMode) && NotifyTags.IsValid() && HasBegunPlay())
	{
		FGameplayTagContainer ValidatedTags;
		for (const FGameplayTag& Tag : NotifyTags)
		{
			if (Tag.IsValid())
			{
				ValidatedTags.AddTag(Tag);
			}
		}

		if (ValidatedTags.Num() > 0)
		{
			for (const FGameplayTag& NotifyTag : NotifyTags)
			{
				ReceiveNotify.Broadcast(nullptr, NotifyTag);
			}

			if (IsNetMode(NM_DedicatedServer) || IsNetMode(NM_ListenServer))
			{
				NotifyTagsFromGraph = NotifyTags;
			}
		}
	}
}

void UFlowComponent::OnRep_NotifyTagsFromGraph()
{
	for (const FGameplayTag& NotifyTag : NotifyTagsFromGraph)
	{
		ReceiveNotify.Broadcast(nullptr, NotifyTag);
	}
}

void UFlowComponent::NotifyActor(const FGameplayTag ActorTag, const FGameplayTag NotifyTag, const EFlowNetMode NetMode /* = EFlowNetMode::Authority*/)
{
	if (IsFlowNetMode(NetMode) && NotifyTag.IsValid() && HasBegunPlay())
	{
		if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
		{
			for (TWeakObjectPtr<UFlowComponent>& Component : FlowSubsystem->GetComponents<UFlowComponent>(ActorTag))
			{
				Component->ReceiveNotify.Broadcast(this, NotifyTag);
			}
		}

		if (IsNetMode(NM_DedicatedServer) || IsNetMode(NM_ListenServer))
		{
			NotifyTagsFromAnotherComponent.Empty();
			NotifyTagsFromAnotherComponent.Add(FNotifyTagReplication(ActorTag, NotifyTag));
		}
	}
}

void UFlowComponent::OnRep_NotifyTagsFromAnotherComponent()
{
	if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
	{
		for (const FNotifyTagReplication& Notify : NotifyTagsFromAnotherComponent)
		{
			for (TWeakObjectPtr<UFlowComponent>& Component : FlowSubsystem->GetComponents<UFlowComponent>(Notify.ActorTag))
			{
				Component->ReceiveNotify.Broadcast(this, Notify.NotifyTag);
			}
		}
	}
}

void UFlowComponent::StartRootFlow()
{
	if (RootFlow && IsFlowNetMode(RootFlowMode))
	{
		if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
		{
			FlowSubsystem->StartRootFlow(this, RootFlow, bAllowMultipleInstances);
		}
	}
}

void UFlowComponent::FinishRootFlow()
{
	if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
	{
		FlowSubsystem->FinishRootFlow(this);
	}
}

UFlowAsset* UFlowComponent::GetRootFlowInstance()
{
	if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
	{
		return FlowSubsystem->GetRootFlow(this);
	}

	return nullptr;
}

FFlowAssetSaveData UFlowComponent::SaveRootFlow()
{
	SavedAssetInstance = FFlowAssetSaveData();

	if (UFlowAsset* FlowAssetInstance = GetRootFlowInstance())
	{
		SavedAssetInstance = FlowAssetInstance->SaveInstance();
	}

	return SavedAssetInstance;
}

void UFlowComponent::LoadRootFlow()
{
	if (RootFlow && GetFlowSubsystem())
	{
		GetFlowSubsystem()->LoadRootFlow(this, RootFlow, SavedAssetInstance);
	}
}

FFlowComponentSaveData UFlowComponent::SaveInstance()
{
	FFlowComponentSaveData ComponentRecord;
	PrepareSaveData();

	FMemoryWriter MemoryWriter(ComponentRecord.ComponentData, true);
	FFlowArchive Ar(MemoryWriter);
	Serialize(Ar);

	return ComponentRecord;
}

bool UFlowComponent::LoadInstance()
{
	const FFlowSaveData SaveData = GetFlowSubsystem()->GetLoadedSaveGame();
	if (SaveData.SavedFlowComponents.Num() > 0)
	{
		// temp hack, checking if Flow Component is inside World Settings
		const FFlowComponentSaveData& ComponentRecord = SaveData.SavedFlowComponents[0];

		FMemoryReader MemoryReader(ComponentRecord.ComponentData, true);
		FFlowArchive Ar(MemoryReader);
		Serialize(Ar);

		OnSaveDataLoaded();
		return true;
	}

	return false;
}

void UFlowComponent::OnSaveDataLoaded_Implementation()
{
}

void UFlowComponent::PrepareSaveData_Implementation()
{
}

UFlowSubsystem* UFlowComponent::GetFlowSubsystem() const
{
	if (GetWorld() && GetWorld()->GetGameInstance())
	{
		return GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>();
	}

	return nullptr;
}

bool UFlowComponent::IsFlowNetMode(const EFlowNetMode NetMode) const
{
	switch (NetMode)
	{
		case EFlowNetMode::Any:
			return true;
		case EFlowNetMode::Authority:
			return GetOwner()->HasAuthority();
		case EFlowNetMode::ClientOnly:
			return IsNetMode(NM_Client) && UFlowSettings::Get()->bCreateFlowSubsystemOnClients;
		case EFlowNetMode::ServerOnly:
			return IsNetMode(NM_DedicatedServer) || IsNetMode(NM_ListenServer);
		case EFlowNetMode::SinglePlayerOnly:
			return IsNetMode(NM_Standalone);
		default:
			return false;
	}
}
