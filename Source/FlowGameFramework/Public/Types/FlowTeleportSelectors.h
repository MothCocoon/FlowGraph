// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Types/FlowTeleportTypes.h"
#include "FlowTeleportSelectors.generated.h"

class AActor;
class FTextBuilder;
class UFlowNode;

/** Common configuration base for teleport selectors. */
USTRUCT(BlueprintType, meta = (Hidden))
struct FLOWGAMEFRAMEWORK_API FFlowTeleportConfig
{
	GENERATED_BODY()

	virtual ~FFlowTeleportConfig() = default;
};

/** Resolves the actors that a teleport operation should process. */
USTRUCT(BlueprintType, meta = (Hidden))
struct FLOWGAMEFRAMEWORK_API FFlowTeleportActorSelector : public FFlowTeleportConfig
{
	GENERATED_BODY()

#if WITH_EDITOR
	virtual void AppendNodeConfigText(FTextBuilder& TextBuilder) const
	{
		// Base selectors have no additional configuration summary.
	}
#endif // WITH_EDITOR

	virtual bool ResolveActors(const UFlowNode& Node, TArray<AActor*>& OutActors, FText& OutFailureReason) const;
};

/** Resolves actors from the node's Actors object-array data pin. */
USTRUCT(BlueprintType, DisplayName = "Teleport Actors from Pin")
struct FLOWGAMEFRAMEWORK_API FFlowTeleportActorSelector_ActorsDataPin : public FFlowTeleportActorSelector
{
	GENERATED_BODY()

#if WITH_EDITOR
	virtual void AppendNodeConfigText(FTextBuilder& TextBuilder) const override;
#endif // WITH_EDITOR

	virtual bool ResolveActors(const UFlowNode& Node, TArray<AActor*>& OutActors, FText& OutFailureReason) const override;
};

/** Resolves and adjusts the destinations used by a teleport operation. */
USTRUCT(BlueprintType, meta = (Hidden))
struct FLOWGAMEFRAMEWORK_API FFlowTeleportDestinationSelector : public FFlowTeleportConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Teleport, meta = (ToolTip = "How destinations are assigned to resolved actors."))
	EFlowTeleportDestinationAssignment Assignment = EFlowTeleportDestinationAssignment::InOrder;

#if WITH_EDITOR
	virtual void AppendNodeConfigText(FTextBuilder& TextBuilder) const;
#endif // WITH_EDITOR

	virtual bool ResolveDestinations(const UFlowNode& Node, TArray<FTransform>& OutDestinations, FText& OutFailureReason) const;
	virtual void AdjustDestination(const UFlowNode& Node, const AActor& Actor, FTransform& InOutDestination) const;
};

/** Resolves destinations from the node's Destinations transform-array data pin. */
USTRUCT(BlueprintType, DisplayName = "Teleport Destinations from Pin")
struct FLOWGAMEFRAMEWORK_API FFlowTeleportDestinationSelector_TransformsDataPin : public FFlowTeleportDestinationSelector
{
	GENERATED_BODY()

#if WITH_EDITOR
	virtual void AppendNodeConfigText(FTextBuilder& TextBuilder) const override;
#endif // WITH_EDITOR

	virtual bool ResolveDestinations(const UFlowNode& Node, TArray<FTransform>& OutDestinations, FText& OutFailureReason) const override;
	virtual void AdjustDestination(const UFlowNode& Node, const AActor& Actor, FTransform& InOutDestination) const override;
};

// Intermediate super struct for deriving nav-mesh adjustable teleport destinations
USTRUCT(BlueprintType, meta = (Hidden))
struct FLOWGAMEFRAMEWORK_API FFlowTeleportDestinationSelector_Navigation : public FFlowTeleportDestinationSelector
{
	GENERATED_BODY()

	/** Half-extents used when projecting a destination onto the navigation mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Teleport, DisplayName = "Nav Projection Extent", meta = (DisplayAfter = bProjectToNavMesh, ToolTip = "Half-extents used when projecting each destination onto the navigation mesh."))
	FVector NavProjectionExtent = FVector(500.0f, 500.0f, 500.0f);

	/** Whether each destination should be projected onto the navigation mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Teleport, DisplayName = "Project To NavMesh", meta = (ToolTip = "Project each destination onto the navigation mesh before teleporting."))
	bool bProjectToNavMesh = true;

	/** Whether a character's capsule half-height should be added to the destination. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Teleport, DisplayName = "Add Character Capsule Offset", meta = (ToolTip = "Add a character's capsule half-height to the destination."))
	bool bAddCharacterCapsuleOffset = true;

	virtual void AdjustDestination(const UFlowNode& Node, const AActor& Actor, FTransform& InOutDestination) const override;
};
