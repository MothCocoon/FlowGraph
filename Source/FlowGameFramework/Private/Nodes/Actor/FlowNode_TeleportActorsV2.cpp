// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Actor/FlowNode_TeleportActorsV2.h"

#include "AddOns/FlowNodeAddOn_TeleportExecution.h"
#include "GameFramework/Actor.h"
#include "Interfaces/FlowTeleportExecutionAddOnInterface.h"
#include "Internationalization/Text.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_TeleportActorsV2)

const FName UFlowNode_TeleportActorsV2::INPIN_Start(TEXT("Start"));
const FName UFlowNode_TeleportActorsV2::INPIN_Actors(TEXT("Actors"));
const FName UFlowNode_TeleportActorsV2::INPIN_Destinations(TEXT("Destinations"));
const FName UFlowNode_TeleportActorsV2::OUTPIN_AllTeleportsSucceeded(TEXT("All Teleports Succeeded"));
const FName UFlowNode_TeleportActorsV2::OUTPIN_AllTeleportAttemptsComplete(TEXT("All Teleport Attempts Complete"));
const FName UFlowNode_TeleportActorsV2::OUTPIN_TeleportSucceeded(TEXT("Teleport Succeeded"));
const FName UFlowNode_TeleportActorsV2::OUTPIN_TeleportFailed(TEXT("Teleport Failed"));

UFlowNode_TeleportActorsV2::UFlowNode_TeleportActorsV2()
{
#if WITH_EDITOR
	Category = TEXT("Flow|Actors");
#endif

	InputPins.Reset();
	InputPins.Add(FFlowPin(INPIN_Start, FText::FromName(INPIN_Start), TEXT("Starts the teleport operation.")));
	InputPins.Add(FFlowPin(INPIN_Actors, FFlowPinType_Object::GetPinTypeNameStatic()));
#if WITH_EDITORONLY_DATA
	InputPins.Last().PinToolTip = TEXT("Actors to teleport.");
#endif
	InputPins.Last().ContainerType = EPinContainerType::Array;
	InputPins.Add(FFlowPin(INPIN_Destinations, FFlowPinType_Transform::GetPinTypeNameStatic()));
#if WITH_EDITORONLY_DATA
	InputPins.Last().PinToolTip = TEXT("Candidate destination transforms.");
#endif
	InputPins.Last().ContainerType = EPinContainerType::Array;

	OutputPins.Reset();
	OutputPins.Add(FFlowPin(OUTPIN_AllTeleportsSucceeded));
	OutputPins.Add(FFlowPin(OUTPIN_AllTeleportAttemptsComplete));
	OutputPins.Add(FFlowPin(OUTPIN_TeleportSucceeded));
	OutputPins.Add(FFlowPin(OUTPIN_TeleportFailed));

	ActorSelector.InitializeAs<FFlowTeleportActorSelector_ActorsDataPin>();
	DestinationSelector.InitializeAs<FFlowTeleportDestinationSelector_TransformsDataPin>();
}

EFlowAddOnAcceptResult UFlowNode_TeleportActorsV2::AcceptFlowNodeAddOnChild_Implementation(
	const UFlowNodeAddOn* AddOnTemplate,
	const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const
{
	if (!IFlowTeleportExecutionAddOnInterface::ImplementsInterfaceSafe(AddOnTemplate))
	{
		return EFlowAddOnAcceptResult::Undetermined;
	}

	return HasOtherDirectAddOnChildMatching(
		*UFlowTeleportExecutionAddOnInterface::StaticClass(),
		AddOnTemplate,
		AdditionalAddOnsToAssumeAreChildren)
		? EFlowAddOnAcceptResult::Reject
		: EFlowAddOnAcceptResult::TentativeAccept;
}

#if WITH_EDITOR

void UFlowNode_TeleportActorsV2::UpdateNodeConfigText_Implementation()
{
	if (!ActorSelector.IsValid())
	{
		SetNodeConfigText(FText::FromString(TEXT("Actor selector is invalid!")));
		return;
	}

	if (!DestinationSelector.IsValid())
	{
		SetNodeConfigText(FText::FromString(TEXT("Destination selector is invalid!")));
		return;
	}

	FTextBuilder TextBuilder;
	if (const FFlowTeleportActorSelector* ActorSelectorConfig = ActorSelector.GetPtr<FFlowTeleportActorSelector>())
	{
		ActorSelectorConfig->AppendNodeConfigText(TextBuilder);
	}

	if (const FFlowTeleportDestinationSelector* DestinationSelectorConfig = DestinationSelector.GetPtr<FFlowTeleportDestinationSelector>())
	{
		DestinationSelectorConfig->AppendNodeConfigText(TextBuilder);
	}

	SetNodeConfigText(TextBuilder.ToText());
}

EDataValidationResult UFlowNode_TeleportActorsV2::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult SuperResult = Super::IsDataValid(Context);

	if (!ActorSelector.IsValid() || ActorSelector.GetScriptStruct() == FFlowTeleportActorSelector::StaticStruct())
	{
		Context.AddError(FText::FromString(TEXT("What to Teleport? must select a concrete actor selector.")));
		return EDataValidationResult::Invalid;
	}

	if (!DestinationSelector.IsValid() || DestinationSelector.GetScriptStruct() == FFlowTeleportDestinationSelector::StaticStruct())
	{
		Context.AddError(FText::FromString(TEXT("Where to Teleport? must select a concrete destination selector.")));
		return EDataValidationResult::Invalid;
	}

	return SuperResult;
}

