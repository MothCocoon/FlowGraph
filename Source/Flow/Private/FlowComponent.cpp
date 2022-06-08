// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowComponent.h"

#include "FlowAsset.h"
#include "FlowModule.h"
#include "FlowSettings.h"
#include "FlowSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/ViewportStatsSubsystem.h"
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
		if (GetFlowSubsystem()->GetLoadedSaveGame())
		{
			bComponentLoadedFromSaveGame = LoadInstance();
		}

		FlowSubsystem->RegisterComponent(this);

		if (RootFlow)
		{
			if (bComponentLoadedFromSaveGame)
			{
				LoadRootFlow();
			}
			else if (bAutoStartRootFlow)
			{
				StartRootFlow();
			}
		}
	}
}

void UFlowComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
	{
		FlowSubsystem->FinishRootFlow(this, EFlowFinishPolicy::Keep);
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

void UFlowComponent::VerifyIdentityTags() const
{
	if (IdentityTags.IsEmpty() && UFlowSettings::Get()->bWarnAboutMissingIdentityTags)
	{
		FString Message = TEXT("Missing Identity Tags on the Flow Component creating Flow Asset instance! This gonna break loading SaveGame for this component!");
		Message.Append(LINE_TERMINATOR).Append(TEXT("If you're not using SaveSystem, you can silence this warning by unchecking bWarnAboutMissingIdentityTags flag in Flow Settings."));
		LogError(Message);
	}
}

void UFlowComponent::LogError(FString Message, const EFlowOnScreenMessageType OnScreenMessageType) const
{
	Message += TEXT(" --- Flow Component in actor ") + GetOwner()->GetName();

	if (OnScreenMessageType == EFlowOnScreenMessageType::Permanent)
	{
		if (GetWorld())
		{
			if (UViewportStatsSubsystem* StatsSubsystem = GetWorld()->GetSubsystem<UViewportStatsSubsystem>())
			{
				StatsSubsystem->AddDisplayDelegate([this, Message](FText& OutText, FLinearColor& OutColor)
				{
					OutText = FText::FromString(Message);
					OutColor = FLinearColor::Red;
					return IsValid(this);
				});
			}
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, Message);
	}

	UE_LOG(LogFlow, Error, TEXT("%s"), *Message);
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
			RecentlySentNotifyTags = ValidatedTags;

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
			for (const FGameplayTag& ValidatedTag : ValidatedTags)
			{
				ReceiveNotify.Broadcast(nullptr, ValidatedTag);
			}

			if (IsNetMode(NM_DedicatedServer) || IsNetMode(NM_ListenServer))
			{
				NotifyTagsFromGraph = ValidatedTags;
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
		if (const UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
		{
			for (const TWeakObjectPtr<UFlowComponent>& Component : FlowSubsystem->GetComponents<UFlowComponent>(ActorTag))
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
	if (const UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
	{
		for (const FNotifyTagReplication& Notify : NotifyTagsFromAnotherComponent)
		{
			for (const TWeakObjectPtr<UFlowComponent>& Component : FlowSubsystem->GetComponents<UFlowComponent>(Notify.ActorTag))
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
			VerifyIdentityTags();

			FlowSubsystem->StartRootFlow(this, RootFlow, bAllowMultipleInstances);
		}
	}
}

void UFlowComponent::FinishRootFlow(const EFlowFinishPolicy FinishPolicy)
{
	if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
	{
		FlowSubsystem->FinishRootFlow(this, FinishPolicy);
	}
}

UFlowAsset* UFlowComponent::GetRootFlowInstance()
{
	if (const UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
	{
		return FlowSubsystem->GetRootFlow(this);
	}

	return nullptr;
}

void UFlowComponent::SaveRootFlow(TArray<FFlowAssetSaveData>& SavedFlowInstances)
{
	if (UFlowAsset* FlowAssetInstance = GetRootFlowInstance())
	{
		const FFlowAssetSaveData AssetRecord = FlowAssetInstance->SaveInstance(SavedFlowInstances);
		SavedAssetInstanceName = AssetRecord.InstanceName;
		return;
	}

	SavedAssetInstanceName = FString();
}

void UFlowComponent::LoadRootFlow()
{
	if (RootFlow && !SavedAssetInstanceName.IsEmpty() && GetFlowSubsystem())
	{
		VerifyIdentityTags();

		GetFlowSubsystem()->LoadRootFlow(this, RootFlow, SavedAssetInstanceName);
		SavedAssetInstanceName = FString();
	}
}

FFlowComponentSaveData UFlowComponent::SaveInstance()
{
	FFlowComponentSaveData ComponentRecord;
	ComponentRecord.WorldName = GetWorld()->GetName();
	ComponentRecord.ActorInstanceName = GetOwner()->GetName();

	// opportunity to collect data before serializing component
	OnSave();

	// serialize component
	FMemoryWriter MemoryWriter(ComponentRecord.ComponentData, true);
	FFlowArchive Ar(MemoryWriter);
	Serialize(Ar);

	return ComponentRecord;
}

bool UFlowComponent::LoadInstance()
{
	const UFlowSaveGame* SaveGame = GetFlowSubsystem()->GetLoadedSaveGame();
	if (SaveGame->FlowComponents.Num() > 0)
	{
		for (const FFlowComponentSaveData& ComponentRecord : SaveGame->FlowComponents)
		{
			if (ComponentRecord.WorldName == GetWorld()->GetName() && ComponentRecord.ActorInstanceName == GetOwner()->GetName())
			{
				FMemoryReader MemoryReader(ComponentRecord.ComponentData, true);
				FFlowArchive Ar(MemoryReader);
				Serialize(Ar);

				OnLoad();
				return true;
			}
		}
	}

	return false;
}

void UFlowComponent::OnSave_Implementation()
{
}

void UFlowComponent::OnLoad_Implementation()
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
