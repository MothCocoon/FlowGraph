// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "LevelSequence/FlowLevelSequencePlayer.h"
#include "LevelSequence/FlowLevelSequenceActor.h"
#include "Nodes/FlowNode.h"

#include "DefaultLevelSequenceInstanceData.h"
#include "Runtime/Launch/Resources/Version.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowLevelSequencePlayer)

UFlowLevelSequencePlayer::UFlowLevelSequencePlayer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, FlowEventReceiver(nullptr)
{
}

UFlowLevelSequencePlayer* UFlowLevelSequencePlayer::CreateFlowLevelSequencePlayer(
	const UObject* WorldContextObject,
	ULevelSequence* LevelSequence,
	FMovieSceneSequencePlaybackSettings Settings,
	FLevelSequenceCameraSettings CameraSettings,
	AActor* TransformOriginActor,
	const bool bReplicates,
	const bool bAlwaysRelevant,
	ALevelSequenceActor*& OutActor
)
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

	// Sequence Actor might be spawned exactly where playback happens
	FTransform SpawnTransform = FTransform::Identity;
	{
		// apply Transform Origin
		// https://dev.epicgames.com/documentation/en-us/unreal-engine/creating-level-sequences-with-dynamic-transforms-in-unreal-engine
		if (TransformOriginActor)
		{
			// moving Level Sequence Actor might allow proper distance-based actor replication in networked games
			SpawnTransform = TransformOriginActor->GetTransform();
			SpawnTransform = FTransform(SpawnTransform.GetRotation(), SpawnTransform.GetLocation(), FVector::OneVector);
		}
	}

	// Create Sequence Actor
	// We use deferred spawn, so we can set all actor properties prior to its initialization.
	// This also helpful in case of multiplayer, since all actor settings are replicated with the spawned actor. No need to call replication just after spawn.
	AFlowLevelSequenceActor* Actor = World->SpawnActorDeferred<AFlowLevelSequenceActor>(AFlowLevelSequenceActor::StaticClass(), SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	Actor->SetPlaybackSettings(Settings);
	Actor->CameraSettings = CameraSettings;

	// InstanceData is not set to replicate to the clients, 
	// so the clients will still play the level sequence using the world origin as the sequence origin.
	// Instead of replicating the data, just assume the spawn transform of the level sequence actor is the sequence origin.
	// The level sequence actor was either spawned at the world origin or at the transform of TransformOriginActor.
	if (UDefaultLevelSequenceInstanceData* InstanceData = Cast<UDefaultLevelSequenceInstanceData>(Actor->DefaultInstanceData))
	{
		Actor->bOverrideInstanceData = true;
		InstanceData->TransformOriginActor = Actor;
	}

	// support networking
	if (bReplicates)
	{
		Actor->bReplicatePlayback = true;
		Actor->bAlwaysRelevant = bAlwaysRelevant;
		Actor->SetReplicatedLevelSequenceAsset(LevelSequence);
	}
	else
	{
		Actor->LevelSequenceAsset = LevelSequence;
	}

	// finish deferred spawn
	Actor->FinishSpawning(SpawnTransform);
	OutActor = Actor;

	// Sequence Player is created by Level Sequence Actor
	return Cast<UFlowLevelSequencePlayer>(Actor->GetSequencePlayer());
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
