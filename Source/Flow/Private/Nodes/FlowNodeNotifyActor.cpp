#include "FlowNodeNotifyActor.h"
#include "FlowComponent.h"
#include "FlowSubsystem.h"

#include "Engine/World.h"

UFlowNodeNotifyActor::UFlowNodeNotifyActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFlowNodeNotifyActor::ExecuteInput(const FName& PinName)
{
	if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
	{
		for (TWeakObjectPtr<UFlowComponent>& Component : FlowSubsystem->GetComponents<UFlowComponent>(ActorTag))
		{
			Component->NotifyFromGraph.Broadcast(NotifyTag);
		}
	}

	TriggerFirstOutput(true);
}

#if WITH_EDITOR
FString UFlowNodeNotifyActor::GetNodeDescription() const
{
	return ActorTag.IsValid() ? ActorTag.ToString() : FString();
}
#endif