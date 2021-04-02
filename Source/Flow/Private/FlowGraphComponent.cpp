#include "FlowGraphComponent.h"
#include "FlowAsset.h"
#include "FlowSubsystem.h"

#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"

void UFlowGraphComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(FlowAsset) && (!bAuthorityOnly || GetOwner()->HasAuthority()))
	{
		if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
		{
			FlowSubsystem->StartRootFlow(GetOwner(), FlowAsset);
		}
	}
}

void UFlowGraphComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(FlowAsset) && (!bAuthorityOnly || GetOwner()->HasAuthority()))
	{
		if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
		{
			FlowSubsystem->FinishRootFlow(GetOwner(), FlowAsset);
		}
	}

	Super::EndPlay(EndPlayReason);
}
