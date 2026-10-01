// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Types/FlowTeleportTypes.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"
#include "Internationalization/Text.h"
#include "NavigationSystem.h"
#include "Nodes/FlowNode.h"
#include "Types/FlowPinTypesStandard.h"
#include "Types/FlowTeleportSelectors.h"

#include "Math/RandomStream.h"

#if WITH_EDITOR
void FFlowTeleportActorSelector_ActorsDataPin::AppendNodeConfigText(FTextBuilder& TextBuilder) const
{
	TextBuilder.AppendLine(FText::FromString(TEXT("Actors: input pin")));
}

void FFlowTeleportDestinationSelector::AppendNodeConfigText(FTextBuilder& TextBuilder) const
{
	const UEnum* AssignmentEnum = StaticEnum<EFlowTeleportDestinationAssignment>();
	const FText AssignmentText = AssignmentEnum != nullptr
		? AssignmentEnum->GetDisplayNameTextByValue(static_cast<int64>(Assignment))
		: FText::FromString(TEXT("Unknown"));

	TextBuilder.AppendLine(FText::Format(
		FText::FromString(TEXT("Assignment: {0}")),
		AssignmentText));
}

void FFlowTeleportDestinationSelector_TransformsDataPin::AppendNodeConfigText(FTextBuilder& TextBuilder) const
{
	TextBuilder.AppendLine(FText::FromString(TEXT("Destinations: input pin")));
	Super::AppendNodeConfigText(TextBuilder);
}

#endif // WITH_EDITOR

bool FFlowTeleportActorSelector::ResolveActors(const UFlowNode& Node, TArray<AActor*>& OutActors, FText& OutFailureReason) const
{
	OutActors.Reset();
	OutFailureReason = FText::FromString(TEXT("The selected actor selector does not resolve actors."));
	return false;
}

bool FFlowTeleportActorSelector_ActorsDataPin::ResolveActors(const UFlowNode& Node, TArray<AActor*>& OutActors, FText& OutFailureReason) const
{
	OutActors.Reset();
	OutFailureReason = FText::GetEmpty();

	TArray<TObjectPtr<UObject>> ActorObjects;
	if (Node.TryResolveDataPinValues<FFlowPinType_Object>(FName(TEXT("Actors")), ActorObjects) != EFlowDataPinResolveResult::Success)
	{
		OutFailureReason = FText::FromString(TEXT("The Actors data pin could not be resolved."));
		return false;
	}

	for (UObject* Object : ActorObjects)
	{
		AActor* Actor = Cast<AActor>(Object);
		if (IsValid(Actor) && !OutActors.Contains(Actor))
		{
			OutActors.Add(Actor);
		}
	}

	if (OutActors.IsEmpty())
	{
		OutFailureReason = FText::FromString(TEXT("The Actors data pin resolved to no valid actors."));
		return false;
	}

	return true;
}

bool FFlowTeleportDestinationSelector::ResolveDestinations(const UFlowNode& Node, TArray<FTransform>& OutDestinations, FText& OutFailureReason) const
{
	OutDestinations.Reset();
	OutFailureReason = FText::FromString(TEXT("The selected destination selector does not resolve destinations."));
	return false;
}

void FFlowTeleportDestinationSelector::AdjustDestination(const UFlowNode& Node, const AActor& Actor, FTransform& InOutDestination) const
{
}

bool FFlowTeleportDestinationSelector_TransformsDataPin::ResolveDestinations(
	const UFlowNode& Node,
	TArray<FTransform>& OutDestinations,
	FText& OutFailureReason) const
{
	OutDestinations.Reset();
	OutFailureReason = FText::GetEmpty();
	if (Node.TryResolveDataPinValues<FFlowPinType_Transform>(FName(TEXT("Destinations")), OutDestinations) != EFlowDataPinResolveResult::Success ||
		OutDestinations.IsEmpty())
	{
		OutFailureReason = FText::FromString(TEXT("The Destinations data pin resolved to no transforms."));
		return false;
	}

	return true;
}

void FFlowTeleportDestinationSelector_TransformsDataPin::AdjustDestination(
	const UFlowNode& Node,
	const AActor& Actor,
	FTransform& InOutDestination) const
{
}

void FFlowTeleportDestinationSelector_Navigation::AdjustDestination(
	const UFlowNode& Node,
	const AActor& Actor,
	FTransform& InOutDestination) const
{
	if (bProjectToNavMesh)
	{
		FNavLocation ProjectedLocation;
		UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Node.GetWorld());
		if (IsValid(NavigationSystem) && NavigationSystem->ProjectPointToNavigation(
			InOutDestination.GetLocation(),
			ProjectedLocation,
			NavProjectionExtent))
		{
			InOutDestination.SetLocation(ProjectedLocation.Location);
		}
	}

	if (bAddCharacterCapsuleOffset)
	{
		const ACharacter* Character = Cast<ACharacter>(&Actor);
		const UCapsuleComponent* CapsuleComponent = IsValid(Character) ? Character->GetCapsuleComponent() : nullptr;
		if (IsValid(CapsuleComponent))
		{
			FVector Location = InOutDestination.GetLocation();
			Location.Z += CapsuleComponent->GetScaledCapsuleHalfHeight();
			InOutDestination.SetLocation(Location);
		}
	}
}

// Destination assignment helpers are kept with the shared teleport types because they operate only on these types.
namespace FlowTeleport
{
	bool AssignDestinations(
		const EFlowTeleportDestinationAssignment Assignment,
		const int32 ActorCount,
		const TArray<FTransform>& Candidates,
		FRandomStream& RandomStream,
		TArray<int32>& OutCandidateIndices,
		FText& OutFailureReason)
	{
		FLOW_ASSERT_ENUM_MAX(EFlowTeleportDestinationAssignment, 3);
		OutCandidateIndices.Reset();
		OutFailureReason = FText::GetEmpty();

		if (ActorCount <= 0 || Candidates.IsEmpty())
		{
			OutFailureReason = FText::FromString(TEXT("Teleport requires at least one actor and one destination."));
			return false;
		}

		if (Assignment == EFlowTeleportDestinationAssignment::SingleDestinationForAll)
		{
			if (Candidates.Num() != 1)
			{
				OutFailureReason = FText::FromString(TEXT("Single Destination For All requires exactly one destination."));
				return false;
			}

			OutCandidateIndices.Init(0, ActorCount);
			return true;
		}

		if (Candidates.Num() < ActorCount)
		{
			OutFailureReason = FText::FromString(TEXT("Teleport requires at least one destination per actor."));
			return false;
		}

		if (Assignment == EFlowTeleportDestinationAssignment::InOrder)
		{
			OutCandidateIndices.SetNumUninitialized(ActorCount);
			for (int32 ActorIndex = 0; ActorIndex < ActorCount; ++ActorIndex)
			{
				OutCandidateIndices[ActorIndex] = ActorIndex;
			}

			return true;
		}

		TArray<int32> RemainingIndices;
		RemainingIndices.Reserve(Candidates.Num());
		for (int32 CandidateIndex = 0; CandidateIndex < Candidates.Num(); ++CandidateIndex)
		{
			RemainingIndices.Add(CandidateIndex);
		}

		OutCandidateIndices.Reserve(ActorCount);
		for (int32 ActorIndex = 0; ActorIndex < ActorCount; ++ActorIndex)
		{
			const int32 RemainingIndex = RandomStream.RandRange(0, RemainingIndices.Num() - 1);
			OutCandidateIndices.Add(RemainingIndices[RemainingIndex]);
			RemainingIndices.RemoveAtSwap(RemainingIndex);
		}

		return true;
	}
}
