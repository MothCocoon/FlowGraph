// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Nodes/Graph/FlowNode_Checkpoint.h"

#include "FlowSubsystem.h"

#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_Checkpoint)

UFlowNode_Checkpoint::UFlowNode_Checkpoint()
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

		if (bUseAsyncSave)
		{
			UGameplayStatics::AsyncSaveGameToSlot(NewSaveGame, NewSaveGame->SaveSlotName, 0);
		}
		else
		{
			UGameplayStatics::SaveGameToSlot(NewSaveGame, NewSaveGame->SaveSlotName, 0);
		}
	}

	TriggerFirstOutput(true);
}

void UFlowNode_Checkpoint::OnLoad_Implementation()
{
	TriggerFirstOutput(true);
}

#if WITH_EDITOR
const FFlowAgentDoc& UFlowNode_Checkpoint::GetAgentDoc() const
{
	static const FFlowAgentDoc Doc = MakeAgentDoc(
		/*Guidance*/ TEXT("The class comment recommends replacing this with a game-specific save node and hiding this one from the palette via UFlowGraphSettings::NodesHiddenFromPalette, rather than using it as-is in shipping content."),
		/*Tags*/     { TEXT("graph"), TEXT("save"), TEXT("checkpoint") },
		/*Articles*/ {  });
	return Doc;
}
#endif
