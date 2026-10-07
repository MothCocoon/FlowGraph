// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "ActorSpawning/FlowDefaultActorSpawnRecord.h"
#include "Containers/Array.h"

#include "FlowIndexedTransformActorSpawnRecord.generated.h"

/** Spawns each record index at its authored transform, with an actor-owner fallback. */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DisplayName = "Spawn at Indexed Transforms")
class FLOWGAMEFRAMEWORK_API UFlowIndexedTransformActorSpawnRecord : public UFlowDefaultActorSpawnRecord
{
	GENERATED_BODY()

protected:
	/** Index-aligned spawn transforms; the record's stable index is reused on refill. */
	UPROPERTY(EditAnywhere, Category = Configuration, DisplayName = "Spawn Transforms")
	TArray<FTransform> SpawnTransforms;

	/** If enabled, a missing indexed transform fails instead of using the actor owner's transform. */
	UPROPERTY(EditAnywhere, Category = Configuration)
	bool bRequireIndexedTransform = false;

#if WITH_SERVER_CODE
	virtual bool AdvanceCurrentState(EFlowActorSpawnRecordState& OutNextState) override;
	virtual FTransform GetDesiredSpawnTransform() const override;
#endif

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
