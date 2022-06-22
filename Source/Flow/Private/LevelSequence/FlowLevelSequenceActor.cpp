// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "LevelSequence/FlowLevelSequenceActor.h"
#include "LevelSequence/FlowLevelSequencePlayer.h"

AFlowLevelSequenceActor::AFlowLevelSequenceActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer
	  .SetDefaultSubobjectClass<UFlowLevelSequencePlayer>("AnimationPlayer"))
{
}
