// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "ActorSpawning/FlowDefaultActorSpawnRecord.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Templates/UnrealTemplate.h"

#include "ActorSpawning/FlowActorSpawnRecordOwner.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowDefaultActorSpawnRecord)

#if WITH_SERVER_CODE
bool UFlowDefaultActorSpawnRecord::AcquireActor()
{
	// Acquire a deferred actor so the owner's pre-finish callback can initialize it before BeginPlay.
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}
	if (!IsValid(GetConfiguredActorClass()))
	{
		return false;
	}

	SpawnedActor = World->SpawnActorDeferred<AActor>(
		GetConfiguredActorClass().Get(), ResolvedSpawnTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!IsValid(SpawnedActor))
	{
		SpawnedActor = nullptr;
		return false;
	}

	// Keep an external-destruction observer for both deferred and finished actors.
	bDeferredActorOwned = true;
	SpawnedActor->OnDestroyed.AddDynamic(this, &UFlowDefaultActorSpawnRecord::OnSpawnedActorDestroyed);
	return true;
}

bool UFlowDefaultActorSpawnRecord::FinishAcquiredActor()
{
	// Run construction and BeginPlay only after pre-finish configuration has seen the deferred actor.
	AActor* Actor = SpawnedActor.Get();
	if (!bDeferredActorOwned || !IsValid(Actor))
	{
		return false;
	}

	Actor->FinishSpawning(ResolvedSpawnTransform);
	bDeferredActorOwned = false;
	return SpawnedActor == Actor && IsValid(Actor);
}

void UFlowDefaultActorSpawnRecord::ReleaseOwnedActor(bool bDestroyActor)
{
	AActor* Actor = SpawnedActor.Get();
	if (!Actor)
	{
		return;
	}
	if (bReleasingActor)
	{
		return;
	}

	// Unbind before notifying the owner; its cleanup callback can re-enter this record.
	TGuardValue<bool> Guard(bReleasingActor, true);
	Actor->OnDestroyed.RemoveDynamic(this, &UFlowDefaultActorSpawnRecord::OnSpawnedActorDestroyed);
	bDeferredActorOwned = false;
	// Keep GetOwnedActor valid for the cleanup callback, then clear the record exactly once.
	if (IFlowActorSpawnRecordOwner* Owner = GetRecordOwner())
	{
		Owner->OnCleanupSpawnedInstance(*this);
	}

	SpawnedActor = nullptr;
	if (!bDestroyActor || !IsValid(Actor) || Actor->IsActorBeingDestroyed())
	{
		return;
	}

	// World teardown already owns actor destruction; do not destroy the same actor during cleanup.
	UWorld* World = Actor->GetWorld();
	if (IsValid(World) && !World->bIsTearingDown)
	{
		Actor->Destroy();
	}
}
#endif

void UFlowDefaultActorSpawnRecord::OnSpawnedActorDestroyed(AActor* Actor)
{
#if WITH_SERVER_CODE
	// An external destroy releases ownership without attempting another Destroy call.
	if (SpawnedActor == Actor)
	{
		ReleaseOwnedActor(false);
	}
#endif
}
