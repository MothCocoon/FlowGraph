// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "FlowWorldSettings.h"

#include "FlowComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowWorldSettings)

AFlowWorldSettings::AFlowWorldSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = false;

	FlowComponent = CreateDefaultSubobject<UFlowComponent>(TEXT("FlowComponent"));

	// We need this if project uses custom AWorldSettings classed inherited after this one
	// In this case engine would call BeginPlay multiple times... for AFlowWorldSettings and every inherited AWorldSettings class...
	FlowComponent->bAllowMultipleInstances = false;
}

void AFlowWorldSettings::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	// Engine reads only the persistent level's WorldSettings at runtime.
	// Sublevel WorldSettings replicate only to serve their own Root Flow.
	if (HasAuthority() && (IsInPersistentLevel() || FlowComponent->RootFlow))
	{
		SetReplicates(true);
	}
}
