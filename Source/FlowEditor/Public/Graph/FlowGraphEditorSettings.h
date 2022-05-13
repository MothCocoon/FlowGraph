// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowGraphConnectionDrawingPolicy.h"
#include "Engine/DeveloperSettings.h"

#include "FlowTypes.h"
#include "FlowGraphEditorSettings.generated.h"

/**
 *
 */
UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "Flow Graph"))
class UFlowGraphEditorSettings final : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()
	static UFlowGraphEditorSettings* Get() { return StaticClass()->GetDefaultObject<UFlowGraphEditorSettings>(); }

	// Displays information on the graph node, either C++ class name or path to blueprint asset
	UPROPERTY(config, EditAnywhere, Category = "Nodes")
	bool bShowNodeClass;

	// Renders preview of entire graph while hovering over 
	UPROPERTY(config, EditAnywhere, Category = "Nodes")
	bool bShowSubGraphPreview;

	UPROPERTY(config, EditAnywhere, Category = "Nodes", meta = (EditCondition = "bShowSubGraphPreview"))
	bool bShowSubGraphPath;
	
	UPROPERTY(config, EditAnywhere, Category = "Nodes", meta = (EditCondition = "bShowSubGraphPreview"))
	FVector2D SubGraphPreviewSize;

	UPROPERTY(EditAnywhere, config, Category = "Wires")
	bool bHighlightInputWiresOfSelectedNodes;

	UPROPERTY(EditAnywhere, config, Category = "Wires")
	bool bHighlightOutputWiresOfSelectedNodes;
};