const FFlowAgentDoc& UFlowNode_TeleportActorsV2::GetAgentDoc() const
{
	static const FFlowAgentDoc Doc = MakeAgentDoc(
		TEXT("Use Teleport Actors to relocate existing actors. Configure What to Teleport and Where to Teleport. The default configs read the Actors object-array and Destinations transform-array pins and use direct AActor::TeleportTo. Destination assignment is explicit: In Order and Random Without Replacement require enough destinations, while Single Destination For All requires exactly one. Teleport Succeeded and Teleport Failed fire once per actor, with Teleport Actor identifying the current actor and Teleport Failure Reason identifying a per-actor failure. All Teleports Succeeded fires only when every operation succeeds, and All Teleport Attempts Complete fires after every operation reaches a terminal state. The Teleported Actors output contains the cumulative successful actors for the activation. Optional execution addons may provide feature-specific behavior. It does not provide spawning, pathfinding, NavMesh, ground, EQS, Stage Marker, Blackboard-array, or Action behavior implicitly."),
		{ TEXT("teleport"), TEXT("actor"), TEXT("relocate"), TEXT("destination"), TEXT("transform") },
		{});
	return Doc;
}
#endif

void UFlowNode_TeleportActorsV2::ExecuteInput(const FName& PinName)
{
	if (PinName == INPIN_Start)
	{
		if (State != EFlowTeleportState::Invalid)
		{
			SetState(EFlowTeleportState::Invalid);
		}

		SetState(EFlowTeleportState::Starting);
		TeleportActors();
	}

	Super::ExecuteInput(PinName);
}

void UFlowNode_TeleportActorsV2::DeinitializeInstance()
{
	SetState(EFlowTeleportState::Invalid);

	Super::DeinitializeInstance();
}

