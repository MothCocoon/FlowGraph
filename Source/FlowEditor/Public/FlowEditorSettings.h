#pragma once

#include "Engine/DeveloperSettings.h"

#include "Graph/FlowAssetTypes.h"
#include "FlowEditorSettings.generated.h"

/**
 *
 */
UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Flow Editor"))
class UFlowEditorSettings final : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()

public:
	static UFlowEditorSettings* Get() { return CastChecked<UFlowEditorSettings>(UFlowEditorSettings::StaticClass()->GetDefaultObject()); }

	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	TMap<EFlowNodeStyle, FLinearColor> NodeTitleColors;

	UPROPERTY(EditAnywhere, config, Category = "NodePopups")
	FLinearColor NodeDescriptionBackground;

	UPROPERTY(EditAnywhere, config, Category = "NodePopups")
	FLinearColor NodeStatusBackground;

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

	/** The color to fade to for execution wires on release */
	UPROPERTY(EditAnywhere, config, Category = "Wires")
	FLinearColor InactiveWireColor;

	/** The thickness to drop down to during release / for unexecuted wires when debugging */
	UPROPERTY(EditAnywhere, config, Category = "Wires", meta = (ClampMin = 0.0f))
	float InactiveWireThickness;
};
