// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "ActorSpawning/FlowActorSpawnRecordOwner.h"
#include "ActorSpawning/FlowDefaultActorSpawnRecord.h"
#include "ActorSpawning/FlowActorSpawnSelector.h"
#include "Nodes/Actor/FlowNode_SpawnActors.h"
#include "GameFramework/Actor.h"
#include "Templates/Function.h"

#include "FlowActorSpawningTestTypes.generated.h"

UCLASS(Hidden, NotBlueprintable, NotPlaceable)
class AFlowActorSpawningTestOwner : public AActor, public IFlowActorSpawnRecordOwner
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UFlowActorSpawnRecord>> Records;

#if WITH_SERVER_CODE
	TArray<FName> Events;
	TFunction<void(UFlowActorSpawnRecord&)> FinishedCallback;
	int32 FinishedCount = 0;
	int32 CleanupCount = 0;
	bool bSawActorBeforeFinish = false;
	bool bSawActorAfterFinish = false;
	bool bSawActorDuringCleanup = false;
	bool bCancelBeforeFinish = false;

	virtual AActor* TryGetActorOwner() const override { return nullptr; }
	virtual void ProcessPreSpawnConfiguration(UFlowActorSpawnRecord& Record) override
	{
		Events.Add(TEXT("PreSpawn"));
	}
	virtual void ProcessPreFinishSpawnActor(UFlowActorSpawnRecord& Record) override
	{
		Events.Add(TEXT("PreFinish"));
		AActor* Actor = Record.GetOwnedActor();
		bSawActorBeforeFinish = IsValid(Actor) && !Actor->HasActorBegunPlay();
		if (bCancelBeforeFinish)
		{
			Record.CleanupRuntime();
		}
	}
	virtual void ProcessPostSpawnConfiguration(UFlowActorSpawnRecord& Record) override
	{
		Events.Add(TEXT("PostSpawn"));
		AActor* Actor = Record.GetOwnedActor();
		bSawActorAfterFinish = IsValid(Actor) && Actor->HasActorBegunPlay();
	}
	virtual void ProcessFinishedSpawnAttempt(UFlowActorSpawnRecord& Record) override
	{
		Events.Add(TEXT("Finished"));
		++FinishedCount;
		if (FinishedCallback)
		{
			FinishedCallback(Record);
		}
	}
	virtual void OnCleanupSpawnedInstance(UFlowActorSpawnRecord& Record) override
	{
		Events.Add(TEXT("Cleanup"));
		bSawActorDuringCleanup = Record.GetOwnedActor() != nullptr;
		++CleanupCount;
	}

#endif
};

UCLASS(Hidden, NotBlueprintable, EditInlineNew)
class UFlowFailingActorSpawnTestRecord : public UFlowDefaultActorSpawnRecord
{
	GENERATED_BODY()

#if WITH_SERVER_CODE
protected:
	virtual bool AcquireActor() override { return false; }
#endif
};

UCLASS(Hidden, NotBlueprintable, EditInlineNew)
class UFlowWaitingActorSpawnTestRecord : public UFlowDefaultActorSpawnRecord
{
	GENERATED_BODY()

public:
#if WITH_SERVER_CODE
	bool bWaitForLocation = true;
	void CompleteLocation(bool bSucceeded)
	{
		bWaitForLocation = false;
		CompleteSpawnLocation(bSucceeded);
	}

protected:
	virtual bool AdvanceCurrentState(EFlowActorSpawnRecordState& OutNextState) override
	{
		if (GetRecordState() == EFlowActorSpawnRecordState::WaitingForSpawnLocation && bWaitForLocation)
		{
			return false;
		}
		return Super::AdvanceCurrentState(OutNextState);
	}
#endif
};

USTRUCT()
struct FFlowActorSpawnTestSelector : public FFlowActorSpawnSelector
{
	GENERATED_BODY()

	int32 Count = 3;
	virtual int32 GetNumToSpawn() const override { return Count; }
	virtual TSubclassOf<AActor> ChooseActorClassToSpawn(int32 Index) const override { return AActor::StaticClass(); }
};

UCLASS(Hidden, NotBlueprintable, NotPlaceable, meta = (ExcludeFromFlowCatalog))
class UFlowSpawnActorsTestNode : public UFlowNode_SpawnActors
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<AActor> TestActorOwner;

public:
	TArray<FName> Outputs;

	void SetTestOwner(AActor* Owner)
	{
		TestActorOwner = Owner;
		ActorSpawnSelector.InitializeAs<FFlowActorSpawnTestSelector>();
	}
	int32 GetRecordCount() const { return ActorRecords.Num(); }
	UFlowActorSpawnRecord* GetRecord(int32 Index) const { return ActorRecords[Index]; }
	int32 GetSpawnedCount() const { return SpawnedActors.Num(); }
	virtual UWorld* GetWorld() const override { return TestActorOwner ? TestActorOwner->GetWorld() : nullptr; }
	virtual void TriggerOutput(const FName PinName, bool bFinish = false,
		EFlowPinActivationType ActivationType = EFlowPinActivationType::Default) override
	{
		Outputs.Add(PinName);
	}
#if WITH_SERVER_CODE
	virtual AActor* TryGetActorOwner() const override { return TestActorOwner.Get(); }
#endif
};
