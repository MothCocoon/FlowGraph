// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Operators/FlowNode_LogicalOR.h"

static FName ResetName = TEXT("Reset");

UFlowNode_LogicalOR::UFlowNode_LogicalOR(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Operators");
	NodeStyle = EFlowNodeStyle::Logic;
#endif

	SetNumberedInputPins(0, 1);
}

void UFlowNode_LogicalOR::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UFlowNode_LogicalOR, bOnce))
	{
		if (bOnce)
		{
			InputPins.EmplaceAt(0, ResetName);
			OutputPins.Emplace(ResetName);
		}
		else
		{
			InputPins.Remove(ResetName);
			OutputPins.Remove(ResetName);
		}

		OnReconstructionRequested.ExecuteIfBound();
	}
}

void UFlowNode_LogicalOR::ExecuteInput(const FName& PinName)
{
	if (PinName == ResetName)
	{
		bCompleted = false;
		TriggerOutput(ResetName, false);
		return;
	}

	if (bOnce && bCompleted)
	{
		return;
	}

	bCompleted = true;

	TriggerFirstOutput(true);
}
