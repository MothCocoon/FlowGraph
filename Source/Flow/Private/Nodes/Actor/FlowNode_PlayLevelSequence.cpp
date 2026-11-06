// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Nodes/Actor/FlowNode_PlayLevelSequence.h"

#include "FlowAsset.h"
#include "FlowLogChannels.h"
#include "FlowSubsystem.h"
#include "AddOns/FlowNodeAddOn.h"
#include "Interfaces/FlowFastForwardGate.h"
#include "LevelSequence/FlowLevelSequenceActor.h"
#include "LevelSequence/FlowLevelSequencePlayer.h"
#include "LevelSequence/IFlowPlayLevelSequenceAddOnInterface.h"
#include "MovieScene/MovieSceneFlowRepeaterSection.h"
#include "MovieScene/MovieSceneFlowTrack.h"
#include "MovieScene/MovieSceneFlowTriggerSection.h"

#include "LevelSequence.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_PlayLevelSequence)

FFlowNodeLevelSequenceEvent UFlowNode_PlayLevelSequence::OnPlaybackStarted;
FFlowNodeLevelSequenceEvent UFlowNode_PlayLevelSequence::OnPlaybackCompleted;

UFlowNode_PlayLevelSequence::UFlowNode_PlayLevelSequence()
{
#if WITH_EDITOR
	Category = TEXT("Actor");
	NodeDisplayStyle = FlowNodeStyle::Latent;
#endif

	InputPins.Empty();
	InputPins.Add(FFlowPin(TEXT("Start")));
	InputPins.Add(FFlowPin(TEXT("Pause")));
	InputPins.Add(FFlowPin(TEXT("Resume")));
	InputPins.Add(FFlowPin(TEXT("Stop")));

	OutputPins.Add(FFlowPin(TEXT("PreStart")));
	OutputPins.Add(FFlowPin(TEXT("Started")));
	OutputPins.Add(FFlowPin(TEXT("Completed")));
	OutputPins.Add(FFlowPin(TEXT("Stopped")));
}

