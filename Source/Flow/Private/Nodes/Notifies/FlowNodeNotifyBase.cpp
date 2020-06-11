#include "FlowNodeNotifyBase.h"
#include "FlowComponent.h"

UFlowNodeNotifyBase::UFlowNodeNotifyBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
FString UFlowNodeNotifyBase::GetNodeDescription() const
{
	const FString IdentityString = IdentityTag.IsValid() ? IdentityTag.ToString() : TEXT("Missing Actor Tag!");
	const FString NotifyString = NotifyTag.IsValid() ? NotifyTag.ToString() : TEXT("---");

	return IdentityString + LINE_TERMINATOR + NotifyString;
}
#endif