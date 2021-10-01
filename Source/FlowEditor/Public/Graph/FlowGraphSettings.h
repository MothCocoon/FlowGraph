#pragma once

#include "FlowGraphConnectionDrawingPolicy.h"
#include "Engine/DeveloperSettings.h"

#include "FlowTypes.h"
#include "FlowGraphSettings.generated.h"

/**
 *
 */
UCLASS(Config = Editor, defaultconfig, meta = (DisplayName = "Flow Graph"))
class UFlowGraphSettings final : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()
	static UFlowGraphSettings* Get() { return CastChecked<UFlowGraphSettings>(UFlowGraphSettings::StaticClass()->GetDefaultObject()); }

	UPROPERTY(config, EditAnywhere, Category = "Flow Graph")
	EFlowConnectionDrawType ConnectionDrawType;

	UPROPERTY(config, EditAnywhere, Category = "Flow Graph", meta = (EditCondition = "ConnectionDrawType == EFlowConnectionDrawType::Circuit"))
	float ConnectionAngle;

	UPROPERTY(config, EditAnywhere, Category = "Flow Graph", meta = (EditCondition = "ConnectionDrawType == EFlowConnectionDrawType::Circuit"))
	FVector2D ConnectionSpacing;

	UPROPERTY(config, EditAnywhere, Category = "Flow Graph")
	bool bShowGraphPreview;

	UPROPERTY(config, EditAnywhere, Category = "Flow Graph", meta = (EditCondition = "bShowGraphPreview"))
	bool bShowGraphPathInPreview;
	
	UPROPERTY(config, EditAnywhere, Category = "Flow Graph", meta = (EditCondition = "bShowGraphPreview"))
	FVector2D GraphPreviewSize;

	/** Show Flow Asset in Flow category of "Create Asset" menu?
	* Requires restart after making a change. */
	UPROPERTY(EditAnywhere, config, Category = "Default UI")
	bool bExposeFlowAssetCreation;

	/** Show Flow Node blueprint in Flow category of "Create Asset" menu?
	* Requires restart after making a change. */
	UPROPERTY(EditAnywhere, config, Category = "Default UI")
	bool bExposeFlowNodeCreation;
	
	/** Show Flow Asset toolbar?
	* Requires restart after making a change. */
	UPROPERTY(EditAnywhere, config, Category = "Default UI")
	bool bShowAssetToolbarAboveLevelEditor;
	
	/** Hide specific nodes from the Flow Palette without changing the source code.
	* Requires restart after making a change. */
	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	TArray<TSubclassOf<class UFlowNode>> NodesHiddenFromPalette;

	/** Hide default pin names on simple nodes, reduces UI clutter */
	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	bool bShowDefaultPinNames;

	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	TMap<EFlowNodeStyle, FLinearColor> NodeTitleColors;

	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	FLinearColor ExecPinColorModifier;

	UPROPERTY(EditAnywhere, config, Category = "NodePopups")
	FLinearColor NodeDescriptionBackground;

	UPROPERTY(EditAnywhere, config, Category = "NodePopups")
	FLinearColor NodeStatusBackground;

	UPROPERTY(EditAnywhere, config, Category = "NodePopups")
	FLinearColor NodePreloadedBackground;
	
	UPROPERTY(EditAnywhere, config, Category = "Wires")
	FLinearColor InactiveWireColor;

	UPROPERTY(EditAnywhere, config, Category = "Wires", meta = (ClampMin = 0.0f))
	float InactiveWireThickness;

	UPROPERTY(EditAnywhere, config, Category = "Wires", meta = (ClampMin = 1.0f))
	float RecentWireDuration;

	/** The color to display execution wires that were just executed */
	UPROPERTY(EditAnywhere, config, Category = "Wires")
	FLinearColor RecentWireColor;

	UPROPERTY(EditAnywhere, config, Category = "Wires", meta = (ClampMin = 0.0f))
	float RecentWireThickness;

	UPROPERTY(EditAnywhere, config, Category = "Wires")
	FLinearColor RecordedWireColor;

	UPROPERTY(EditAnywhere, config, Category = "Wires", meta = (ClampMin = 0.0f))
	float RecordedWireThickness;

	UPROPERTY(EditAnywhere, config, Category = "Wires")
	bool bHighlightInputWiresOfSelectedNodes;

	UPROPERTY(EditAnywhere, config, Category = "Wires")
	bool bHighlightOutputWiresOfSelectedNodes;

	UPROPERTY(EditAnywhere, config, Category = "Wires")
	FLinearColor SelectedWireColor;

	UPROPERTY(EditAnywhere, config, Category = "Wires", meta = (ClampMin = 0.0f))
	float SelectedWireThickness;
};
