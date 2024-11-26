// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "EngineDefines.h"
#include "Engine/StreamableManager.h"
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

public:	
	static FFlowNodeLevelSequenceEvent OnPlaybackStarted;
	static FFlowNodeLevelSequenceEvent OnPlaybackCompleted;

	UPROPERTY(EditAnywhere, Category = "Sequence")
	TSoftObjectPtr<ULevelSequence> Sequence;

	UPROPERTY(EditAnywhere, Category = "Sequence")
	FMovieSceneSequencePlaybackSettings PlaybackSettings;

	UPROPERTY(EditAnywhere, Category = "Sequence")
	bool bPlayReverse;

	UPROPERTY(EditAnywhere, Category = "Sequence")
	FLevelSequenceCameraSettings CameraSettings;
	
	// Level Sequence playback can be moved to any place in the world by applying Transform Origin
	// Enabling this option will use actor that created Root Flow instance, i.e. World Settings or Player Controller
	// https://docs.unrealengine.com/5.0/en-US/creating-level-sequences-with-dynamic-transforms-in-unreal-engine/
	UPROPERTY(EditAnywhere, Category = "Sequence")
	bool bUseGraphOwnerAsTransformOrigin;

	// If true, playback of this level sequence on the server will be synchronized across other clients
	UPROPERTY(EditAnywhere, Category = "Sequence")
	bool bReplicates;

	// Always relevant for network (overrides bOnlyRelevantToOwner)
	UPROPERTY(EditAnywhere, Category = "Sequence")
	bool bAlwaysRelevant;

	// If True, Play Rate will by multiplied by Custom Time Dilation
	// Enabling this option will use Custom Time Dilation from actor that created Root Flow instance, i.e. World Settings or Player Controller
	UPROPERTY(EditAnywhere, Category = "Sequence")
	bool bApplyOwnerTimeDilation;
	
protected:
	UPROPERTY()
	TObjectPtr<ULevelSequence> LoadedSequence;

	UPROPERTY()
	TObjectPtr<UFlowLevelSequencePlayer> SequencePlayer;

	// Play Rate set by the user in PlaybackSettings
	float CachedPlayRate;

	UPROPERTY(SaveGame)
	float StartTime;

	UPROPERTY(SaveGame)
	float ElapsedTime;

	UPROPERTY(SaveGame)
	float TimeDilation;

	FStreamableManager StreamableManager;

public:
#if WITH_EDITOR
	// IFlowContextPinSupplierInterface
	virtual bool SupportsContextPins() const override { return true; }
	virtual TArray<FFlowPin> GetContextOutputs() const override;
	// --

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
	virtual EDataValidationResult ValidateNode() override;
	
	virtual FString GetStatusString() const override;
	virtual UObject* GetAssetToEdit() override;
#endif

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(struct FVisualLogEntry* Snapshot) const override;
#endif
};
