#include "Nodes/World/FlowNode_PlayLevelSequence.h"

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
	, LoadedSequence(nullptr)
	, SequencePlayer(nullptr)
	, StartTime(0)
{
#if WITH_EDITOR
	Category = TEXT("World");
	NodeStyle = EFlowNodeStyle::Latent;
#endif

	InputNames.Empty();
	InputNames.Add(TEXT("Start"));
	InputNames.Add(TEXT("Stop"));

	OutputNames.Add(TEXT("PreStart"));
	OutputNames.Add(TEXT("Started"));
	OutputNames.Add(TEXT("Completed"));
	OutputNames.Add(TEXT("Stopped"));
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

void UFlowNode_PlayLevelSequence::CreatePlayer(const FMovieSceneSequencePlaybackSettings& PlaybackSettings)
{
	LoadedSequence = LoadAsset<ULevelSequence>(Sequence);
	if (LoadedSequence)
	{
		ALevelSequenceActor* SequenceActor;
		SequencePlayer = UFlowLevelSequencePlayer::CreateFlowLevelSequencePlayer(this, LoadedSequence, PlaybackSettings, SequenceActor);
		SequencePlayer->SetFlowEventReceiver(this);

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
			CreatePlayer(FMovieSceneSequencePlaybackSettings());

			TriggerOutput(TEXT("PreStart"));

			SequencePlayer->OnFinished.AddDynamic(this, &UFlowNode_PlayLevelSequence::OnPlaybackFinished);
			SequencePlayer->Play();

			TriggerOutput(TEXT("Started"));
		}

		TriggerFirstOutput(false);
	}
	else if (PinName == TEXT("Stop"))
	{
		StopPlayback();
	}
}

void UFlowNode_PlayLevelSequence::TriggerEvent(const FString& EventName)
{
	TriggerOutput(*EventName, false);
}

void UFlowNode_PlayLevelSequence::OnTimeDilationUpdate(const float NewTimeDilation) const
{
	if (SequencePlayer)
	{
		SequencePlayer->SetPlayRate(NewTimeDilation);
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

#if ENABLE_VISUAL_LOG
	UE_VLOG(this, LogFlow, Log, TEXT("Finished playback: %s"), *Sequence.ToString());
#endif
}

FString UFlowNode_PlayLevelSequence::GetPlaybackProgress() const
{
	if (SequencePlayer && SequencePlayer->IsPlaying())
	{
		return GetProgressAsString(SequencePlayer->GetCurrentTime().AsSeconds() - StartTime) + TEXT(" / ") + GetProgressAsString(SequencePlayer->GetDuration().AsSeconds());
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
