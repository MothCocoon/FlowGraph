// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_CustomOutput.h"
#include "FlowAsset.h"
#include "FlowSettings.h"

#define LOCTEXT_NAMESPACE "FlowNode_CustomOutput"

UFlowNode_CustomOutput::UFlowNode_CustomOutput(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OutputPins.Empty();
}

void UFlowNode_CustomOutput::ExecuteInput(const FName& PinName)
{
	if (!EventName.IsNone() && GetFlowAsset()->GetCustomOutputs().Contains(EventName) && GetFlowAsset()->GetNodeOwningThisAssetInstance())
	{
		GetFlowAsset()->TriggerCustomOutput(EventName);
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
