#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "EditorStyleSet.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/InputChord.h"

class FFlowGraphCommands : public TCommands<FFlowGraphCommands>
{
public:
	/** Constructor */
	FFlowGraphCommands() 
		: TCommands<FFlowGraphCommands>("FlowGraphEditor", NSLOCTEXT("Contexts", "FlowGraphEditor", "Flow Graph Editor"), NAME_None, FEditorStyle::GetStyleSetName())
	{
	}
	
	TSharedPtr<FUICommandInfo> BreakLink;

	TSharedPtr<FUICommandInfo> AddInput;
	TSharedPtr<FUICommandInfo> DeleteInput;

	TSharedPtr<FUICommandInfo> AddOutput;
	TSharedPtr<FUICommandInfo> DeleteOutput;

	TSharedPtr<FUICommandInfo> FocusViewport;

	/** Initialize commands */
	virtual void RegisterCommands() override;
};

// FFlowNodeSpawnInfo
class FFlowNodeSpawnInfo
{
public:
	/** Constructor */
	FFlowNodeSpawnInfo(UClass* InFlowNodeClass) : FlowNodeClass(InFlowNodeClass) {}

	/** Holds the UI Command to verify chords for this action are held */
	TSharedPtr< FUICommandInfo > CommandInfo;

	TSharedPtr<FEdGraphSchemaAction> GetAction(UEdGraph* InDestGraph);
	UClass* GetClass() { return FlowNodeClass; }

private:
	UClass* FlowNodeClass;
};

// FFlowGraphSpawnCommands
class FFlowGraphSpawnCommands : public TCommands<FFlowGraphSpawnCommands>
{
public:
	/** Constructor */
	FFlowGraphSpawnCommands()
		: TCommands<FFlowGraphSpawnCommands>(TEXT("FlowGraphSpawnNodes"), NSLOCTEXT("Contexts", "FlowGraph_SpawnNodes", "Flow Graph - Spawn Nodes"), NAME_None, FEditorStyle::GetStyleSetName())
	{
	}

	/** TCommands interface */
	virtual void RegisterCommands() override;

	TSharedPtr<FEdGraphSchemaAction> GetGraphActionByChord(FInputChord& InChord, UEdGraph* InDestGraph) const;
	const TSharedPtr<const FInputChord> GetChordByClass(UClass* FlowNodeClass) const;
	const TSharedPtr<const FInputChord> GetCommentChord() const;

private:
	TArray< TSharedPtr<class FFlowNodeSpawnInfo>> NodeCommands;
};