#if WITH_EDITOR
TArray<FFlowPin> UFlowNode_PlayLevelSequence::GetContextOutputs() const
{
	TArray<FFlowPin> Pins = Super::GetContextOutputs();

	if (Sequence.IsNull())
	{
		return Pins;
	}

	(void)Sequence.LoadSynchronous();
	if (Sequence && Sequence->GetMovieScene())
	{
		for (const UMovieSceneTrack* Track : Sequence->GetMovieScene()->GetTracks())
		{
			if (Track->GetClass() == UMovieSceneFlowTrack::StaticClass())
			{
				for (UMovieSceneSection* Section : Track->GetAllSections())
				{
					if (UMovieSceneFlowSectionBase* FlowSection = Cast<UMovieSceneFlowSectionBase>(Section))
					{
						for (const FString& EventName : FlowSection->GetAllEntryPoints())
						{
							FFlowPin NewEventPin(EventName);
							if (!EventName.IsEmpty() && !Pins.Contains(NewEventPin))
							{
								Pins.Emplace(NewEventPin);
							}
						}
					}
				}
			}
		}
	}

	return Pins;
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

EFlowPreloadResult UFlowNode_PlayLevelSequence::PreloadContent()
{
#if ENABLE_VISUAL_LOG
	UE_VLOG(this, LogFlow, Log, TEXT("Preloading Content"));
#endif

	FLOW_ASSERT_ENUM_MAX(EFlowPreloadResult, 2);

	if (Sequence.IsNull())
	{
		return EFlowPreloadResult::Completed;
	}

	// Bind a weak delegate so NotifyPreloadComplete() is called when streaming finishes.
	// If the asset is already cached, RequestAsyncLoad fires the delegate synchronously
	// (safe — PendingPreloadCount is already set by TriggerPreload before this call).
	PreloadHandle = StreamableManager.RequestAsyncLoad(
		Sequence.ToSoftObjectPath(),
		FStreamableDelegate::CreateWeakLambda(this, [this]()
		{
			NotifyPreloadComplete();
		}));

	return EFlowPreloadResult::PreloadInProgress;
}

void UFlowNode_PlayLevelSequence::FlushContent()
{
#if ENABLE_VISUAL_LOG
	UE_VLOG(this, LogFlow, Log, TEXT("Flushing Preloaded Content"));
#endif

	if (PreloadHandle.IsValid())
	{
		PreloadHandle->CancelHandle();
		PreloadHandle.Reset();
	}

	if (!Sequence.IsNull())
	{
		StreamableManager.Unload(Sequence.ToSoftObjectPath());
	}
}

EFlowAddOnAcceptResult UFlowNode_PlayLevelSequence::AcceptFlowNodeAddOnChild_Implementation(
	const UFlowNodeAddOn* AddOnTemplate,
	const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const
{
	if (IFlowPlayLevelSequenceAddOnInterface::ImplementsInterfaceSafe(AddOnTemplate))
	{
		return EFlowAddOnAcceptResult::TentativeAccept;
	}

	return Super::AcceptFlowNodeAddOnChild_Implementation(AddOnTemplate, AdditionalAddOnsToAssumeAreChildren);
}

void UFlowNode_PlayLevelSequence::InitializeInstance()
{
	Super::InitializeInstance();

	SequenceActor = nullptr;

	// Cache Play Rate set by user
	CachedPlayRate = PlaybackSettings.PlayRate;
}

void UFlowNode_PlayLevelSequence::CreatePlayer()
{
	LoadedSequence = Sequence.LoadSynchronous();
	if (LoadedSequence)
	{
		AActor* OwningActor = TryGetRootFlowActorOwner();

		// Apply AActor::CustomTimeDilation from owner of the Root Flow
		if (IsValid(OwningActor))
		{
			PlaybackSettings.PlayRate = CachedPlayRate * OwningActor->CustomTimeDilation;
		}

		// Apply Transform Origin
		const AActor* TransformOriginActor = bUseGraphOwnerAsTransformOrigin ? OwningActor : nullptr;

		// Finally create the player
		SequencePlayer = UFlowLevelSequencePlayer::CreateFlowLevelSequencePlayer(this, LoadedSequence, PlaybackSettings, CameraSettings, TransformOriginActor, bReplicates, bAlwaysRelevant, SequenceActor);

		if (SequencePlayer)
		{
			SequencePlayer->SetFlowEventReceiver(this);
		}

		// Notify add-ons so they can apply binding overrides before Play() is called
		if (AFlowLevelSequenceActor* FlowSequenceActor = SequenceActor.Get())
		{
			ForEachAddOnForClass<UFlowPlayLevelSequenceAddOnInterface>([this, FlowSequenceActor, OwningActor](UFlowNodeAddOn& AddOn)
			{
				IFlowPlayLevelSequenceAddOnInterface* Interface = CastChecked<IFlowPlayLevelSequenceAddOnInterface>(&AddOn);
				Interface->OnSequencePlayerCreated(*FlowSequenceActor, OwningActor);
				return EFlowForEachAddOnFunctionReturnValue::Continue;
			});
		}

		const FFrameRate FrameRate = LoadedSequence->GetMovieScene()->GetTickResolution();
		const FFrameNumber PlaybackStartFrame = LoadedSequence->GetMovieScene()->GetPlaybackRange().GetLowerBoundValue();
		StartTime = FQualifiedFrameTime(FFrameTime(PlaybackStartFrame, 0.0f), FrameRate).AsSeconds();
	}
}

void UFlowNode_PlayLevelSequence::ExecuteInput(const FName& PinName)
{
	// Since this node implements IFlowPreloadableInterface,
	// we need to call this to allow the PreloadHelper to intercept preload-specific PinNames
	if (DispatchExecuteInputToPreloadHelper(PinName))
	{
		return;
	}

	if (PinName == TEXT("Start"))
	{
		LoadedSequence = Sequence.LoadSynchronous();

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
	else if (PinName == TEXT("Pause") && SequencePlayer)
	{
		SequencePlayer->Pause();
	}
	else if (PinName == TEXT("Resume") && SequencePlayer && SequencePlayer->IsPaused())
	{
		SequencePlayer->Play();
	}
}

void UFlowNode_PlayLevelSequence::FastForwardInput(const FName& PinName, const FName& OutputPinName)
{
	// Playback starts, pauses, resumes or stops like after a real input
	if (!PinName.IsNone())
	{
		ExecuteInputForSelfAndAddOns(PinName);
	}

	if (GetActivationState() == EFlowNodeState::Active && SequencePlayer)
	{
		if (OutputPinName == TEXT("Completed"))
		{
			// Duration includes subframes, unlike the end time
			FastForwardPlaybackTo(bPlayReverse
				? SequencePlayer->GetStartTime().Time - FFrameTime(1)
				: SequencePlayer->GetStartTime().Time + SequencePlayer->GetDuration().Time);

			// Looping, paused or not yet playing sequence doesn't finish by evaluating its remaining frames
			if (GetActivationState() == EFlowNodeState::Active)
			{
				FFlowFastForwardGate::ReportForcedOutput(*this, OutputPinName);

				// Reverse playback finishes at the start
				if (bPlayReverse)
				{
					SequencePlayer->SetPlaybackPosition(FMovieSceneSequencePlaybackParams(SequencePlayer->GetStartTime().Time, EUpdatePositionMethod::Jump));
					SequencePlayer->StopAtCurrentTime();
				}
				else
				{
					SequencePlayer->GoToEndAndStop();
				}

				OnPlaybackFinished();
			}
		}
		else if (OutputPinName == TEXT("Stopped"))
		{
			StopPlayback();
		}
		else if (const TOptional<FFrameTime> EventTime = FindEventTime(OutputPinName.ToString()))
		{
			FastForwardPlaybackTo(EventTime.GetValue());
		}
	}

	// Event that didn't fire, output triggered by the active node before or playback waiting for preloaded content
	if (IsFastForwardOutputPending(OutputPinName) && GetActivationState() == EFlowNodeState::Active)
	{
		FFlowFastForwardGate::ReportForcedOutput(*this, OutputPinName);
		TriggerOutput(OutputPinName, OutputPinName == TEXT("Completed"));
	}
}

bool UFlowNode_PlayLevelSequence::CanFastForward(const FName& InputPinName, const FName& OutputPinName) const
{
	bool bInputLeadsToOutput = OutputPinName != TEXT("Stopped");

	if (InputPinName == TEXT("Stop"))
	{
		bInputLeadsToOutput = OutputPinName == TEXT("Stopped");
	}
	else if (InputPinName == TEXT("Pause"))
	{
		bInputLeadsToOutput = false;
	}
	else if (InputPinName == TEXT("Resume"))
	{
		// Only started sequence resumes, outputs triggered when playback starts are behind it already
		bInputLeadsToOutput &= SequencePlayer != nullptr
			&& OutputPinName != TEXT("PreStart") && OutputPinName != TEXT("Started") && OutputPinName != UFlowNode::DefaultOutputPin.PinName;
	}

	return bInputLeadsToOutput;
}

void UFlowNode_PlayLevelSequence::FastForwardPlaybackTo(const FFrameTime& Frame) const
{
	// Play method sweeps the whole range, unlike Jump, so actors end up in the state set by the sequence
	SequencePlayer->SetPlaybackPosition(FMovieSceneSequencePlaybackParams(Frame, EUpdatePositionMethod::Play));
}

TOptional<FFrameTime> UFlowNode_PlayLevelSequence::FindEventTime(const FString& EventName) const
{
	TOptional<FFrameTime> EventTime;

	const UMovieScene* MovieScene = LoadedSequence ? LoadedSequence->GetMovieScene() : nullptr;
	if (MovieScene && SequencePlayer)
	{
		// Section keys use the tick resolution
		TArray<FFrameNumber> KeyTimes;

		for (const UMovieSceneTrack* Track : MovieScene->GetTracks())
		{
			if (Track->IsA<UMovieSceneFlowTrack>())
			{
				for (const UMovieSceneSection* Section : Track->GetAllSections())
				{
					if (const UMovieSceneFlowTriggerSection* TriggerSection = Cast<UMovieSceneFlowTriggerSection>(Section))
					{
						const TMovieSceneChannelData<const FString> ChannelData = TriggerSection->StringChannel.GetData();
						for (int32 KeyIndex = 0; KeyIndex < ChannelData.GetValues().Num(); ++KeyIndex)
						{
							if (ChannelData.GetValues()[KeyIndex] == EventName)
							{
								KeyTimes.Add(ChannelData.GetTimes()[KeyIndex]);
							}
						}
					}
					else if (const UMovieSceneFlowRepeaterSection* RepeaterSection = Cast<UMovieSceneFlowRepeaterSection>(Section))
					{
						if (RepeaterSection->EventName == EventName && RepeaterSection->HasStartFrame())
						{
							KeyTimes.Add(RepeaterSection->GetInclusiveStartFrame());
						}
					}
				}
			}
		}

		// Nearest event ahead of the playback, so the sequence never moves backward
		const FFrameTime CurrentTime = SequencePlayer->GetCurrentTime().Time;
		for (const FFrameNumber& KeyTime : KeyTimes)
		{
			const FFrameTime KeyPlayerTime = ConvertFrameTime(FFrameTime(KeyTime), MovieScene->GetTickResolution(), SequencePlayer->GetFrameRate());
			const bool bAhead = bPlayReverse ? KeyPlayerTime <= CurrentTime : KeyPlayerTime >= CurrentTime;
			const bool bNearer = !EventTime.IsSet() || (bPlayReverse ? KeyPlayerTime > EventTime.GetValue() : KeyPlayerTime < EventTime.GetValue());

			if (bAhead && bNearer)
			{
				EventTime = KeyPlayerTime;
			}
		}
	}

	return EventTime;
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
		LoadedSequence = Sequence.LoadSynchronous();
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
		if (!PlaybackSettings.bPauseAtEnd)
		{
			SequencePlayer->Stop();
		}
		SequencePlayer = nullptr;
	}

	if (IsValid(SequenceActor) && SequenceActor->HasAuthority())
	{
		SequenceActor->Destroy();
	}
	SequenceActor = nullptr;

	LoadedSequence = nullptr;
	StartTime = 0.0f;
	ElapsedTime = 0.0f;
	TimeDilation = 1.0f;

#if ENABLE_VISUAL_LOG
	UE_VLOG(this, LogFlow, Log, TEXT("Finished playback: %s"), *Sequence.ToString());
#endif

	Super::Cleanup();
}

FString UFlowNode_PlayLevelSequence::GetPlaybackProgress() const
{
	if (SequencePlayer && SequencePlayer->IsPlaying())
	{
		return FString::Printf(TEXT("%.*f / %.*f"), 2, SequencePlayer->GetCurrentTime().AsSeconds() - StartTime, 2, SequencePlayer->GetDuration().AsSeconds());
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
	return Sequence.IsNull() ? nullptr : Sequence.LoadSynchronous();
}

EDataValidationResult UFlowNode_PlayLevelSequence::ValidateNode()
{
	if (Sequence.IsNull())
	{
		ValidationLog.Error<UFlowNode>(TEXT("Level Sequence asset not assigned or invalid!"), this);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
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

#if WITH_EDITOR
const FFlowAgentDoc& UFlowNode_PlayLevelSequence::GetAgentDoc() const
{
	static const FFlowAgentDoc Doc = MakeAgentDoc(
		/*Guidance*/ TEXT("Attach AddOns implementing IFlowPlayLevelSequenceAddOnInterface to apply setup, such as actor binding overrides, after the sequence player is created and before playback begins. The plugin ships no such AddOn, projects provide their own. Enable bUseGraphOwnerAsTransformOrigin to play the sequence relative to the actor that created the root flow instance rather than world origin."),
		/*Tags*/     { TEXT("actor"), TEXT("sequence"), TEXT("cutscene") },
		/*Articles*/ {  });
	return Doc;
}
#endif
