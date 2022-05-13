// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Utils/FlowNode_Checkpoint.h"
#include "FlowSubsystem.h"

#include "Kismet/GameplayStatics.h"

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
