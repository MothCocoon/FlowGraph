// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "AddOns/FlowNodeAddOn_FightActorsCohortBase.h"
#include "StructUtils/InstancedStruct.h"

#include "FlowNodeAddOn_FightActorsCohortFixedCount.generated.h"

class UFlowActorSpawnRecord;

/** Fixed-count cohort with a Flow actor-class selector and an instanced location record. */
UCLASS(EditInlineNew, Blueprintable, DisplayName = "Cohort (Fixed Count)")
class FLOWGAMEFRAMEWORK_API UFlowNodeAddOn_FightActorsCohortFixedCount
	: public UFlowNodeAddOn_FightActorsCohortBase
{
	GENERATED_BODY()

protected:
	/** The configured class and count, selected using the same struct family as vanilla SAN. */
	UPROPERTY(EditAnywhere, Category = Configuration, DisplayName = "What to Spawn", NoClear,
		meta = (ExcludeBaseStruct, BaseStruct = "/Script/FlowGameFramework.FlowActorSpawnSelector", DisplayPriority = 2))
	FInstancedStruct ActorSpawnSelector;

	/** Authored location and acquisition template; defaults to a normal deferred Flow actor. */
	UPROPERTY(EditAnywhere, Instanced, Category = Configuration, DisplayName = "Where to Spawn", NoClear,
		meta = (DisplayPriority = 2))
	TObjectPtr<UFlowActorSpawnRecord> SpawnRecordTemplate;

public:
	UFlowNodeAddOn_FightActorsCohortFixedCount();
	virtual void PostInitProperties() override;

#if WITH_SERVER_CODE
	virtual int32 ExecuteSpawningPass(EFlowFightActorSpawnMethod Method,
		FFlowActorSpawningAssistant& Assistant, bool bIsInitialPass) override;
#endif

protected:
#if WITH_SERVER_CODE
	virtual int32 GetInitialActorCount() const override;
#endif
	virtual EFlowAddOnAcceptResult AcceptFlowNodeAddOnChild_Implementation(
		const UFlowNodeAddOn* AddOnTemplate,
		const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const override;
	virtual void UpdateNodeConfigText_Implementation() override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
