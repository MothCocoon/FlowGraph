#include "FlowWorldSettings.h"
#include "FlowSubsystem.h"

AFlowWorldSettings::AFlowWorldSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, FlowAsset(nullptr)
{
}

void AFlowWorldSettings::BeginPlay()
{
	Super::BeginPlay();

	if (FlowAsset)
	{
		if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
		{
			FlowSubsystem->StartRootFlow(this, FlowAsset);
		}
	}
}

void AFlowWorldSettings::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (FlowAsset)
	{
		if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
		{
			FlowSubsystem->FinishRootFlow(this, FlowAsset);
		}
	}

	Super::EndPlay(EndPlayReason);
}
