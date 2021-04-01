#include "FlowComponent.h"
#include "FlowSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

UFlowComponent::UFlowComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UFlowComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
	{
		FlowSubsystem->RegisterComponent(this);
	}
}

void UFlowComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
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
			if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
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
		if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
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
			if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
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
		if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
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
	if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
	{
		for (TWeakObjectPtr<UFlowComponent>& Component : FlowSubsystem->GetComponents<UFlowComponent>(ActorTag))
		{
			Component->ReceiveNotify.Broadcast(this, NotifyTag);
		}
	}
}

UFlowSubsystem* UFlowComponent::GetFlowSubsystem() const
{
	if (GetOwner()->GetGameInstance())
	{
		return GetOwner()->GetGameInstance()->GetSubsystem<UFlowSubsystem>();
	}

	return nullptr;
}
