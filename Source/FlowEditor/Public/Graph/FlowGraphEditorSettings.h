// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Engine/DeveloperSettings.h"
#include "Find/FindInFlowEnums.h"

#include "FlowGraphEditorSettings.generated.h"

UENUM()
enum class EFlowNodeDoubleClickTarget : uint8
{
	NodeDefinition				 UMETA(Tooltip = "Open node class: either blueprint or C++ class"),
	PrimaryAsset				 UMETA(Tooltip = "Open asset defined as primary asset, i.e. Dialogue asset for PlayDialogue node"),
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

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/* Double-clicking a Flow Node might open relevant asset/code editor. */
	UPROPERTY(config, EditAnywhere, Category = "Nodes")
	EFlowNodeDoubleClickTarget NodeDoubleClickTarget;

	/* Displays information on the graph node, either C++ class name or path to blueprint asset. */
	UPROPERTY(config, EditAnywhere, Category = "Nodes")
	bool bShowNodeClass;

	/* Shows the node description when you play in editor. */
	UPROPERTY(config, EditAnywhere, Category = "Nodes")
	bool bShowNodeDescriptionWhilePlaying;

	/* Display descriptions from attached addons in node descriptions. */
	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	bool bShowAddonDescriptions;

	/* Pin names will be displayed in a format that is easier to read, even if PinFriendlyName wasn't set. */
	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	bool bEnforceFriendlyPinNames;

	/* Renders preview of entire graph while hovering over. */
	UPROPERTY(config, EditAnywhere, Category = "Nodes")
	bool bShowSubGraphPreview;

	UPROPERTY(config, EditAnywhere, Category = "Nodes", meta = (EditCondition = "bShowSubGraphPreview"))
	bool bShowSubGraphPath;

	UPROPERTY(config, EditAnywhere, Category = "Nodes", meta = (EditCondition = "bShowSubGraphPreview"))
	FVector2D SubGraphPreviewSize;

	/** Clicking action button in FlowIdentity menu will close it */
	UPROPERTY(EditAnywhere, config, Category = "Identity")
	bool bFlowIdentity_CloseOnAction = true;

	/** FlowIdentity menu will display short tag names */
	UPROPERTY(EditAnywhere, config, Category = "Identity")
	bool bFlowIdentity_DisplayShortNames = true;

	/** FlowIdentity menu tag selection windows will be at least this size. Use in projects with long tag names */
	UPROPERTY(EditAnywhere, config, Category = "Identity")
	int32 FlowIdentity_MinWindowWidth = 400;

	/* Default search filter flags for the Flow Editor. */
	UPROPERTY(VisibleAnywhere, config, Category = "Search", meta = (Bitmask, BitmaskEnum = "/Script/Flow.EFlowSearchFlags"))
	uint32 DefaultSearchFlags = static_cast<uint32>(EFlowSearchFlags::DefaultSearchFlags);

	UPROPERTY(VisibleAnywhere, config, Category = "Search")
	uint8 DefaultSearchPinDirection = 0;

	UPROPERTY(VisibleAnywhere, config, Category = "Search")
	uint8 DefaultSearchPinConnection = 0;

	/* Max search depth for inline objects in the Flow Editor. */
	UPROPERTY(EditAnywhere, config, Category = "Search", meta = (ClampMin = 1))
	int32 DefaultMaxSearchDepth = 1;

	UPROPERTY(EditAnywhere, config, Category = "Wires")
	bool bHighlightInputWiresOfSelectedNodes;

	UPROPERTY(EditAnywhere, config, Category = "Wires")
	bool bHighlightOutputWiresOfSelectedNodes;

public:
	virtual FName GetCategoryName() const override { return FName("Flow Graph"); }
	virtual FText GetSectionText() const override { return INVTEXT("User Settings"); }
};
