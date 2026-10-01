// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "AddOns/FlowNodeAddOn_FightActorsCohortFixedCount.h"

#include "ActorSpawning/FlowActorSpawnRecord.h"
#include "ActorSpawning/FlowActorSpawnSelector.h"
#include "ActorSpawning/FlowActorSpawningAssistant.h"
#include "ActorSpawning/FlowDefaultActorSpawnRecord.h"
#include "AddOns/FlowNodeAddOn_PredicateAND.h"
#include "GameFramework/Actor.h"
#include "Interfaces/FlowPredicateInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNodeAddOn_FightActorsCohortFixedCount)

UFlowNodeAddOn_FightActorsCohortFixedCount::UFlowNodeAddOn_FightActorsCohortFixedCount()
{
	ActorSpawnSelector.InitializeAs<FFlowActorSpawnSelector_ActorClass>();
}

void UFlowNodeAddOn_FightActorsCohortFixedCount::PostInitProperties()
{
	Super::PostInitProperties();

#if WITH_EDITOR
	if (!HasAnyFlags(RF_ClassDefaultObject) && !IsValid(SpawnRecordTemplate))
	{
		SpawnRecordTemplate = NewObject<UFlowDefaultActorSpawnRecord>(
			this, UFlowDefaultActorSpawnRecord::StaticClass(), TEXT("SpawnRecordTemplate"), RF_Public | RF_Transactional);
	}
#endif
}

#if WITH_SERVER_CODE
int32 UFlowNodeAddOn_FightActorsCohortFixedCount::ExecuteSpawningPass(
	EFlowFightActorSpawnMethod Method, FFlowActorSpawningAssistant& Assistant, bool bIsInitialPass)
{
	using namespace EFlowFightActorSpawnMethod_Classifiers;
	if (!IsAnySpawnMethod(Method))
	{
		return 0;
	}

	if (!UFlowNodeAddOn_PredicateAND::EvaluatePredicateAND(AddOns))
	{
		return 0;
	}

	if (!IsValid(SpawnRecordTemplate) || !ActorSpawnSelector.GetPtr<FFlowActorSpawnSelector>())
	{
		return INDEX_NONE;
	}

	const FFlowActorSpawningPassConfig Config{
		*this, ActorSpawnSelector, *SpawnRecordTemplate, ActorRecords, EFlowActorSpawnQueueMode::Staggered,
		GetActorSpawningMethodFromFightActorSpawnMethod(Method), bIsInitialPass};

	return Assistant.TryExecuteSpawningPass(Config).NumQueued;
}

int32 UFlowNodeAddOn_FightActorsCohortFixedCount::GetInitialActorCount() const
{
	if (!UFlowNodeAddOn_PredicateAND::EvaluatePredicateAND(AddOns))
	{
		return 0;
	}

	const FFlowActorSpawnSelector* Selector = ActorSpawnSelector.GetPtr<FFlowActorSpawnSelector>();
	if (Selector)
	{
		return Selector->GetNumToSpawn();
	}

	return 0;
}

#endif

EFlowAddOnAcceptResult UFlowNodeAddOn_FightActorsCohortFixedCount::AcceptFlowNodeAddOnChild_Implementation(
	const UFlowNodeAddOn* AddOnTemplate,
	const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const
{
	if (IFlowPredicateInterface::ImplementsInterfaceSafe(AddOnTemplate))
	{
		return EFlowAddOnAcceptResult::TentativeAccept;
	}

	return Super::AcceptFlowNodeAddOnChild_Implementation(AddOnTemplate, AdditionalAddOnsToAssumeAreChildren);
}

void UFlowNodeAddOn_FightActorsCohortFixedCount::UpdateNodeConfigText_Implementation()
{
#if WITH_EDITOR
	const FFlowActorSpawnSelector* Selector = ActorSpawnSelector.GetPtr<FFlowActorSpawnSelector>();
	int32 NumToSpawn = 0;
	if (Selector)
	{
		NumToSpawn = Selector->GetNumToSpawn();
	}

	SetNodeConfigText(FText::FromString(FString::Printf(TEXT("Spawn %d actors"), NumToSpawn)));
#endif
}

#if WITH_EDITOR
EDataValidationResult UFlowNodeAddOn_FightActorsCohortFixedCount::IsDataValid(FDataValidationContext& Context) const
{
	const FFlowActorSpawnSelector* Selector = ActorSpawnSelector.GetPtr<FFlowActorSpawnSelector>();
	if (!IsValid(SpawnRecordTemplate) || !Selector || (Selector->GetNumToSpawn() > 0 && !IsValid(Selector->ChooseActorClassToSpawn(0))))
	{
		return EDataValidationResult::Invalid;
	}

	return Super::IsDataValid(Context);
}
#endif
