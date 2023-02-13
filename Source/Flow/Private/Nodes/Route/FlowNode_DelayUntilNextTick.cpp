// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_DelayUntilNextTick.h"

UFlowNode_DelayUntilNextTick::UFlowNode_DelayUntilNextTick()
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::Latent;
#endif
	
	OutputPins.Empty();
	OutputPins.Add(FFlowPin(TEXT("Completed"), "Executed in next frame."));
	OutputPins.Add(FFlowPin(TEXT("Skipped"), "Executed if GetWorld is null and this pin will run in same frame."));
}

void UFlowNode_DelayUntilNextTick::ExecuteInput(const FName& PinName)
{
	const UWorld* World = GetWorld();
	if (IsValid(World))
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::OnCompletion));
	}
	else
	{
		LogError(TEXT("No valid world for delay until next tick node"));
		TriggerOutput(TEXT("Skipped"), true);
	}
}

void UFlowNode_DelayUntilNextTick::OnCompletion()
{
	TriggerOutput(TEXT("Completed"), true);
}
