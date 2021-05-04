#include "Nodes/Utils/FlowNode_Checkpoint.h"
#include "FlowSubsystem.h"

UFlowNode_Checkpoint::UFlowNode_Checkpoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Utils");
#endif
}

void UFlowNode_Checkpoint::ExecuteInput(const FName& PinName)
{
	if (GetFlowSubsystem())
	{
		GetFlowSubsystem()->SaveGame();
	}
}
