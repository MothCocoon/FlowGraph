#pragma once

#include "EngineDefines.h"
#include "MovieSceneSequencePlayer.h"
#include "Nodes/FlowNode.h"
#include "FlowNode_PlayLevelSequence.generated.h"

class UFlowLevelSequencePlayer;
class ULevelSequence;

DECLARE_MULTICAST_DELEGATE(FFlowNodeLevelSequenceEvent);

/**
 * Order of triggering outputs after calling Start
 * - PreStart, just before starting playback
 * - Started
 * - Out (always, even if Sequence is invalid)
 * - Completed
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Play Level Sequence"))
class FLOW_API UFlowNode_PlayLevelSequence : public UFlowNode
{
	GENERATED_UCLASS_BODY()

	friend struct FFlowTrackExecutionToken;

	static FFlowNodeLevelSequenceEvent OnPlaybackStarted;
	static FFlowNodeLevelSequenceEvent OnPlaybackCompleted;

	UPROPERTY(EditDefaultsOnly, Category = "Sequence")
	TSoftObjectPtr<ULevelSequence> Sequence;

protected:
	UPROPERTY()
	ULevelSequence* LoadedSequence;

	UPROPERTY()
	UFlowLevelSequencePlayer* SequencePlayer;

	float StartTime;

public:
#if WITH_EDITOR
	virtual bool SupportsContextPins() const override { return true; }
	virtual TArray<FName> GetContextOutputs() override;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void PreloadContent() override;
	virtual void FlushContent() override;

	void CreatePlayer(const FMovieSceneSequencePlaybackSettings& PlaybackSettings);

protected:
	virtual void ExecuteInput(const FName& PinName) override;

private:
	void TriggerEvent(const FString& EventName);

public:
	void OnTimeDilationUpdate(const float NewTimeDilation) const;

protected:
	UFUNCTION()
	virtual void OnPlaybackFinished();

public:
	virtual void StopPlayback();

protected:
	virtual void Cleanup() override;

public:
	FString GetPlaybackProgress() const;

#if WITH_EDITOR
	virtual FString GetNodeDescription() const override;
	virtual FString GetStatusString() const override;
	virtual UObject* GetAssetToEdit() override;
#endif

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(struct FVisualLogEntry* Snapshot) const override;
#endif
};
