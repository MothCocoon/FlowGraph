#include "Nodes/World/FlowNode_NotifyActor.h"
#include "FlowComponent.h"
#include "FlowSubsystem.h"

#include "Engine/World.h"

UFlowNode_NotifyActor::UFlowNode_NotifyActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Notifies");
#endif
}

void UFlowNode_NotifyActor::ExecuteInput(const FName& PinName)
{
	if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
	{
		for (TWeakObjectPtr<UFlowComponent>& Component : FlowSubsystem->GetComponents<UFlowComponent>(IdentityTag))
		{
			Component->NotifyFromGraph(NotifyTag);
		}
	}

	TriggerFirstOutput(true);
}

#if WITH_EDITOR
FString UFlowNode_NotifyActor::GetNodeDescription() const
{
	const FString IdentityString = IdentityTag.IsValid() ? IdentityTag.ToString() : MissingIdentityTag;
	const FString NotifyString = NotifyTag.IsValid() ? NotifyTag.ToString() : TEXT("---");

	return IdentityString + LINE_TERMINATOR + NotifyString;
}
#endif
