// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "LevelSequence/FlowLevelSequenceActor.h"
#include "LevelSequence/FlowLevelSequencePlayer.h"
#include "Net/UnrealNetwork.h"

AFlowLevelSequenceActor::AFlowLevelSequenceActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UFlowLevelSequencePlayer>("AnimationPlayer"))
	, ReplicatedLevelSequenceAsset(nullptr)
{
}

void AFlowLevelSequenceActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFlowLevelSequenceActor, ReplicatedLevelSequenceAsset);
}

void AFlowLevelSequenceActor::SetReplicatedLevelSequenceAsset(ULevelSequence* Asset)
{
	if (HasAuthority())
	{
		LevelSequenceAsset = Asset;
		ReplicatedLevelSequenceAsset = LevelSequenceAsset;
	}
}

void AFlowLevelSequenceActor::OnRep_ReplicatedLevelSequenceAsset()
{
	LevelSequenceAsset = ReplicatedLevelSequenceAsset;
	ReplicatedLevelSequenceAsset = nullptr;

	InitializePlayer();
}
