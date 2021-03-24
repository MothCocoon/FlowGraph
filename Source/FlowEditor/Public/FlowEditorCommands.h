#pragma once

#include "EdGraph/EdGraphSchema.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandInfo.h"
#include "Templates/SharedPointer.h"

class FFlowToolbarCommands final : public TCommands<FFlowToolbarCommands>
{
public:
	FFlowToolbarCommands();

	TSharedPtr<FUICommandInfo> RefreshAsset;
	TSharedPtr<FUICommandInfo> GoToMasterInstance;

	virtual void RegisterCommands() override;
};

/** Generic graph commands for the flow graph */
class FFlowGraphCommands final : public TCommands<FFlowGraphCommands>
{
public:
	FFlowGraphCommands();

	/** Context Pins */
	TSharedPtr<FUICommandInfo> RefreshContextPins;

	/** Pins */
	TSharedPtr<FUICommandInfo> AddInput;
	TSharedPtr<FUICommandInfo> AddOutput;
	TSharedPtr<FUICommandInfo> RemovePin;

	/** Breakpoints */
	TSharedPtr<FUICommandInfo> AddPinBreakpoint;
	TSharedPtr<FUICommandInfo> RemovePinBreakpoint;
	TSharedPtr<FUICommandInfo> EnablePinBreakpoint;
	TSharedPtr<FUICommandInfo> DisablePinBreakpoint;
	TSharedPtr<FUICommandInfo> TogglePinBreakpoint;

	/** Jumps */
	TSharedPtr<FUICommandInfo> FocusViewport;
	TSharedPtr<FUICommandInfo> JumpToNodeDefinition;

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