void UFlowNode_TeleportActorsV2::TeleportActors()
{
	FLOW_ASSERT_ENUM_MAX(EFlowTeleportState, 3);
	checkf(
		State == EFlowTeleportState::Starting,
		TEXT("TeleportActors must only run while the teleport activation is in the Starting state."));

	// Reset per-activation output context before resolving the selector inputs.
	CurrentTeleportActor = nullptr;
	TeleportFailureReason = EFlowTeleportFailureReason::None;
	TeleportedActors.Reset();

	// Resolve the actors and candidate destinations.
	TArray<AActor*> Actors;
	const FFlowTeleportActorSelector* ActorSelectorConfig = ActorSelector.GetPtr<FFlowTeleportActorSelector>();
	FText FailureReason;
	if (ActorSelectorConfig == nullptr || !ActorSelectorConfig->ResolveActors(*this, Actors, FailureReason))
	{
		Activation.bAllTeleportsSucceeded = false;
		SetState(EFlowTeleportState::Completed);
		return;
	}

	TArray<FTransform> Candidates;
	const FFlowTeleportDestinationSelector* DestinationSelectorConfig = DestinationSelector.GetPtr<FFlowTeleportDestinationSelector>();
	if (DestinationSelectorConfig == nullptr || !DestinationSelectorConfig->ResolveDestinations(*this, Candidates, FailureReason))
	{
		Activation.bAllTeleportsSucceeded = false;
		SetState(EFlowTeleportState::Completed);
		return;
	}

	// Assign each actor to a candidate destination.
	TArray<int32> CandidateIndices;

	FRandomStream RandomStream(GetRandomSeed());

	if (!FlowTeleport::AssignDestinations(DestinationSelectorConfig->Assignment, Actors.Num(), Candidates, RandomStream, CandidateIndices, FailureReason))
	{
		Activation.bAllTeleportsSucceeded = false;
		SetState(EFlowTeleportState::Completed);
		return;
	}

	// Start one operation for each actor. Operations may complete immediately or
	// remain pending until an execution addon reports completion.
	for (int32 ActorIndex = 0; ActorIndex < Actors.Num(); ++ActorIndex)
	{
		CurrentTeleportActor = Actors[ActorIndex];
		TeleportFailureReason = EFlowTeleportFailureReason::None;
		FTransform Destination = Candidates[CandidateIndices[ActorIndex]];
		DestinationSelectorConfig->AdjustDestination(*this, *CurrentTeleportActor, Destination);
		ExecuteTeleportOperation(*CurrentTeleportActor, Destination);
	}

	if (Activation.PendingActors.IsEmpty())
	{
		SetState(EFlowTeleportState::Completed);
	}
	else
	{
		SetState(EFlowTeleportState::WaitingForOperations);
	}
}

void UFlowNode_TeleportActorsV2::ExecuteTeleportOperation(AActor& Actor, const FTransform& Destination)
{
	checkf(
		State == EFlowTeleportState::Starting,
		TEXT("ExecuteTeleportOperation must only run while TeleportActors is starting operations."));

	// Register the operation before dispatch so asynchronous addons can complete it.
	CurrentTeleportActor = &Actor;
	TeleportFailureReason = EFlowTeleportFailureReason::None;

	const FGuid OperationGuid = FGuid::NewGuid();
	Activation.PendingActors.Add(OperationGuid, CurrentTeleportActor);

	// Give execution addons the first opportunity to handle the teleport.
	EFlowTeleportExecutionResult ExecutionResult = EFlowTeleportExecutionResult::UseDefault;
	ForEachAddOnForClass<UFlowTeleportExecutionAddOnInterface>(
		[&ExecutionResult, this, &Destination, OperationGuid](UFlowNodeAddOn& AddOn)
		{
			IFlowTeleportExecutionAddOnInterface* ExecutionAddOn = CastChecked<IFlowTeleportExecutionAddOnInterface>(&AddOn);
			ExecutionResult = ExecutionAddOn->ExecuteTeleport(*this, *CurrentTeleportActor, Destination, OperationGuid);

			return EFlowForEachAddOnFunctionReturnValue::BreakWithSuccess;
	});

	FLOW_ASSERT_ENUM_MAX(EFlowTeleportExecutionResult, 4);

	// Complete immediately when no addon owns the operation or reports a terminal result.
	if (ExecutionResult == EFlowTeleportExecutionResult::UseDefault)
	{
		const bool bTeleported = CurrentTeleportActor->TeleportTo(Destination.GetLocation(), Destination.Rotator());
		CompleteTeleportOperation(Activation.Guid, OperationGuid, bTeleported,
			bTeleported ? EFlowTeleportFailureReason::None : EFlowTeleportFailureReason::TeleportRejected);
	}
	else if (ExecutionResult == EFlowTeleportExecutionResult::Succeeded)
	{
		CompleteTeleportOperation(Activation.Guid, OperationGuid, true);
	}
	else if (ExecutionResult == EFlowTeleportExecutionResult::Failed)
	{
		CompleteTeleportOperation(Activation.Guid, OperationGuid, false, EFlowTeleportFailureReason::ExecutionAddonFailed);
	}
}

void UFlowNode_TeleportActorsV2::SetState(const EFlowTeleportState NextState)
{
	FLOW_ASSERT_ENUM_MAX(EFlowTeleportState, 3);
	if (State == NextState)
	{
		return;
	}

	const EFlowTeleportState PreviousState = State;
	State = NextState;

	ManageWaitingForOperations(PreviousState, NextState);

	// Entering state effects
	switch (NextState)
	{
	case EFlowTeleportState::Starting:
		{
			ResetActivation();
			Activation.Guid = FGuid::NewGuid();
		}
		break;

	case EFlowTeleportState::WaitingForOperations:
		break;

	case EFlowTeleportState::Completed:
		{
			TriggerCompletion();
		}
		break;

	case EFlowTeleportState::Invalid:
		{
			ResetActivation();
		}
		break;

	default:
		checkNoEntry();
		break;
	}

	checkf(
		State == NextState,
		TEXT("State must not change as a side-effect of processing a state transition; SetState must finish in the requested NextState."));
}

