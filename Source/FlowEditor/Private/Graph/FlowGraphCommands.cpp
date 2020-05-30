#include "FlowGraphCommands.h"
#include "FlowGraphSchema.h"

#include "Nodes/FlowNode.h"

#include "Misc/ConfigCacheIni.h"

#define LOCTEXT_NAMESPACE "FlowGraphCommands"

FFlowGraphCommands::FFlowGraphCommands()
	: TCommands<FFlowGraphCommands>("FlowGraph", LOCTEXT("FlowGraph", "Flow Graph"), NAME_None, FEditorStyle::GetStyleSetName())
{
}

void FFlowGraphCommands::RegisterCommands()
{
	UI_COMMAND(RefreshContextInputs, "Refresh context inputs", "Refresh inputs generated from the context asset", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(RefreshContextOutputs, "Refresh context outputs", "Refresh outputs generated from the context asset", EUserInterfaceActionType::Button, FInputChord());
	
	UI_COMMAND(AddInput, "Add Input", "Adds an input to the node", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(AddOutput, "Add Output", "Adds an output to the node", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(RemovePin, "Remove Pin", "Removes a pin from the node", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(AddPinBreakpoint, "Add Pin Breakpoint", "Adds a breakpoint to the pin", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(RemovePinBreakpoint, "Remove Pin Breakpoint", "Removes a breakpoint from the pin", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(EnablePinBreakpoint, "Enable Pin Breakpoint", "Enables a breakpoint on the pin", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(DisablePinBreakpoint, "Disable Pin Breakpoint", "Disables a breakpoint on the pin", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(TogglePinBreakpoint, "Toggle Pin Breakpoint", "Toggles a breakpoint on the pin", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(FocusViewport, "Focus Viewport", "Focus viewport on actor assigned to the node", EUserInterfaceActionType::Button, FInputChord());
}

FFlowSpawnNodeCommands::FFlowSpawnNodeCommands()
	: TCommands<FFlowSpawnNodeCommands>(TEXT("FFlowSpawnNodeCommands"), LOCTEXT("FlowGraph_SpawnNodes", "Flow Graph - Spawn Nodes"), NAME_None, FEditorStyle::GetStyleSetName())
{
}

void FFlowSpawnNodeCommands::RegisterCommands()
{
	const FString ConfigSection = TEXT("FlowSpawnNodes");
	const FString SettingName = TEXT("Node");
	TArray< FString > NodeSpawns;
	GConfig->GetArray(*ConfigSection, *SettingName, NodeSpawns, GEditorPerProjectIni);

	for (int32 x = 0; x < NodeSpawns.Num(); ++x)
	{
		UClass* NodeClass = nullptr;

		FString ClassName;
		if (FParse::Value(*NodeSpawns[x], TEXT("Class="), ClassName))
		{
			UClass* FoundClass = FindObject<UClass>(ANY_PACKAGE, *ClassName, true);
			if (FoundClass && FoundClass->IsChildOf(UFlowNode::StaticClass()))
			{
				NodeClass = FoundClass;
			}
		}
		else
		{
			FString CustomName;
			if (FParse::Value(*NodeSpawns[x], TEXT("Name="), CustomName))
			{
				NodeClass = UFlowNode::StaticClass();
			}
			else
			{
				// Matching node not found
				continue;
			}
		}

		// Setup a UI Command for key-binding
		TSharedPtr< FUICommandInfo > CommandInfo;

		FKey Key;
		bool bShift = false;
		bool bCtrl = false;
		bool bAlt = false;

		// Parse the key-binding information
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

		const FText CommandLabelText = FText::FromString(NodeClass->GetName());
		const FText Description = FText::Format(LOCTEXT("NodeSpawnDescription", "Hold down the bound keys and left click in the graph panel to spawn a {0} node."), CommandLabelText);

		FUICommandInfo::MakeCommandInfo(this->AsShared(), CommandInfo, FName(*NodeSpawns[x]), CommandLabelText, Description, FSlateIcon(FEditorStyle::GetStyleSetName(), *FString::Printf(TEXT("%s.%s"), *this->GetContextName().ToString(), *NodeSpawns[x])), EUserInterfaceActionType::Button, Chord);

		NodeCommands.Add(NodeClass, CommandInfo);
	}
}

TSharedPtr<const FInputChord> FFlowSpawnNodeCommands::GetChordByClass(UClass* NodeClass) const
{
	if (NodeCommands.Contains(NodeClass) && NodeCommands[NodeClass]->GetFirstValidChord()->IsValidChord())
	{
		return NodeCommands[NodeClass]->GetFirstValidChord();
	}

	return nullptr;
}

TSharedPtr<FEdGraphSchemaAction> FFlowSpawnNodeCommands::GetActionByChord(FInputChord& InChord) const
{
	if (InChord.IsValidChord())
	{
		for (const TPair<UClass*, TSharedPtr<FUICommandInfo>>& NodeCommand : NodeCommands)
		{
			if (NodeCommand.Value.Get()->HasActiveChord(InChord))
			{
				return GetActionByClass(NodeCommand.Key);
			}
		}
	}

	return nullptr;
}

TSharedPtr<FEdGraphSchemaAction> FFlowSpawnNodeCommands::GetActionByClass(UClass* NodeClass) const
{
	if (NodeClass == UFlowNode::StaticClass())
	{
		return MakeShareable(new FFlowGraphSchemaAction_NewComment);
	}
	else
	{
		TSharedPtr<FFlowGraphSchemaAction_NewNode> NewNodeAction(new FFlowGraphSchemaAction_NewNode);
		NewNodeAction->NodeClass = NodeClass;
		return NewNodeAction;
	}
}

#undef LOCTEXT_NAMESPACE