// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_CustomNodeBase.h"
#include "FlowSettings.h"

UFlowNode_CustomNodeBase::UFlowNode_CustomNodeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::InOut;
#endif

	AllowedSignalModes = {EFlowSignalMode::Enabled, EFlowSignalMode::Disabled};
}

void UFlowNode_CustomNodeBase::SetEventName(const FName& InEventName)
{
	if (EventName != InEventName)
	{
		EventName = InEventName;

#if WITH_EDITOR
		// Must reconstruct the Graph Visuals if anything that is included in AdaptiveNodeTitles changes
		OnReconstructionRequested.ExecuteIfBound();
#endif // WITH_EDITOR
	}
}

#if WITH_EDITOR

FString UFlowNode_CustomNodeBase::GetNodeDescription() const
{
	const bool bUseAdaptiveNodeTitles = UFlowSettings::Get()->bUseAdaptiveNodeTitles;

	if (bUseAdaptiveNodeTitles)
	{
		return Super::GetNodeDescription();
	}
	else
	{
		return EventName.ToString();
	}
}

EDataValidationResult UFlowNode_CustomNodeBase::ValidateNode()
{
	if (EventName.IsNone())
	{
		ValidationLog.Error<UFlowNode>(TEXT("Event Name is empty!"), this);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}
#endif
