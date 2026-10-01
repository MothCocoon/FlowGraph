// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "ActorSpawning/FlowActorSpawnSelector.h"

#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowActorSpawnSelector)

TSubclassOf<AActor> FFlowActorSpawnSelector_ActorClass::ChooseActorClassToSpawn(int32 Index) const
{
	UClass* ActorClass = ActorClassPath.ResolveClass();
	return IsValid(ActorClass) ? ActorClass : ActorClassPath.TryLoadClass<AActor>();
}
