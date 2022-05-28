// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Engine/DeveloperSettings.h"
#include "FlowGraphEditorSettings.generated.h"

UENUM()
enum class EFlowNodeDoubleClickTarget : uint8
{
	NodeDefinition UMETA(Tooltip = "Open node class: either blueprint or C++ class"),
	PrimaryAsset   UMETA(Tooltip = "Open asset defined as primary asset, i.e. Dialogue asset for PlayDialogue node")
};

/**
 *
 */
UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "Flow Graph"))
class UFlowGraphEditorSettings final : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()

	static UFlowGraphEditorSettings* Get() { return StaticClass()->GetDefaultObject<UFlowGraphEditorSettings>(); }

	// Double-clicking a Flow Node might open relevant asset/code editor
	UPROPERTY(config, EditAnywhere, Category = "Nodes")
	EFlowNodeDoubleClickTarget NodeDoubleClickTarget;
	
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
