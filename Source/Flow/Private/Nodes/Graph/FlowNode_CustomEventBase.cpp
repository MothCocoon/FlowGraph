// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Graph/FlowNode_CustomEventBase.h"
#include "FlowSettings.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_CustomEventBase)

UFlowNode_CustomEventBase::UFlowNode_CustomEventBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Graph");
	NodeDisplayStyle = FlowNodeStyle::InOut;
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
#endif
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

EDataValidationResult UFlowNode_CustomEventBase::ValidateNode(FDataValidationContext& Context) const
{
	const EDataValidationResult SuperResult = Super::ValidateNode(Context);

	EDataValidationResult FinalResult = CombineDataValidationResults(SuperResult, EDataValidationResult::Valid);

	if (EventName.IsNone())
	{
		Context.AddError(FText::FromString(TEXT("Event Name is empty!")));

		FinalResult = CombineDataValidationResults(FinalResult, EDataValidationResult::Invalid);
	}

	return FinalResult;
}
#endif
