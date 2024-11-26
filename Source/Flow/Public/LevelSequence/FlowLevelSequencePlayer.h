// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "LevelSequencePlayer.h"
#include "FlowLevelSequencePlayer.generated.h"

class UFlowNode;

/**
 * Custom ULevelSequencePlayer allows for binding Flow Nodes to Level Sequence events
 */
UCLASS()
class FLOW_API UFlowLevelSequencePlayer : public ULevelSequencePlayer
{
	GENERATED_UCLASS_BODY()

private:
	// most likely this is a UFlowNode_PlayLevelSequence or its child
	UPROPERTY()
	TObjectPtr<UFlowNode> FlowEventReceiver;

public:
	// variant of ULevelSequencePlayer::CreateLevelSequencePlayer
	static UFlowLevelSequencePlayer* CreateFlowLevelSequencePlayer(
		const UObject* WorldContextObject,
		ULevelSequence* LevelSequence,
		FMovieSceneSequencePlaybackSettings Settings,
		FLevelSequenceCameraSettings CameraSettings,
		AActor* TransformOriginActor,
		const bool bReplicates,
		const bool bAlwaysRelevant,
		ALevelSequenceActor*& OutActor);

	void SetFlowEventReceiver(UFlowNode* FlowNode) { FlowEventReceiver = FlowNode; }

	// IMovieScenePlayer
	virtual TArray<UObject*> GetEventContexts() const override;
	// --
};
