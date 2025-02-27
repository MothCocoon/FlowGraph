// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Engine/DeveloperSettings.h"
#include "FlowGraphEditorSettings.generated.h"

UENUM()
enum class EFlowNodeDoubleClickTarget : uint8
{
	NodeDefinition               UMETA(Tooltip = "Open node class: either blueprint or C++ class"),
	PrimaryAsset                 UMETA(Tooltip = "Open asset defined as primary asset, i.e. Dialogue asset for PlayDialogue node"),
	PrimaryAssetOrNodeDefinition UMETA(Tooltip = "First try opening the asset then if there is none, open the node class") 
};

/**
 *
 */
UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "Flow Graph"))
class FLOWEDITOR_API UFlowGraphEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UFlowGraphEditorSettings();

	static UFlowGraphEditorSettings* Get() { return StaticClass()->GetDefaultObject<UFlowGraphEditorSettings>(); }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	// Double-clicking a Flow Node might open relevant asset/code editor
	UPROPERTY(config, EditAnywhere, Category = "Nodes")
	EFlowNodeDoubleClickTarget NodeDoubleClickTarget;
	
	// Displays information on the graph node, either C++ class name or path to blueprint asset
	UPROPERTY(config, EditAnywhere, Category = "Nodes")
	bool bShowNodeClass;

	// Shows the node description when you play in editor
	UPROPERTY(config, EditAnywhere, Category = "Nodes")
	bool bShowNodeDescriptionWhilePlaying;

	// Pin names will will be displayed in a format that is easier to read, even if PinFriendlyName wasn't set
	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	bool bEnforceFriendlyPinNames;

	// Renders preview of entire graph while hovering over 
	UPROPERTY(config, EditAnywhere, Category = "Nodes")
	bool bShowSubGraphPreview;

	UPROPERTY(config, EditAnywhere, Category = "Nodes", meta = (EditCondition = "bShowSubGraphPreview"))
	bool bShowSubGraphPath;
	
	UPROPERTY(config, EditAnywhere, Category = "Nodes", meta = (EditCondition = "bShowSubGraphPreview"))
	FVector2D SubGraphPreviewSize;

	/** Enable hot reload for native flow nodes?
	 * WARNING: hot reload can easily crash the editor and you can lose progress */
	UPROPERTY(EditAnywhere, Config, Category = "Nodes", AdvancedDisplay)
	bool bHotReloadNativeNodes;

	UPROPERTY(EditAnywhere, config, Category = "Wires")
	bool bHighlightInputWiresOfSelectedNodes;

	UPROPERTY(EditAnywhere, config, Category = "Wires")
	bool bHighlightOutputWiresOfSelectedNodes;

public:
	virtual FName GetCategoryName() const override { return FName("Flow Graph"); }
	virtual FText GetSectionText() const override { return INVTEXT("User Settings"); }
};
