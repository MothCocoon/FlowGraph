// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/Actor/FlowNode_FightActorsBase.h"

#include "FlowNode_FightActors.generated.h"

// NOTE (gtaylor) The FightActors Node (FAN) implementation for FlowGraph plugin is not very
// usable on its own. To get the most out of it, you'd want to make your own version of FAN
// (inheriting from FANBase) which hooks into your own life/death systems for your game.
// But we felt having the base parts in FlowGraph (under FlowGameFramework) would be a good
// baseline for others to implement a similar sort of 'spawn and fight some NPCs' sort of 
// node in Flow -- and at least it's an example for how to use more complex Pin & AddOn setups.
//
// This node shares spawning technology with Spawn Actors Node (SAN).  But notably, this
// node configures the groupings of actors to spawn via 'cohort' addons, rather than via
// a single embedded definition of what to spawn.  This was done because in the fight case, we 
// found designers would often want to finely tune mixes of NPCs to spawn, distinctly
// configure them, but treat them as a single group for the purposes of tracking success 
// in the fight.

/** Standalone Flow fight using record-owned destruction as its defeat signal. */
UCLASS(Blueprintable, DisplayName = "Fight Actors")
class FLOWGAMEFRAMEWORK_API UFlowNode_FightActors : public UFlowNode_FightActorsBase
{
	GENERATED_BODY()

public:
	UFlowNode_FightActors();

protected:
#if WITH_SERVER_CODE
	virtual bool ShouldCheckCompletionAfterPass(bool bQueuedAnyRecords) const override;
	virtual void OnBeforeCohortActorCleanup(UFlowActorSpawnRecord& Record) override;
#endif
};
