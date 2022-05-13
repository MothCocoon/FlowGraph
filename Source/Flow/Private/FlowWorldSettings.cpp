// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowWorldSettings.h"
#include "FlowComponent.h"
#include "FlowSubsystem.h"

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

void AFlowWorldSettings::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (!IsValidInstance())
	{
		GetFlowComponent()->bAutoStartRootFlow = false;
	}
}

bool AFlowWorldSettings::IsValidInstance() const
{
	if (const UWorld* World = GetWorld())
	{
		// workaround to prevent starting Flow from stray AWorldSettings actor that still exists in the world
		// cause of this issue fixed in UE 5.0: https://github.com/EpicGames/UnrealEngine/commit/001f50b8b55507940f9c2cb1349592c692aae2c1?diff=unified
		if (World->GetWorldSettings() == this)
		{
			return true;
		}
	}

	return false;
}
