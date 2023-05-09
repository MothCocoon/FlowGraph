// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_CustomEventBase.h"
#include "FlowSettings.h"

UFlowNode_CustomEventBase::UFlowNode_CustomEventBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::InOut;
#endif

	AllowedSignalModes = {EFlowSignalMode::Enabled, EFlowSignalMode::Disabled};
}

void UFlowNode_CustomEventBase::SetEventName(const FName& InEventName)
{
	if (EventName != InEventName)
	{
		EventName = InEventName;

#if WITH_EDITOR
		// Must reconstruct the visual representation if anything that is included in AdaptiveNodeTitles changes
		OnReconstructionRequested.ExecuteIfBound();
#endif // WITH_EDITOR
	}
}

#if WITH_EDITOR

FString UFlowNode_CustomEventBase::GetNodeDescription() const
{
	if (UFlowSettings::Get()->bUseAdaptiveNodeTitles)
	{
		return Super::GetNodeDescription();
	}

	return EventName.ToString();
}

EDataValidationResult UFlowNode_CustomEventBase::ValidateNode()
{
	if (EventName.IsNone())
	{
		ValidationLog.Error<UFlowNode>(TEXT("Event Name is empty!"), this);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}
#endif
