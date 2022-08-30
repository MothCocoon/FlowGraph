// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "LevelSequenceActor.h"
#include "FlowLevelSequenceActor.generated.h"

/**
 * Custom ALevelSequenceActor is needed to override ULevelSequencePlayer class
 */
UCLASS(hideCategories=(Rendering, Physics, LOD, Activation, Input))
class FLOW_API AFlowLevelSequenceActor : public ALevelSequenceActor
{
	GENERATED_UCLASS_BODY()

protected:	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
public:
	void SetReplicatedLevelSequenceAsset(ULevelSequence* Asset);

	UFUNCTION(NetMulticast, Reliable)
	void RPC_InitializePlayer();
	
protected:
	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedLevelSequenceAsset)
	TObjectPtr<ULevelSequence> ReplicatedLevelSequenceAsset;

	UFUNCTION()
	void OnRep_ReplicatedLevelSequenceAsset();
};
