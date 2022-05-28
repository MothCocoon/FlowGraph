// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "LevelSequence/FlowLevelSequencePlayer.h"
#include "LevelSequence/FlowLevelSequenceActor.h"
#include "DefaultLevelSequenceInstanceData.h"
#include "Nodes/FlowNode.h"

UFlowLevelSequencePlayer::UFlowLevelSequencePlayer(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , FlowEventReceiver(nullptr)
{
}

UFlowLevelSequencePlayer* UFlowLevelSequencePlayer::CreateFlowLevelSequencePlayer(UObject* WorldContextObject, ULevelSequence* LevelSequence, FMovieSceneSequencePlaybackSettings Settings, FLevelSequenceCameraSettings CameraSettings, AActor* OriginalPointActor, ALevelSequenceActor*& OutActor)
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
	Actor->bOverrideInstanceData = true;
	OutActor = Actor;

	UDefaultLevelSequenceInstanceData* LevelSequenceData = static_cast<UDefaultLevelSequenceInstanceData*>(Actor->DefaultInstanceData);
	if (IsValid(LevelSequenceData) && IsValid(OriginalPointActor))
	{
		LevelSequenceData->TransformOrigin = OriginalPointActor->GetTransform();
	}

    // It seems doesn't really matter where the level sequence actor is.
	const FTransform DefaultTransform = FTransform(OriginalPointActor->GetActorRotation(),
			OriginalPointActor->GetActorLocation(), FVector::OneVector);
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
