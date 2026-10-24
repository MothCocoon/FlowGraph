// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Delegates/DelegateCombinations.h"
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
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/* Double-clicking a Flow Node might open relevant asset/code editor. */
	UPROPERTY(config, EditAnywhere, Category = "Nodes")
	EFlowNodeDoubleClickTarget NodeDoubleClickTarget = EFlowNodeDoubleClickTarget::PrimaryAssetOrNodeDefinition;

	/* Displays information on the graph node, either C++ class name or path to blueprint asset. */
	UPROPERTY(config, EditAnywhere, Category = "Nodes")
	bool bShowNodeClass = false;

	/* Shows the node description when you play in editor. */
	UPROPERTY(config, EditAnywhere, Category = "Nodes")
	bool bShowNodeDescriptionWhilePlaying = true;

	/* Display descriptions from attached addons in node descriptions. */
	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	bool bShowAddonDescriptions = true;

	/* Shows the editable AddOn hierarchy in the owning Flow Node's (or AddOn's) Details panel. */
	UPROPERTY(EditAnywhere, config, Category = "Nodes", meta = (DisplayName = "Show AddOns in Node Details"))
	bool bShowAddOnsInNodeDetails = false;

	/* Pin names will be displayed in a format that is easier to read, even if PinFriendlyName wasn't set. */
	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	bool bEnforceFriendlyPinNames = false;

	/* Renders preview of entire graph while hovering over. */
	UPROPERTY(config, EditAnywhere, Category = "Nodes")
	bool bShowSubGraphPreview = true;

	UPROPERTY(config, EditAnywhere, Category = "Nodes", meta = (EditCondition = "bShowSubGraphPreview"))
	bool bShowSubGraphPath = true;

	UPROPERTY(config, EditAnywhere, Category = "Nodes", meta = (EditCondition = "bShowSubGraphPreview"))
	FVector2D SubGraphPreviewSize = FVector2D(640.f, 360.f);

	/** Clicking action button in FlowIdentity menu will close it */
	UPROPERTY(EditAnywhere, config, Category = "Identity")
	bool bFlowIdentity_CloseOnAction = true;

	/** FlowIdentity menu will display short tag names */
	UPROPERTY(EditAnywhere, config, Category = "Identity")
	bool bFlowIdentity_DisplayShortNames = true;

	/** FlowIdentity menu tag selection windows will be at least this size. Use in projects with long tag names */
	UPROPERTY(EditAnywhere, config, Category = "Identity")
	int32 FlowIdentity_MinWindowWidth = 400;

	/* Max search depth for inline objects in the Flow Editor. */
	UPROPERTY(EditAnywhere, config, Category = "Search", meta = (ClampMin = 1))
	int32 DefaultMaxSearchDepth = 1;
	
	/* Search filter flags excluded from search in the Flow Editor. */
	UPROPERTY(VisibleAnywhere, config, Category = "Search", meta = (Bitmask, BitmaskEnum = "/Script/FlowEditor.EFlowSearchFlags"))
	uint32 DisabledSearchFlags = 0;

	UPROPERTY(VisibleAnywhere, config, Category = "Search")
	EFlowSearchPinDirection DefaultSearchPinDirection = EFlowSearchPinDirection::Any;

	UPROPERTY(VisibleAnywhere, config, Category = "Search")
	EFlowSearchPinConnectionState DefaultSearchPinConnection = EFlowSearchPinConnectionState::Any;

	UPROPERTY(EditAnywhere, config, Category = "Wires")
	bool bHighlightInputWiresOfSelectedNodes = false;

	UPROPERTY(EditAnywhere, config, Category = "Wires")
	bool bHighlightOutputWiresOfSelectedNodes = false;

public:
	virtual FName GetCategoryName() const override { return FName("Flow Graph"); }
	virtual FText GetSectionText() const override { return INVTEXT("User Settings"); }

	DECLARE_MULTICAST_DELEGATE(FOnSettingsChanged);
	static FOnSettingsChanged& OnSettingsChanged();

private:
	static FOnSettingsChanged SettingsChanged;
};
