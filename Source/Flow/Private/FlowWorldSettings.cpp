#include "FlowWorldSettings.h"
#include "FlowComponent.h"

AFlowWorldSettings::AFlowWorldSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FlowComponent = CreateDefaultSubobject<UFlowComponent>(TEXT("FlowComponent"));
}

void AFlowWorldSettings::PostLoad()
{
	Super::PostLoad();

	if (FlowAsset_DEPRECATED)
	{
		FlowComponent->RootFlow = FlowAsset_DEPRECATED;
	}
}
