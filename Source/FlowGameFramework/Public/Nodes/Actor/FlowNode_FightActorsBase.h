// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "ActorSpawning/FlowFightActorsTypes.h"
#include "Interfaces/FlowFightActorsReinforcementHost.h"
#include "Math/RandomStream.h"
#include "Nodes/Actor/FlowNode_SpawnActorsBase.h"

#include "FlowNode_FightActorsBase.generated.h"

class UFlowNodeAddOn;
class UFlowNodeAddOn_ReinforcementBase;

/** Shared authored fight policy, cohort passes, reinforcement, and completion for Flow and project fights. */
UCLASS(Abstract, Blueprintable, DisplayName = "Fight Actors Base")
class FLOWGAMEFRAMEWORK_API UFlowNode_FightActorsBase
	: public UFlowNode_SpawnActorsBase
	, public IFlowFightActorsReinforcementHost
{
	GENERATED_BODY()

protected:
	/** Initial cohort spawning policy for this fight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Configuration,
		DisplayName = "Initial Spawn Method", meta = (DisplayPriority = 4))
	EFlowFightActorSpawnMethod InitialSpawnMethod = EFlowFightActorSpawnMethod::FullSpawnAllCohorts;

	/** Rule deciding when all cohorts and any scheduled reinforcements allow the fight to finish. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Configuration,
		DisplayName = "Completion Rule", meta = (DisplayPriority = 4))
	EFlowFightActorCompletionRule CompletionRule =
		EFlowFightActorCompletionRule::AllActorsDefeatedWithNoReinforcementsScheduled;

	/** Fight lifecycle state, separate from the Flow node's activation state. */
	UPROPERTY(Transient)
	EFlowFightActorState FightState = EFlowFightActorState::Invalid;

	/** Deterministic stream initialized once per fight and advanced across reinforcement passes. */
	FRandomStream CohortSpawnOrderRandomStream;

	/** Prevents completion while a multi-cohort pass is still registering records. Not a fight lifecycle state. */
	bool bPreparingFightPass = false;

	/** Suppresses defeat notifications while releasing records during Flow cleanup. */
	bool bCleaningUpFight = false;

public:
	UFlowNode_FightActorsBase();

	static const FName INPIN_DefeatAndDisableAutoReinforce;
	static const FName OUTPIN_EachActorDefeated;
	static const FName OUTPIN_AllActorsDefeated;

	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;
	virtual void DeinitializeInstance() override;

#if WITH_SERVER_CODE
	virtual int32 ExecuteReinforcementPass(EFlowFightActorSpawnMethod Method, bool bShuffleCohorts) override;
	virtual float GetOverallAliveUnitPercent() const override;
	virtual void ReevaluateFightCompletion() override;

	/** Called by the cohort's Flow record-owner callbacks, retaining the inherited spawn output order. */
	void NotifyCohortPreSpawn(UFlowActorSpawnRecord& Record);
	void NotifyCohortPreFinishSpawn(UFlowActorSpawnRecord& Record);
	void NotifyCohortPostSpawn(UFlowActorSpawnRecord& Record);
	void NotifyCohortSpawnAttemptFinished(UFlowActorSpawnRecord& Record);
	void NotifyCohortActorCleanup(UFlowActorSpawnRecord& Record);
	void NotifyActorDefeated(AActor& Actor);
#endif

protected:
#if WITH_SERVER_CODE
	virtual void StartSpawningPass() override;
	virtual void DefeatAllSpawnedActors() override;
	virtual void CleanupActorRecords() override;
	virtual bool HasAnyOutstandingSpawns() const override;

	int32 ExecuteFightPass(EFlowFightActorSpawnMethod Method, bool bShuffleCohorts, bool bIsInitialPass);
	void DefeatCohorts(bool bDisableAutoReinforcement);
	void SetFightState(EFlowFightActorState NextState);
	void UpdateFightState();
	void ReevaluateAfterDefeat();
	FFlowFightActorCounts GetFightActorCounts() const;
	UFlowNodeAddOn_ReinforcementBase* FindReinforcement() const;

	virtual bool ShouldCheckCompletionAfterPass(bool bQueuedAnyRecords) const;
	virtual void OnFightInitialized(bool bInitialized) {}
	virtual void OnCohortActorSpawned(UFlowActorSpawnRecord& Record);
	virtual void OnBeforeCohortActorCleanup(UFlowActorSpawnRecord& Record);
#endif

	virtual EFlowAddOnAcceptResult AcceptFlowNodeAddOnChild_Implementation(
		const UFlowNodeAddOn* AddOnTemplate,
		const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const override;
	virtual void UpdateNodeConfigText_Implementation() override;

#if WITH_EDITOR
	virtual bool SupportsContextPins() const override { return true; }
	virtual EDataValidationResult ValidateNode() override;
#endif
};
