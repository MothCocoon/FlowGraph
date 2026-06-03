// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "LevelSequenceActor.h"
#include "FlowLevelSequenceActor.generated.h"

class ULevelSequence;

/** Single actor binding override entry, replicated from server to clients. */
USTRUCT()
struct FFlowSequenceBindingEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FName BindingTag;

	UPROPERTY()
	TObjectPtr<AActor> BoundActor;
};

/**
 * Custom ALevelSequenceActor is needed to override ULevelSequencePlayer class.
 */
UCLASS(hideCategories=(Rendering, Physics, LOD, Activation, Input))
class FLOW_API AFlowLevelSequenceActor : public ALevelSequenceActor
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedLevelSequenceAsset)
	TObjectPtr<ULevelSequence> ReplicatedLevelSequenceAsset;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	void SetPlaybackSettings(FMovieSceneSequencePlaybackSettings NewPlaybackSettings);
	void SetReplicatedLevelSequenceAsset(ULevelSequence* Asset);

	/** Server only. Adds a binding override that replicates to clients via OnRep_BindingEntries. */
	void AddBinding(FName Tag, AActor* Actor);

	/** Server only. Clears all binding overrides and replicates the cleared state to clients. */
	void ClearAllBindings();

protected:
	UFUNCTION()
	void OnRep_ReplicatedLevelSequenceAsset();

	UPROPERTY(ReplicatedUsing = OnRep_BindingEntries)
	TArray<FFlowSequenceBindingEntry> BindingEntries;

	UFUNCTION()
	void OnRep_BindingEntries();
};
