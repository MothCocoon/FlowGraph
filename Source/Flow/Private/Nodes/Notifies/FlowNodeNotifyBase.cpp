#include "FlowNodeNotifyBase.h"
#include "FlowComponent.h"

UFlowNodeNotifyBase::UFlowNodeNotifyBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
FString UFlowNodeNotifyBase::GetNodeDescription() const
{
	const FString ActorString = ActorTag.IsValid() ? ActorTag.ToString() : TEXT("Missing Actor Tag!");
	const FString NotifyString = NotifyTag.IsValid() ? NotifyTag.ToString() : TEXT("---");

	return ActorString + LINE_TERMINATOR + NotifyString;
}
#endif