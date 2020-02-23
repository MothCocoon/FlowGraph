#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "EditorStyleSet.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/InputChord.h"

/** Generic graph commands for the flow graph */
class FFlowGraphCommands : public TCommands<FFlowGraphCommands>
{
public:
	FFlowGraphCommands();
	
	TSharedPtr<FUICommandInfo> BreakLink;

	TSharedPtr<FUICommandInfo> AddInput;
	TSharedPtr<FUICommandInfo> AddOutput;
	TSharedPtr<FUICommandInfo> RemovePin;

	TSharedPtr<FUICommandInfo> AddPinBreakpoint;
	TSharedPtr<FUICommandInfo> RemovePinBreakpoint;
	TSharedPtr<FUICommandInfo> EnablePinBreakpoint;
	TSharedPtr<FUICommandInfo> DisablePinBreakpoint;
	TSharedPtr<FUICommandInfo> TogglePinBreakpoint;

	TSharedPtr<FUICommandInfo> FocusViewport;

	virtual void RegisterCommands() override;
};

/** Handles spawning nodes by keyboard shortcut */
class FFlowSpawnNodeCommands : public TCommands<FFlowSpawnNodeCommands>
{
public:
	FFlowSpawnNodeCommands();

	virtual void RegisterCommands() override;

	TSharedPtr<const FInputChord> GetChordByClass(UClass* NodeClass) const;
	TSharedPtr<FEdGraphSchemaAction> GetActionByChord(FInputChord& InChord) const;

private:
	TSharedPtr<FEdGraphSchemaAction> GetActionByClass(UClass* NodeClass) const;

	TMap<UClass*, TSharedPtr<FUICommandInfo>> NodeCommands;
};