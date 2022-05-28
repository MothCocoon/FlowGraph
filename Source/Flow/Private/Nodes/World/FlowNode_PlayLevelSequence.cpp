// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/World/FlowNode_PlayLevelSequence.h"

#include "FlowAsset.h"
#include "FlowModule.h"
#include "FlowSubsystem.h"
#include "LevelSequence/FlowLevelSequencePlayer.h"
#include "MovieScene/MovieSceneFlowTrack.h"
#include "MovieScene/MovieSceneFlowTriggerSection.h"

#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "VisualLogger/VisualLogger.h"

FFlowNodeLevelSequenceEvent UFlowNode_PlayLevelSequence::OnPlaybackStarted;
FFlowNodeLevelSequenceEvent UFlowNode_PlayLevelSequence::OnPlaybackCompleted;

UFlowNode_PlayLevelSequence::UFlowNode_PlayLevelSequence(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bPlayReverse(false)
	, bUseGraphOwnerAsTransformOrigin(false)
	, bApplyOwnerTimeDilation(true)
	, LoadedSequence(nullptr)
	, SequencePlayer(nullptr)
	, CachedPlayRate(0)
	, StartTime(0.0f)
	, ElapsedTime(0.0f)
	, TimeDilation(1.0f)
{
#if WITH_EDITOR
	Category = TEXT("World");
	NodeStyle = EFlowNodeStyle::Latent;
#endif

	InputPins.Empty();
	InputPins.Add(FFlowPin(TEXT("Start")));
	InputPins.Add(FFlowPin(TEXT("Stop")));

	OutputPins.Add(FFlowPin(TEXT("PreStart")));
	OutputPins.Add(FFlowPin(TEXT("Started")));
	OutputPins.Add(FFlowPin(TEXT("Completed")));
	OutputPins.Add(FFlowPin(TEXT("Stopped")));
}

#if WITH_EDITOR
TArray<FName> UFlowNode_PlayLevelSequence::GetContextOutputs()
{
	if (Sequence.IsNull())
	{
		return TArray<FName>();
	}

	TArray<FName> PinNames = {};

	Sequence = Sequence.LoadSynchronous();
	if (Sequence && Sequence->GetMovieScene())
	{
		for (const UMovieSceneTrack* Track : Sequence->GetMovieScene()->GetMasterTracks())
		{
			if (Track->GetClass() == UMovieSceneFlowTrack::StaticClass())
			{
				for (UMovieSceneSection* Section : Track->GetAllSections())
				{
					if (UMovieSceneFlowSectionBase* FlowSection = Cast<UMovieSceneFlowSectionBase>(Section))
					{
						for (const FString& EventName : FlowSection->GetAllEntryPoints())
						{
							if (!EventName.IsEmpty())
							{
								PinNames.Emplace(EventName);
							}
						}
					}
				}
			}
		}
	}

	return PinNames;
}

void UFlowNode_PlayLevelSequence::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property && PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UFlowNode_PlayLevelSequence, Sequence))
	{
		OnReconstructionRequested.ExecuteIfBound();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UFlowNode_PlayLevelSequence::PreloadContent()
{
#if ENABLE_VISUAL_LOG
	UE_VLOG(this, LogFlow, Log, TEXT("Preloading"));
#endif

	if (!Sequence.IsNull())
	{
		StreamableManager.RequestAsyncLoad({Sequence.ToSoftObjectPath()}, FStreamableDelegate());
	}
}

void UFlowNode_PlayLevelSequence::FlushContent()
{
#if ENABLE_VISUAL_LOG
	UE_VLOG(this, LogFlow, Log, TEXT("Flushing preload"));
#endif

	if (!Sequence.IsNull())
	{
		StreamableManager.Unload(Sequence.ToSoftObjectPath());
	}
}

void UFlowNode_PlayLevelSequence::InitializeInstance()
{
	Super::InitializeInstance();

	// Cache Play Rate set by user
	CachedPlayRate = PlaybackSettings.PlayRate;
}

void UFlowNode_PlayLevelSequence::CreatePlayer()
{
	LoadedSequence = LoadAsset<ULevelSequence>(Sequence);
	if (LoadedSequence)
	{
		ALevelSequenceActor* SequenceActor;

		AActor* OwningActor = nullptr;
		if (GetFlowAsset())
		{
			if (UObject* RootFlowOwner = GetFlowAsset()->GetOwner())
			{
				OwningActor = Cast<AActor>(RootFlowOwner); // in case Root Flow was created directly from some actor
				if (OwningActor == nullptr)
				{
					if (const UActorComponent* OwningComponent = Cast<UActorComponent>(RootFlowOwner))
					{
						OwningActor = OwningComponent->GetOwner();
					}
				}
			}
		}

		// Apply AActor::CustomTimeDilation from owner of the Root Flow
		if (IsValid(OwningActor))
		{
			PlaybackSettings.PlayRate = CachedPlayRate * OwningActor->CustomTimeDilation;
		}

		// Apply Transform Origin
		AActor* TransformOriginActor = bUseGraphOwnerAsTransformOrigin ? OwningActor : nullptr;

		// Finally create the player
		SequencePlayer = UFlowLevelSequencePlayer::CreateFlowLevelSequencePlayer(this, LoadedSequence, PlaybackSettings, CameraSettings, TransformOriginActor, SequenceActor);

		if (SequencePlayer)
		{
			SequencePlayer->SetFlowEventReceiver(this);
		}

		const FFrameRate FrameRate = LoadedSequence->GetMovieScene()->GetTickResolution();
		const FFrameNumber PlaybackStartFrame = LoadedSequence->GetMovieScene()->GetPlaybackRange().GetLowerBoundValue();
		StartTime = FQualifiedFrameTime(FFrameTime(PlaybackStartFrame, 0.0f), FrameRate).AsSeconds();
	}
}

void UFlowNode_PlayLevelSequence::ExecuteInput(const FName& PinName)
{
	if (PinName == TEXT("Start"))
	{
		LoadedSequence = LoadAsset<ULevelSequence>(Sequence);

		if (GetFlowSubsystem()->GetWorld() && LoadedSequence)
		{
			CreatePlayer();

			if (SequencePlayer)
			{
				TriggerOutput(TEXT("PreStart"));

				SequencePlayer->OnFinished.AddDynamic(this, &UFlowNode_PlayLevelSequence::OnPlaybackFinished);

				if (bPlayReverse)
				{
					SequencePlayer->PlayReverse();
				}
				else
				{
					SequencePlayer->Play();
				}

				TriggerOutput(TEXT("Started"));
			}
		}

		TriggerFirstOutput(false);
	}
	else if (PinName == TEXT("Stop"))
	{
		StopPlayback();
	}
}

void UFlowNode_PlayLevelSequence::OnSave_Implementation()
{
	if (SequencePlayer)
	{
		ElapsedTime = SequencePlayer->GetCurrentTime().AsSeconds();
	}
}

void UFlowNode_PlayLevelSequence::OnLoad_Implementation()
{
	if (ElapsedTime != 0.0f)
	{
		LoadedSequence = LoadAsset<ULevelSequence>(Sequence);

		if (GetFlowSubsystem()->GetWorld() && LoadedSequence)
		{
			CreatePlayer();

			if (SequencePlayer)
			{
				SequencePlayer->OnFinished.AddDynamic(this, &UFlowNode_PlayLevelSequence::OnPlaybackFinished);

				SequencePlayer->SetPlaybackPosition(FMovieSceneSequencePlaybackParams(ElapsedTime, EUpdatePositionMethod::Jump));

				// Take into account Play Rate set in the Playback Settings
				SequencePlayer->SetPlayRate(TimeDilation * CachedPlayRate);

				if (bPlayReverse)
				{
					SequencePlayer->PlayReverse();
				}
				else
				{
					SequencePlayer->Play();
				}
			}
		}
	}
}

void UFlowNode_PlayLevelSequence::TriggerEvent(const FString& EventName)
{
	TriggerOutput(*EventName, false);
}

void UFlowNode_PlayLevelSequence::OnTimeDilationUpdate(const float NewTimeDilation)
{
	if (SequencePlayer)
	{
		TimeDilation = NewTimeDilation;

		// Take into account Play Rate set in the Playback Settings
		SequencePlayer->SetPlayRate(NewTimeDilation * CachedPlayRate);
	}
}

void UFlowNode_PlayLevelSequence::OnPlaybackFinished()
{
	TriggerOutput(TEXT("Completed"), true);
}

void UFlowNode_PlayLevelSequence::StopPlayback()
{
	if (SequencePlayer)
	{
		SequencePlayer->Stop();
	}

	TriggerOutput(TEXT("Stopped"), true);
}

void UFlowNode_PlayLevelSequence::Cleanup()
{
	if (SequencePlayer)
	{
		SequencePlayer->SetFlowEventReceiver(nullptr);
		SequencePlayer->OnFinished.RemoveAll(this);
		SequencePlayer->Stop();
		SequencePlayer = nullptr;
	}

	LoadedSequence = nullptr;
	StartTime = 0.0f;
	ElapsedTime = 0.0f;
	TimeDilation = 1.0f;

#if ENABLE_VISUAL_LOG
	UE_VLOG(this, LogFlow, Log, TEXT("Finished playback: %s"), *Sequence.ToString());
#endif
}

FString UFlowNode_PlayLevelSequence::GetPlaybackProgress() const
{
	if (SequencePlayer && SequencePlayer->IsPlaying())
	{
		return GetProgressAsString(SequencePlayer->GetCurrentTime().AsSeconds() - StartTime).Append(TEXT(" / ")).Append(GetProgressAsString(SequencePlayer->GetDuration().AsSeconds()));
	}

	return FString();
}

#if WITH_EDITOR
FString UFlowNode_PlayLevelSequence::GetNodeDescription() const
{
	return Sequence.IsNull() ? TEXT("[No sequence]") : Sequence.GetAssetName();
}

FString UFlowNode_PlayLevelSequence::GetStatusString() const
{
	return GetPlaybackProgress();
}

UObject* UFlowNode_PlayLevelSequence::GetAssetToEdit()
{
	return Sequence.IsNull() ? nullptr : LoadAsset<UObject>(Sequence);
}
#endif

#if ENABLE_VISUAL_LOG
void UFlowNode_PlayLevelSequence::GrabDebugSnapshot(struct FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory NewCategory = FVisualLogStatusCategory(TEXT("Sequence"));
	NewCategory.Add(*Sequence.ToString(), FString());
	Snapshot->Status.Add(NewCategory);
}
#endif
