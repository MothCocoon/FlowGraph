// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "LevelSequence/FlowLevelSequenceActor.h"
#include "FlowLogChannels.h"
#include "LevelSequence/FlowLevelSequencePlayer.h"
#include "Net/UnrealNetwork.h"
#include "Runtime/Launch/Resources/Version.h"

// <MKT> #PlayLevelSequenceAtSpawnTransform
#include "DefaultLevelSequenceInstanceData.h"
// </MKT>

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowLevelSequenceActor)

AFlowLevelSequenceActor::AFlowLevelSequenceActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UFlowLevelSequencePlayer>("AnimationPlayer"))
	, ReplicatedLevelSequenceAsset(nullptr)
{
}

void AFlowLevelSequenceActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFlowLevelSequenceActor, ReplicatedLevelSequenceAsset);
	DOREPLIFETIME(AFlowLevelSequenceActor, BindingEntries);
}

void AFlowLevelSequenceActor::SetPlaybackSettings(FMovieSceneSequencePlaybackSettings NewPlaybackSettings)
{
	PlaybackSettings = NewPlaybackSettings;
	GetSequencePlayer()->SetPlaybackSettings(PlaybackSettings);
}

void AFlowLevelSequenceActor::SetReplicatedLevelSequenceAsset(ULevelSequence* Asset)
{
	if (HasAuthority())
	{
		LevelSequenceAsset = Asset;
		ReplicatedLevelSequenceAsset = LevelSequenceAsset;
	}
}

void AFlowLevelSequenceActor::OnRep_ReplicatedLevelSequenceAsset()
{
	LevelSequenceAsset = ReplicatedLevelSequenceAsset;
	ReplicatedLevelSequenceAsset = nullptr;
	
	// InstanceData is not replicated to the client.
	// However, it can be assumed that the spawn transform of the level sequence actor is the transform origin for the sequence.
	if (UDefaultLevelSequenceInstanceData* InstanceData = Cast<UDefaultLevelSequenceInstanceData>(DefaultInstanceData))
	{
		bOverrideInstanceData = true;
		InstanceData->TransformOriginActor = this;
	}

	InitializePlayer();

	// Re-apply bindings that replicated before the sequence player was created
	OnRep_BindingEntries();
}

void AFlowLevelSequenceActor::AddBinding(FName Tag, AActor* Actor)
{
	if (!HasAuthority() || !IsValid(Actor) || Tag.IsNone())
	{
		return;
	}

	FFlowSequenceBindingEntry& Entry = BindingEntries.AddDefaulted_GetRef();
	Entry.BindingTag = Tag;
	Entry.BoundActor = Actor;

	if (IsValid(GetSequencePlayer()))
	{
		SetBindingByTag(Tag, {Actor}, false);
	}
	else
	{
		UE_LOG(LogFlow, Warning, TEXT("AFlowLevelSequenceActor::AddBinding - sequence player not initialized for tag '%s'; binding queued for replication but not applied locally"), *Tag.ToString());
	}
}

void AFlowLevelSequenceActor::ClearAllBindings()
{
	if (!HasAuthority())
	{
		return;
	}

	BindingEntries.Empty();
	ResetBindings();
}

void AFlowLevelSequenceActor::OnRep_BindingEntries()
{
	ResetBindings();

	for (const FFlowSequenceBindingEntry& Entry : BindingEntries)
	{
		if (IsValid(Entry.BoundActor) && !Entry.BindingTag.IsNone())
		{
			SetBindingByTag(Entry.BindingTag, {Entry.BoundActor.Get()}, false);
		}
	}
}
