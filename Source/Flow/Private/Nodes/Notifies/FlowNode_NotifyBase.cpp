#include "Nodes/Notifies/FlowNode_NotifyBase.h"
#include "FlowComponent.h"

UFlowNode_NotifyBase::UFlowNode_NotifyBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Notifies");
#endif
}

#if WITH_EDITOR
FString UFlowNode_NotifyBase::GetNodeDescription() const
{
	const FString IdentityString = IdentityTag.IsValid() ? IdentityTag.ToString() : TEXT("Missing Actor Tag!");
	const FString NotifyString = NotifyTag.IsValid() ? NotifyTag.ToString() : TEXT("---");

	return IdentityString + LINE_TERMINATOR + NotifyString;
}
#endif
