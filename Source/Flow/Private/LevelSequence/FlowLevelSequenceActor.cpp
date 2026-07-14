// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "LevelSequence/FlowLevelSequenceActor.h"
#include "LevelSequence/FlowLevelSequencePlayer.h"
#include "Net/UnrealNetwork.h"
#include "Runtime/Launch/Resources/Version.h"

#include "DefaultLevelSequenceInstanceData.h"

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
}
