// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "ActorSpawning/FlowActorSpawnRecord.h"

#include "FlowDefaultActorSpawnRecord.generated.h"

/** Spawns an ordinary deferred engine actor and owns it until cleanup or external destruction. */
UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class FLOWGAMEFRAMEWORK_API UFlowDefaultActorSpawnRecord : public UFlowActorSpawnRecord
{
	GENERATED_BODY()

protected:
	/** Acquired actor, available to pre-finish callbacks before BeginPlay. */
	UPROPERTY(Transient)
	TObjectPtr<AActor> SpawnedActor;

	/** True between deferred acquisition and FinishSpawning, independently of the record's state. */
	bool bDeferredActorOwned = false;

	/** Guards cleanup callbacks from releasing the same actor recursively. */
	bool bReleasingActor = false;

public:
#if WITH_SERVER_CODE
	// -- UFlowActorSpawnRecord
	virtual AActor* GetOwnedActor() const override { return SpawnedActor.Get(); }

protected:
	virtual bool AcquireActor() override;
	virtual bool FinishAcquiredActor() override;
	virtual void ReleaseOwnedActor(bool bDestroyActor) override;
#endif

	/** Drop ownership when another system destroys the actor. */
	UFUNCTION()
	void OnSpawnedActorDestroyed(AActor* Actor);
};
