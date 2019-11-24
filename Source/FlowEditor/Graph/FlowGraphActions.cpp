#include "FlowGraphActions.h"
#include "FlowGraphSchema.h"

#include "Flow/Graph/Nodes/FlowNode.h"

#include "Misc/ConfigCacheIni.h"

#define LOCTEXT_NAMESPACE "FlowGraphActions"

void FFlowGraphCommands::RegisterCommands()
{
	UI_COMMAND(AddInput, "Add Input", "Adds an input to the node", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(DeleteInput, "Delete Input", "Removes an input from the node", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(AddOutput, "Add Output", "Adds an output to the node", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(DeleteOutput, "Delete Output", "Removes an output from the node", EUserInterfaceActionType::Button, FInputChord());
	
	UI_COMMAND(FocusViewport, "Focus Viewport", "Focus viewport on actor assigned to the node", EUserInterfaceActionType::Button, FInputChord());
}

//////////////////////////////////////////////////////////////////////////
// FFlowNodeSpawnInfo

TSharedPtr<FEdGraphSchemaAction> FFlowNodeSpawnInfo::GetAction(UEdGraph* InDestGraph)
{
	if (FlowNodeClass == UFlowNode::StaticClass())
	{
		return MakeShareable(new FFlowGraphSchemaAction_NewComment);
	}
	else
	{
		TSharedPtr<FFlowGraphSchemaAction_NewNode> NewNodeAction(new FFlowGraphSchemaAction_NewNode);
		NewNodeAction->FlowNodeClass = FlowNodeClass;
		return NewNodeAction;
	}
}

//////////////////////////////////////////////////////////////////////////
// FFlowGraphEditorSpawnCommands

void FFlowGraphSpawnCommands::RegisterCommands()
{
	const FString ConfigSection = TEXT("FlowGraphSpawnNodes");
	const FString SettingName = TEXT("Node");
	TArray< FString > NodeSpawns;
	GConfig->GetArray(*ConfigSection, *SettingName, NodeSpawns, GEditorPerProjectIni);

	for (int32 x = 0; x < NodeSpawns.Num(); ++x)
	{
		FString CommandLabel;
		TSharedPtr< FFlowNodeSpawnInfo > InfoPtr;

		FString ClassName;
		if (FParse::Value(*NodeSpawns[x], TEXT("Class="), ClassName))
		{
			UClass* FoundClass = FindObject<UClass>(ANY_PACKAGE, *ClassName, true);
			if (FoundClass && FoundClass->IsChildOf(UFlowNode::StaticClass()))
			{
				CommandLabel = FoundClass->GetName();
				InfoPtr = MakeShareable(new FFlowNodeSpawnInfo(FoundClass));
			}
		}
		else
		{
			FString CustomName;
			if (FParse::Value(*NodeSpawns[x], TEXT("Name="), CustomName))
			{
				CommandLabel = TEXT("Comment");
				InfoPtr = MakeShareable(new FFlowNodeSpawnInfo(UFlowNode::StaticClass()));
			}
			else
			{
				// Matching node not found
				continue;
			}
		}
			
		

		// If spawn info was created, setup a UI Command for keybinding.
		if (InfoPtr.IsValid())
		{
			TSharedPtr< FUICommandInfo > CommandInfo;

			FKey Key;
			bool bShift = false;
			bool bCtrl = false;
			bool bAlt = false;

			// Parse the keybinding information
			FString KeyString;
			if (FParse::Value(*NodeSpawns[x], TEXT("Key="), KeyString))
			{
				Key = *KeyString;
			}

			if (Key.IsValid())
			{
				FParse::Bool(*NodeSpawns[x], TEXT("Shift="), bShift);
				FParse::Bool(*NodeSpawns[x], TEXT("Alt="), bAlt);
				FParse::Bool(*NodeSpawns[x], TEXT("Ctrl="), bCtrl);
			}

			FInputChord Chord(Key, EModifierKey::FromBools(bCtrl, bAlt, bShift, false));

			const FText CommandLabelText = FText::FromString(CommandLabel);
			const FText Description = FText::Format(LOCTEXT("NodeSpawnDescription", "Hold down the bound keys and left click in the graph panel to spawn a {0} node."), CommandLabelText);

			FUICommandInfo::MakeCommandInfo(this->AsShared(), CommandInfo, FName(*NodeSpawns[x]), CommandLabelText, Description, FSlateIcon(FEditorStyle::GetStyleSetName(), *FString::Printf(TEXT("%s.%s"), *this->GetContextName().ToString(), *NodeSpawns[x])), EUserInterfaceActionType::Button, Chord);

			InfoPtr->CommandInfo = CommandInfo;

			NodeCommands.Add(InfoPtr);
		}
	}
}

TSharedPtr< FEdGraphSchemaAction > FFlowGraphSpawnCommands::GetGraphActionByChord(FInputChord& InChord, UEdGraph* InDestGraph) const
{
	if (InChord.IsValidChord())
	{
		for (int32 x = 0; x < NodeCommands.Num(); ++x)
		{
			if (NodeCommands[x]->CommandInfo->HasActiveChord(InChord))
			{
				return NodeCommands[x]->GetAction(InDestGraph);
			}
		}
	}

	return TSharedPtr< FEdGraphSchemaAction >();
}

const TSharedPtr<const FInputChord> FFlowGraphSpawnCommands::GetChordByClass(UClass* FlowNodeClass) const
{
	for (int32 Index = 0; Index < NodeCommands.Num(); ++Index)
	{
		if (NodeCommands[Index]->GetClass() == FlowNodeClass && NodeCommands[Index]->CommandInfo->GetFirstValidChord()->IsValidChord())
		{
			// Just return the first valid chord
			return NodeCommands[Index]->CommandInfo->GetFirstValidChord();
		}
	}

	return TSharedPtr<const FInputChord>();
}

const TSharedPtr<const FInputChord> FFlowGraphSpawnCommands::GetCommentChord() const
{
	for (int32 Index = 0; Index < NodeCommands.Num(); ++Index)
	{
		if (NodeCommands[Index]->GetClass() == UFlowNode::StaticClass() && NodeCommands[Index]->CommandInfo->GetFirstValidChord()->IsValidChord())
		{
			// Just return the first valid chord
			return NodeCommands[Index]->CommandInfo->GetFirstValidChord();
		}
	}

	return TSharedPtr<const FInputChord>();
}

#undef LOCTEXT_NAMESPACE
