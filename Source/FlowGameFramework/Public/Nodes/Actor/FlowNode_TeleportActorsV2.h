// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/FlowNode.h"
#include "StructUtils/InstancedStruct.h"
#include "Types/FlowTeleportSelectors.h"
#include "Types/FlowTeleportTypes.h"

#include "FlowNode_TeleportActorsV2.generated.h"

class AActor;

// Teleport state machine enum
enum class EFlowTeleportState : uint8
{
	/** Resolving selectors and starting teleport operations. */
	Starting,
	/** Waiting for execution addons to complete asynchronous operations. */
	WaitingForOperations,
	/** All teleport operations have reached a terminal state. */
	Completed,

	/** Sentinel used to validate the range of teleport states. */
	Max,
	/** No teleport activation is currently active. */
	Invalid,
	/** First valid value in the enum range. */
	Min = 0,
};
FLOW_ENUM_RANGE_VALUES(EFlowTeleportState);

namespace EFlowTeleportState_Classifiers
{
	FORCEINLINE bool IsOperationInProgressState(const EFlowTeleportState State)
	{
		return State == EFlowTeleportState::Starting || State == EFlowTeleportState::WaitingForOperations;
	}

	FORCEINLINE bool IsActivationResetState(const EFlowTeleportState State)
	{
		return State == EFlowTeleportState::Starting || State == EFlowTeleportState::Invalid;
	}
}

/** Runtime identity and aggregate result for one teleport activation. */
USTRUCT()
struct FLOWGAMEFRAMEWORK_API FFlowTeleportActivation
{
	GENERATED_BODY()

	/** Identifies the activation so late completion callbacks can be ignored safely. */
	UPROPERTY(Transient)
	FGuid Guid;

	/** Teleport operations that have been started but have not reached a terminal state. */
	UPROPERTY(Transient)
	TMap<FGuid, TObjectPtr<AActor>> PendingActors;

	/** True only when every teleport operation in this activation succeeds. */
	UPROPERTY(Transient)
	bool bAllTeleportsSucceeded = true;

	void Reset()
	{
		Guid.Invalidate();
		PendingActors.Reset();
		bAllTeleportsSucceeded = true;
	}
};

// NOTE (gtaylor) This is a Teleport Actors flow node, adapted from a version we had internally.
// It is branded here as 'v2' as a disambiguator, since our version was named the same.
// It has a number of different teleport configuration options, combined from multiple different
// teleport cases we were using, across a few different teleport nodes.  So we combined them
// all into a single teleport node.
//
// You can customize how the teleport operation works by implementing a TeleportExecution AddOn,
// which can augment or replace the default teleportation mechanics that this node implements.
//
// You can also customize how you select which actors to teleport and also the destinations to
// teleport them to by developing customized 'selector' structs.

// Relocate one or more existing actors to explicit destinations.
UCLASS(BlueprintType, meta = (DisplayName = "Teleport Actors", Keywords = "Teleport Actor Relocate Destination Transform"))
class FLOWGAMEFRAMEWORK_API UFlowNode_TeleportActorsV2 : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowNode_TeleportActorsV2();

	virtual EFlowAddOnAcceptResult AcceptFlowNodeAddOnChild_Implementation(
		const UFlowNodeAddOn* AddOnTemplate,
		const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const override;

#if WITH_EDITOR
	virtual void UpdateNodeConfigText_Implementation() override;
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	virtual const FFlowAgentDoc& GetAgentDoc() const override;
#endif

	virtual void ExecuteInput(const FName& PinName) override;
	virtual void DeinitializeInstance() override;

	void CompleteTeleportOperation(
		const FGuid& InActivationGuid,
		const FGuid& OperationGuid,
		bool bSucceeded,
		EFlowTeleportFailureReason FailureReason = EFlowTeleportFailureReason::None);

	const FGuid& GetActivationGuid() const { return Activation.Guid; }

protected:
	void TeleportActors();
	void ExecuteTeleportOperation(AActor& Actor, const FTransform& Destination);
	void SetState(EFlowTeleportState NextState);
	void ManageWaitingForOperations(EFlowTeleportState PreviousState, EFlowTeleportState NextState);
	void ResetActivation();
	void TriggerCompletion();
	void CancelTeleportExecutionAddOn();

protected:
	/** Selects how the node resolves actors to teleport. */
	UPROPERTY(EditAnywhere, Category = "Configuration", NoClear, meta = (ExcludeBaseStruct, BaseStruct = "/Script/FlowGameFramework.FlowTeleportActorSelector", DisplayName = "What to Teleport?", ToolTip = "Select how the node resolves actors to teleport."))
	FInstancedStruct ActorSelector;

	/** Selects how the node resolves and assigns teleport destinations. */
	UPROPERTY(EditAnywhere, Category = "Configuration", NoClear, meta = (ExcludeBaseStruct, BaseStruct = "/Script/FlowGameFramework.FlowTeleportDestinationSelector", DisplayName = "Where to Teleport?", ToolTip = "Select how the node resolves and assigns destinations."))
	FInstancedStruct DestinationSelector;

	/** Current aggregate lifecycle state for this node's teleport activation. */
	EFlowTeleportState State = EFlowTeleportState::Invalid;

	/** Owns the activation identity, pending operations, and aggregate result. */
	UPROPERTY(Transient)
	FFlowTeleportActivation Activation;

	/** Actor associated with the most recently processed or completed operation. */
	UPROPERTY(Transient, meta = (SourceForOutputFlowPin = "Teleport Actor", FlowPinType = "Object"))
	TObjectPtr<AActor> CurrentTeleportActor;

	/** Failure reason associated with the most recently processed operation. */
	UPROPERTY(Transient, meta = (SourceForOutputFlowPin = "Teleport Failure Reason", FlowPinType = "Enum"))
	EFlowTeleportFailureReason TeleportFailureReason = EFlowTeleportFailureReason::None;

	/** Actors successfully teleported during the current activation. */
	UPROPERTY(Transient, meta = (SourceForOutputFlowPin = "Teleported Actors", FlowPinType = "Object"))
	TArray<AActor*> TeleportedActors;

	static const FName INPIN_Start;
	static const FName INPIN_Actors;
	static const FName INPIN_Destinations;
	static const FName OUTPIN_AllTeleportsSucceeded;
	static const FName OUTPIN_AllTeleportAttemptsComplete;
	static const FName OUTPIN_TeleportSucceeded;
	static const FName OUTPIN_TeleportFailed;
};
