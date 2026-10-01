// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "ActorSpawning/FlowIndexedTransformActorSpawnRecord.h"

#include "ActorSpawning/FlowActorSpawnRecordOwner.h"
#include "GameFramework/Actor.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowIndexedTransformActorSpawnRecord)

#if WITH_SERVER_CODE
bool UFlowIndexedTransformActorSpawnRecord::AdvanceCurrentState(EFlowActorSpawnRecordState& OutNextState)
{
	FLOW_ASSERT_ENUM_MAX(EFlowActorSpawnRecordState, 10);
	if (GetRecordState() == EFlowActorSpawnRecordState::WaitingForSpawnLocation
		&& !SpawnTransforms.IsValidIndex(GetRecordIndex()))
	{
		const IFlowActorSpawnRecordOwner* Owner = GetRecordOwner();
		if (bRequireIndexedTransform || !Owner || !IsValid(Owner->TryGetActorOwner()))
		{
			OutNextState = EFlowActorSpawnRecordState::FailedSpawn;
			return true;
		}
	}

	return Super::AdvanceCurrentState(OutNextState);
}

FTransform UFlowIndexedTransformActorSpawnRecord::GetDesiredSpawnTransform() const
{
	if (SpawnTransforms.IsValidIndex(GetRecordIndex()))
	{
		return SpawnTransforms[GetRecordIndex()];
	}

	return Super::GetDesiredSpawnTransform();
}
#endif

#if WITH_EDITOR
EDataValidationResult UFlowIndexedTransformActorSpawnRecord::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult ParentResult = Super::IsDataValid(Context);
	if (bRequireIndexedTransform && SpawnTransforms.IsEmpty())
	{
		return EDataValidationResult::Invalid;
	}

	return ParentResult;
}
#endif
