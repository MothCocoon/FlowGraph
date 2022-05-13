// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "LevelSequence/FlowLevelSequencePlayer.h"
#include "LevelSequence/FlowLevelSequenceActor.h"
#include "Nodes/FlowNode.h"

UFlowLevelSequencePlayer::UFlowLevelSequencePlayer(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , FlowEventReceiver(nullptr)
{
}

UFlowLevelSequencePlayer* UFlowLevelSequencePlayer::CreateFlowLevelSequencePlayer(UObject* WorldContextObject, ULevelSequence* LevelSequence, FMovieSceneSequencePlaybackSettings Settings, FLevelSequenceCameraSettings CameraSettings, ALevelSequenceActor*& OutActor)
{
	if (LevelSequence == nullptr)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr || World->bIsTearingDown)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.ObjectFlags |= RF_Transient;
	SpawnParams.bAllowDuringConstructionScript = true;

	// Defer construction for autoplay so that BeginPlay() is called
	SpawnParams.bDeferConstruction = true;

	ALevelSequenceActor* Actor = World->SpawnActor<AFlowLevelSequenceActor>(SpawnParams);

	Actor->PlaybackSettings = Settings;
	Actor->LevelSequenceAsset = LevelSequence;
	Actor->CameraSettings = CameraSettings;

	Actor->InitializePlayer();
	OutActor = Actor;

    const FTransform DefaultTransform;
	Actor->FinishSpawning(DefaultTransform);

	return Cast<UFlowLevelSequencePlayer>(Actor->SequencePlayer);
}

TArray<UObject*> UFlowLevelSequencePlayer::GetEventContexts() const
{
	TArray<UObject*> EventContexts;
	
    if (FlowEventReceiver)
    {
	    EventContexts.Add(FlowEventReceiver);
    }

	return EventContexts;
}
