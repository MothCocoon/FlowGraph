// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "EdGraph/EdGraphSchema.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandInfo.h"
#include "Templates/SharedPointer.h"

class FLOWEDITOR_API FFlowToolbarCommands : public TCommands<FFlowToolbarCommands>
{
public:
	FFlowToolbarCommands();

	TSharedPtr<FUICommandInfo> AssetDefaults;
	TSharedPtr<FUICommandInfo> RefreshAsset;
	TSharedPtr<FUICommandInfo> ValidateAsset;
	TSharedPtr<FUICommandInfo> AutoFormatGraph;

	TSharedPtr<FUICommandInfo> SearchInAsset;

	TSharedPtr<FUICommandInfo> ContinueFlowExecution;
	TSharedPtr<FUICommandInfo> StopFlowExecution;

	// Public despite protected base, TCommands::Register calls it through the derived type
	virtual void RegisterCommands() override;
};

/**
 * Generic graph commands for the flow graph.
 */
class FLOWEDITOR_API FFlowGraphCommands : public TCommands<FFlowGraphCommands>
{
public:
	FFlowGraphCommands();

	// Context Pins
	TSharedPtr<FUICommandInfo> ReconstructNode;

	// Pins
	TSharedPtr<FUICommandInfo> AddInput;
	TSharedPtr<FUICommandInfo> AddOutput;
	TSharedPtr<FUICommandInfo> RemovePin;

	// Organization
	TSharedPtr<FUICommandInfo> ExtractToSubGraph;

	// Pin Breakpoints
	TSharedPtr<FUICommandInfo> AddPinBreakpoint;
	TSharedPtr<FUICommandInfo> RemovePinBreakpoint;
	TSharedPtr<FUICommandInfo> EnablePinBreakpoint;
	TSharedPtr<FUICommandInfo> DisablePinBreakpoint;
	TSharedPtr<FUICommandInfo> TogglePinBreakpoint;

	// Breakpoints
	TSharedPtr<FUICommandInfo> EnableAllBreakpoints;
	TSharedPtr<FUICommandInfo> DisableAllBreakpoints;
	TSharedPtr<FUICommandInfo> RemoveAllBreakpoints;

	// Execution Override
	TSharedPtr<FUICommandInfo> EnableNode;
	TSharedPtr<FUICommandInfo> DisableNode;
	TSharedPtr<FUICommandInfo> SetPassThrough;
	TSharedPtr<FUICommandInfo> ForcePinActivation;

	// Jumps
	TSharedPtr<FUICommandInfo> FocusViewport;
	TSharedPtr<FUICommandInfo> JumpToNodeDefinition;

	// Public despite protected base, TCommands::Register calls it through the derived type
	virtual void RegisterCommands() override;
};

/**
 * Handles spawning nodes by keyboard shortcut.
 */
class FLOWEDITOR_API FFlowSpawnNodeCommands : public TCommands<FFlowSpawnNodeCommands>
{
public:
	FFlowSpawnNodeCommands();

	// Public despite protected base, TCommands::Register calls it through the derived type
	virtual void RegisterCommands() override;

public:
	TSharedPtr<const FInputChord> GetChordByClass(const UClass* NodeClass) const;
	TSharedPtr<FEdGraphSchemaAction> GetActionByChord(const FInputChord& InChord) const;

private:
	TSharedPtr<FEdGraphSchemaAction> GetActionByClass(UClass* NodeClass) const;

	TMap<UClass*, TSharedPtr<FUICommandInfo>> NodeCommands;
};
