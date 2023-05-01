// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_CustomOutput.h"
#include "FlowAsset.h"
#include "FlowSettings.h"

#define LOCTEXT_NAMESPACE "FlowNode"

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
	const bool bUseAdaptiveNodeTitles = UFlowSettings::Get()->bUseAdaptiveNodeTitles;

	if (bUseAdaptiveNodeTitles && !EventName.IsNone())
	{
		return FText::Format(LOCTEXT("CustomOutputTitle", "{0} Output"), { FText::FromString(EventName.ToString()) });
	}
	else
	{
		return Super::GetNodeTitle();
	}
}
#endif

#undef LOCTEXT_NAMESPACE