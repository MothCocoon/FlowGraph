// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPath.h"

#include "FlowActorSpawnSelector.generated.h"

class AActor;

/** Selects actor classes during a spawning pass; subclasses can prepare and release per-pass selection state. */
USTRUCT(BlueprintType, meta = (Hidden))
struct FLOWGAMEFRAMEWORK_API FFlowActorSpawnSelector
{
	GENERATED_BODY()

	virtual ~FFlowActorSpawnSelector() = default;
	virtual int32 GetNumToSpawn() const { return 0; }
	virtual TSubclassOf<AActor> ChooseActorClassToSpawn(int32 Index) const { return nullptr; }
	virtual void StartSpawningPass() { }
	virtual void FinishSpawningPass() { }
};

/** Spawns a fixed number of actors of one softly referenced class. */
USTRUCT(BlueprintType, DisplayName = "Actor Class")
struct FLOWGAMEFRAMEWORK_API FFlowActorSpawnSelector_ActorClass : public FFlowActorSpawnSelector
{
	GENERATED_BODY()

protected:
	/** Actor class to resolve when a pass selects an instance to spawn. */
	UPROPERTY(EditAnywhere, Category = Configuration, meta = (MetaClass = "/Script/Engine.Actor", DisplayName = "Actor Class", DisplayPriority = 1, ShowContentFromUnreferencedPlugins))
	FSoftClassPath ActorClassPath;

	/** Number of actors requested by each full spawning pass. */
	UPROPERTY(EditAnywhere, Category = Configuration, DisplayName = "Number to Spawn", meta = (ClampMin = 0, UIMin = 0, DisplayPriority = 2))
	int32 NumToSpawn = 1;

public:
	virtual int32 GetNumToSpawn() const override { return NumToSpawn; }
	virtual TSubclassOf<AActor> ChooseActorClassToSpawn(int32 Index) const override;
};
