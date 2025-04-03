// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "LevelSequence/FlowLevelSequenceActor.h"
#include "LevelSequence/FlowLevelSequencePlayer.h"
#include "Net/UnrealNetwork.h"
#include "Runtime/Launch/Resources/Version.h"

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

	InitializePlayer();
}
