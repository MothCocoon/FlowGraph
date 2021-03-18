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

	if (FlowAsset && HasAuthority())
	{
		if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
		{
			FlowSubsystem->StartRootFlow(GetWorld(), FlowAsset);
		}
	}
}

void AFlowWorldSettings::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (FlowAsset && HasAuthority())
	{
		if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
		{
			FlowSubsystem->FinishRootFlow(GetWorld(), FlowAsset);
		}
	}

	Super::EndPlay(EndPlayReason);
}
