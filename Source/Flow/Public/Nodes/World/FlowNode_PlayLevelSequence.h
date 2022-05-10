#pragma once

#include "EngineDefines.h"
#include "LevelSequencePlayer.h"
#include "MovieSceneSequencePlayer.h"

#include "Nodes/FlowNode.h"
#include "FlowNode_PlayLevelSequence.generated.h"

class UFlowLevelSequencePlayer;

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

	UPROPERTY(EditAnywhere, Category = "Sequence")
	TSoftObjectPtr<ULevelSequence> Sequence;

	UPROPERTY(EditAnywhere, Category = "Sequence")
	FMovieSceneSequencePlaybackSettings PlaybackSettings;

	UPROPERTY(EditAnywhere, Category = "Sequence")
	bool bPlayReverse;
	
	// if True, Play Rate will by multiplied by Custom Time Dilation
	// set in the actor that owns Root Flow
	UPROPERTY(EditAnywhere, Category = "Sequence")
	bool bApplyOwnerTimeDilation;
	
	UPROPERTY(EditAnywhere, Category = "Sequence")
	FLevelSequenceCameraSettings CameraSettings;

	UPROPERTY(EditAnywhere, Category = "Sequence")
	bool bUseGraphOwnerAsOriginalPointActor;
	
protected:
	UPROPERTY()
	ULevelSequence* LoadedSequence;

	UPROPERTY()
	UFlowLevelSequencePlayer* SequencePlayer;

	UPROPERTY()
	AActor* GraphOwner;

	// Play Rate set by the user in PlaybackSettings
	float CachedPlayRate;

	UPROPERTY(SaveGame)
	float StartTime;

	UPROPERTY(SaveGame)
	float ElapsedTime;

	UPROPERTY(SaveGame)
	float TimeDilation;

public:
#if WITH_EDITOR
	virtual bool SupportsContextPins() const override { return true; }
	virtual TArray<FName> GetContextOutputs() override;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void PreloadContent() override;
	virtual void FlushContent() override;

	virtual void InitializeInstance() override;
	void CreatePlayer();

protected:
	virtual void ExecuteInput(const FName& PinName) override;

	virtual void OnSave_Implementation() override;
	virtual void OnLoad_Implementation() override;

private:
	void TriggerEvent(const FString& EventName);

public:
	void OnTimeDilationUpdate(const float NewTimeDilation);

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
