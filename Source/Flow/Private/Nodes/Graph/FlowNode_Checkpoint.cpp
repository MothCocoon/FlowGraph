// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Graph/FlowNode_Checkpoint.h"
#include "FlowSubsystem.h"

#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_Checkpoint)

UFlowNode_Checkpoint::UFlowNode_Checkpoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Graph");
#endif
}

void UFlowNode_Checkpoint::ExecuteInput(const FName& PinName)
{
	if (GetFlowSubsystem())
	{
		UFlowSaveGame* NewSaveGame = Cast<UFlowSaveGame>(UGameplayStatics::CreateSaveGameObject(UFlowSaveGame::StaticClass()));
		GetFlowSubsystem()->OnGameSaved(NewSaveGame);

		UGameplayStatics::SaveGameToSlot(NewSaveGame, NewSaveGame->SaveSlotName, 0);
	}

	TriggerFirstOutput(true);
}

void UFlowNode_Checkpoint::OnLoad_Implementation()
{
	TriggerFirstOutput(true);
}
