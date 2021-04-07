#include "FlowWorldSettings.h"
#include "FlowComponent.h"

AFlowWorldSettings::AFlowWorldSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FlowComponent = CreateDefaultSubobject<UFlowComponent>(TEXT("FlowComponent"));

	// We need this if project uses custom AWorldSettings classed inherited after this one
	// In this case engine would call BeginPlay multiple times... for AFlowWorldSettings and every inherited AWorldSettings class...
	FlowComponent->bAllowMultipleInstances = false;
}

void AFlowWorldSettings::PostLoad()
{
	Super::PostLoad();

	if (FlowAsset_DEPRECATED)
	{
		FlowComponent->RootFlow = FlowAsset_DEPRECATED;
	}
}
