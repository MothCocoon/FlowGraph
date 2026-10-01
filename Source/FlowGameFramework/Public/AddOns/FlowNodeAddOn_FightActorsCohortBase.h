// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "ActorSpawning/FlowActorSpawnRecordOwner.h"
#include "AddOns/FlowNodeAddOn.h"
#include "Interfaces/FlowFightActorsCohort.h"

#include "FlowNodeAddOn_FightActorsCohortBase.generated.h"

class UFlowActorSpawnRecord;

/** Shared record bookkeeping and Flow owner callbacks for vanilla and project cohorts. */
UCLASS(Abstract, EditInlineNew, Blueprintable, DisplayName = "Fight Actors Cohort Base")
class FLOWGAMEFRAMEWORK_API UFlowNodeAddOn_FightActorsCohortBase
	: public UFlowNodeAddOn
	, public IFlowFightActorsCohort
	, public IFlowActorSpawnRecordOwner
{
	GENERATED_BODY()

protected:
	/** A cohort retains its own records across refill passes. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UFlowActorSpawnRecord>> ActorRecords;

#if WITH_EDITORONLY_DATA
	/** Optional author-facing title for this cohort in the Flow graph. */
	UPROPERTY(EditAnywhere, Category = Configuration)
	FText CustomCohortTitle;
#endif

public:
	virtual void Cleanup() override;

#if WITH_SERVER_CODE
	virtual int32 ExecuteSpawningPass(EFlowFightActorSpawnMethod Method,
		FFlowActorSpawningAssistant& Assistant, bool bIsInitialPass) override;
	virtual void AppendActorCounts(FFlowFightActorCounts& InOutCounts) const override;
	virtual void DefeatSpawnedActors() override;
	virtual void CleanupActorRecords() override;

	virtual AActor* TryGetActorOwner() const override;
	virtual void ProcessPreSpawnConfiguration(UFlowActorSpawnRecord& Record) override;
	virtual void ProcessPreFinishSpawnActor(UFlowActorSpawnRecord& Record) override;
	virtual void ProcessPostSpawnConfiguration(UFlowActorSpawnRecord& Record) override;
	virtual void ProcessFinishedSpawnAttempt(UFlowActorSpawnRecord& Record) override;
	virtual void OnCleanupSpawnedInstance(UFlowActorSpawnRecord& Record) override;
#endif

protected:
#if WITH_SERVER_CODE
	virtual int32 GetInitialActorCount() const;
#endif

#if WITH_EDITOR
	virtual FText K2_GetNodeTitle_Implementation() const override;
#endif
};
