#include "FlowComponent.h"
#include "FlowSettings.h"
#include "FlowSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

UFlowComponent::UFlowComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, RootFlow(nullptr)
	, bAutoStartRootFlow(true)
	, RootFlowMode(EFlowNetMode::Authority)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UFlowComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
	{
		FlowSubsystem->RegisterComponent(this);
	}

	if (RootFlow && bAutoStartRootFlow)
	{
		StartRootFlow();
	}
}

void UFlowComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
	{
		FlowSubsystem->UnregisterComponent(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UFlowComponent::AddIdentityTag(const FGameplayTag Tag)
{
	ensure(GetOwner()->HasAuthority());

	if (Tag.IsValid() && !IdentityTags.HasTagExact(Tag))
	{
		IdentityTags.AddTag(Tag);

		if (HasBegunPlay())
		{
			if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
			{
				FlowSubsystem->OnIdentityTagAdded(this, Tag);
			}
		}
	}
}

void UFlowComponent::AddIdentityTags(FGameplayTagContainer Tags)
{
	ensure(GetOwner()->HasAuthority());

	// todo: iterator and remove invalid tags
	for (const FGameplayTag& Tag : Tags)
	{
		if (Tag.IsValid() && !IdentityTags.HasTagExact(Tag))
		{
			IdentityTags.AddTag(Tag);
		}
	}

	if (Tags.Num() > 0 && HasBegunPlay())
	{
		if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
		{
			FlowSubsystem->OnIdentityTagsAdded(this, Tags);
		}
	}
}

void UFlowComponent::RemoveIdentityTag(const FGameplayTag Tag)
{
	ensure(GetOwner()->HasAuthority());

	if (Tag.IsValid() && IdentityTags.HasTagExact(Tag))
	{
		IdentityTags.RemoveTag(Tag);

		if (HasBegunPlay())
		{
			if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
			{
				FlowSubsystem->OnIdentityTagRemoved(this, Tag);
			}
		}
	}
}

void UFlowComponent::RemoveIdentityTags(FGameplayTagContainer Tags)
{
	ensure(GetOwner()->HasAuthority());

	// todo: iterator and remove invalid tags
	for (const FGameplayTag& Tag : Tags)
	{
		if (Tag.IsValid() && IdentityTags.HasTagExact(Tag))
		{
			IdentityTags.RemoveTag(Tag);
		}
	}

	if (Tags.Num() > 0 && HasBegunPlay())
	{
		if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
		{
			FlowSubsystem->OnIdentityTagsRemoved(this, Tags);
		}
	}
}

void UFlowComponent::NotifyGraph(const FGameplayTag NotifyTag)
{
	OnNotifyFromComponent.Broadcast(this, NotifyTag);
}

void UFlowComponent::NotifyFromGraph(const FGameplayTagContainer& NotifyTags)
{
	for (const FGameplayTag& NotifyTag : NotifyTags)
	{
		ReceiveNotify.Broadcast(nullptr, NotifyTag);
	}
}

void UFlowComponent::NotifyActor(const FGameplayTag ActorTag, const FGameplayTag NotifyTag)
{
	if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
	{
		for (TWeakObjectPtr<UFlowComponent>& Component : FlowSubsystem->GetComponents<UFlowComponent>(ActorTag))
		{
			Component->ReceiveNotify.Broadcast(this, NotifyTag);
		}
	}
}

void UFlowComponent::StartRootFlow() const
{
	if (RootFlow && IsFlowNetMode(RootFlowMode))
	{
		if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
		{
			FlowSubsystem->StartRootFlow(GetOwner(), RootFlow);
		}
	}
}

void UFlowComponent::FinishRootFlow() const
{
	if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
	{
		FlowSubsystem->FinishRootFlow(GetOwner());
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