void UFlowNode_TeleportActorsV2::ManageWaitingForOperations(
	const EFlowTeleportState PreviousState,
	const EFlowTeleportState NextState)
{
	FLOW_ASSERT_ENUM_MAX(EFlowTeleportState, 3);
	const bool bWasWaitingForOperations = PreviousState == EFlowTeleportState::WaitingForOperations;
	const bool bIsWaitingForOperations = NextState == EFlowTeleportState::WaitingForOperations;
	if (bWasWaitingForOperations && !bIsWaitingForOperations)
	{
		CancelTeleportExecutionAddOn();
	}
}

void UFlowNode_TeleportActorsV2::ResetActivation()
{
	checkf(
		EFlowTeleportState_Classifiers::IsActivationResetState(State),
		TEXT("ResetActivation is only valid while entering Starting or Invalid."));

	Activation.Reset();
	CurrentTeleportActor = nullptr;
	TeleportFailureReason = EFlowTeleportFailureReason::None;
	TeleportedActors.Reset();
}

void UFlowNode_TeleportActorsV2::TriggerCompletion()
{
	checkf(
		State == EFlowTeleportState::Completed,
		TEXT("TriggerCompletion must only broadcast completion while the state is Completed."));

	if (Activation.bAllTeleportsSucceeded)
	{
		TriggerOutput(OUTPIN_AllTeleportsSucceeded, false);
	}

	TriggerOutput(OUTPIN_AllTeleportAttemptsComplete, true);
}

void UFlowNode_TeleportActorsV2::CompleteTeleportOperation(
	const FGuid& InActivationGuid,
	const FGuid& OperationGuid,
	const bool bSucceeded,
	const EFlowTeleportFailureReason FailureReason)
{
	FLOW_ASSERT_ENUM_MAX(EFlowTeleportState, 3);
	if (InActivationGuid != Activation.Guid || !Activation.PendingActors.Contains(OperationGuid))
	{
		return;
	}

	checkf(
		EFlowTeleportState_Classifiers::IsOperationInProgressState(State),
		TEXT("A valid teleport operation may only complete while the activation is Starting or WaitingForOperations."));

	AActor* Actor = Activation.PendingActors.FindRef(OperationGuid);
	Activation.PendingActors.Remove(OperationGuid);
	if (IsValid(Actor))
	{
		CurrentTeleportActor = Actor;
		TeleportFailureReason = bSucceeded ? EFlowTeleportFailureReason::None : FailureReason;

		if (bSucceeded)
		{
			TeleportedActors.Add(Actor);

			constexpr bool bFinish = false;
			TriggerOutput(OUTPIN_TeleportSucceeded, bFinish);
		}
		else
		{
			Activation.bAllTeleportsSucceeded = false;

			constexpr bool bFinish = false;
			TriggerOutput(OUTPIN_TeleportFailed, bFinish);
		}
	}
	else
	{
		Activation.bAllTeleportsSucceeded = false;
	}

	if (Activation.PendingActors.IsEmpty() && State == EFlowTeleportState::WaitingForOperations)
	{
		checkf(
			State == EFlowTeleportState::WaitingForOperations,
			TEXT("Teleport completion may only transition to Completed from WaitingForOperations."));
		SetState(EFlowTeleportState::Completed);
	}
}

void UFlowNode_TeleportActorsV2::CancelTeleportExecutionAddOn()
{
	ForEachAddOnForClass<UFlowTeleportExecutionAddOnInterface>(
		[this](UFlowNodeAddOn& AddOn)
		{
			IFlowTeleportExecutionAddOnInterface* ExecutionAddOn = CastChecked<IFlowTeleportExecutionAddOnInterface>(&AddOn);
			ExecutionAddOn->CancelTeleport(*this);

			return EFlowForEachAddOnFunctionReturnValue::Continue;
		});
}
