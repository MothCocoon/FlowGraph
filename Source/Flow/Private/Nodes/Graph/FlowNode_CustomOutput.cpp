// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Graph/FlowNode_CustomOutput.h"
#include "FlowAsset.h"
#include "FlowSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_CustomOutput)

#define LOCTEXT_NAMESPACE "FlowNode_CustomOutput"

UFlowNode_CustomOutput::UFlowNode_CustomOutput(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OutputPins.Empty();
}

void UFlowNode_CustomOutput::ExecuteInput(const FName& PinName)
{
	UFlowAsset* FlowAsset = GetFlowAsset();
	check(IsValid(FlowAsset));

	if (EventName.IsNone())
	{
		LogWarning(FString::Printf(TEXT("Attempted to trigger a CustomOutput (Node %s, Asset %s), with no EventName"),
		                           *GetName(),
		                           *FlowAsset->GetPathName()));
	}
	else if (!FlowAsset->TryFindCustomOutputNodeByEventName(EventName))
	{
		const TArray<FName> OutputNames = FlowAsset->GatherCustomOutputNodeEventNames();
		FString CustomOutputsString;

		for (const FName& OutputName : OutputNames)
		{
			if (!CustomOutputsString.IsEmpty())
			{
				CustomOutputsString += TEXT(", ");
			}

			CustomOutputsString += OutputName.ToString();
		}

		LogWarning(FString::Printf(TEXT("Attempted to trigger a CustomOutput (Node %s, Asset %s), with EventName %s, which is not a listed CustomOutput { %s }"),
		                           *GetName(),
		                           *FlowAsset->GetPathName(),
		                           *EventName.ToString(),
		                           *CustomOutputsString));
	}
	else
	{
		FlowAsset->TriggerCustomOutput(EventName);
	}
}

#if WITH_EDITOR
FText UFlowNode_CustomOutput::GetNodeTitle() const
{
	if (!EventName.IsNone() && UFlowSettings::Get()->bUseAdaptiveNodeTitles)
	{
		return FText::Format(LOCTEXT("CustomOutputTitle", "{0} Output"), {FText::FromString(EventName.ToString())});
	}

	return Super::GetNodeTitle();
}
#endif

#undef LOCTEXT_NAMESPACE
