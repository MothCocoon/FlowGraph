// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "LevelSequence/FlowLevelSequencePlayer.h"
#include "LevelSequence/FlowLevelSequenceActor.h"
#include "Nodes/FlowNode.h"

#include "DefaultLevelSequenceInstanceData.h"

UFlowLevelSequencePlayer::UFlowLevelSequencePlayer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, FlowEventReceiver(nullptr)
{
}

UFlowLevelSequencePlayer* UFlowLevelSequencePlayer::CreateFlowLevelSequencePlayer(UObject* WorldContextObject, const ULevelSequence* LevelSequence, FMovieSceneSequencePlaybackSettings Settings, FLevelSequenceCameraSettings CameraSettings, AActor* TransformOriginActor, ALevelSequenceActor*& OutActor)
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
	Actor->LevelSequence = LevelSequence;
	Actor->CameraSettings = CameraSettings;

	Actor->InitializePlayer();
	OutActor = Actor;

	{
		FTransform DefaultTransform;
	
		// apply Transform Origin
		// https://docs.unrealengine.com/5.0/en-US/creating-level-sequences-with-dynamic-transforms-in-unreal-engine/
		if (IsValid(TransformOriginActor))
		{
			if (UDefaultLevelSequenceInstanceData* InstanceData = Cast<UDefaultLevelSequenceInstanceData>(Actor->DefaultInstanceData))
			{
				Actor->bOverrideInstanceData = true;
				InstanceData->TransformOriginActor = TransformOriginActor;

				// moving Level Sequence Actor might allow proper distance-based actor replication in networked games
				const FTransform OriginTransform = TransformOriginActor->GetTransform();
				DefaultTransform = FTransform(OriginTransform.GetRotation(), OriginTransform.GetLocation(), FVector::OneVector);
			}
		}

		Actor->FinishSpawning(DefaultTransform);
	}
	
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
