// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowGraphConnectionDrawingPolicy.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"

#include "FlowTypes.h"
#include "FlowGraphSettings.generated.h"

class UFlowNodeBase;

USTRUCT()
struct FFlowNodeDisplayStyleConfig
{
	GENERATED_BODY()

public:
	FFlowNodeDisplayStyleConfig()
		: TitleColor(FLinearColor::White)
	{
	}

	FFlowNodeDisplayStyleConfig(const FGameplayTag& InTag, const FLinearColor& InNodeColor)
		: Tag(InTag)
		, TitleColor(InNodeColor)
	{
	}

	FORCEINLINE bool operator ==(const FFlowNodeDisplayStyleConfig& Other) const
	{
		return Tag == Other.Tag;
	}

	FORCEINLINE bool operator !=(const FFlowNodeDisplayStyleConfig& Other) const
	{
		return Tag != Other.Tag;
	}

	FORCEINLINE bool operator <(const FFlowNodeDisplayStyleConfig& Other) const
	{
		return Tag < Other.Tag;
	}

public:
	UPROPERTY(Config, EditAnywhere, Category = "Nodes", meta = (Categories = "Flow.NodeStyle"))
	FGameplayTag Tag;

	UPROPERTY(Config, EditAnywhere, Category = "Nodes")
	FLinearColor TitleColor;
};

/**
 *
 */
UCLASS(Config = Editor, defaultconfig, meta = (DisplayName = "Flow Graph"))
class FLOWEDITOR_API UFlowGraphSettings : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()

	static UFlowGraphSettings* Get() { return StaticClass()->GetDefaultObject<UFlowGraphSettings>(); }

	virtual void PostInitProperties() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Show Flow Asset in Flow category of "Create Asset" menu?
	* Requires restart after making a change. */
	UPROPERTY(EditAnywhere, config, Category = "Default UI", meta = (ConfigRestartRequired = true))
	bool bExposeFlowAssetCreation;

	/** Show Flow Node blueprint in Flow category of "Create Asset" menu?
	* Requires restart after making a change. */
	UPROPERTY(EditAnywhere, config, Category = "Default UI", meta = (ConfigRestartRequired = true))
	bool bExposeFlowNodeCreation;

	/** Show Flow Asset toolbar?
	* Requires restart after making a change. */
	UPROPERTY(EditAnywhere, config, Category = "Default UI", meta = (ConfigRestartRequired = true))
	bool bShowAssetToolbarAboveLevelEditor;

	UPROPERTY(EditAnywhere, config, Category = "Default UI", meta = (ConfigRestartRequired = true))
	FText FlowAssetCategoryName;

	/** Use this class to create new assets. Class picker will show up if None */
	UPROPERTY(EditAnywhere, config, Category = "Default UI")
	TSubclassOf<class UFlowAsset> DefaultFlowAssetClass;

	/** Flow Asset class allowed to be assigned via Level Editor toolbar*/
	UPROPERTY(EditAnywhere, config, Category = "Default UI", meta = (EditCondition = "bShowAssetToolbarAboveLevelEditor"))
	TSubclassOf<class UFlowAsset> WorldAssetClass;

	/** Hide specific nodes from the Flow Palette without changing the source code.
	* Requires restart after making a change. */
	UPROPERTY(EditAnywhere, config, Category = "Nodes", meta = (ConfigRestartRequired = true))
	TArray<TSubclassOf<class UFlowNode>> NodesHiddenFromPalette;

	/** Allows anyone to override Flow Palette category for specific nodes without modifying source code.*/
	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	TMap<TSubclassOf<class UFlowNode>, FString> OverridenNodeCategories;

	/** Hide default pin names on simple nodes, reduces UI clutter */
	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	bool bShowDefaultPinNames;

	/** List of prefixes to hide on node titles and palette without need to add custom DisplayName.
	 * If node class has meta = (DisplayName = ... ) or BlueprintDisplayName, those texts will be displayed */
	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	TArray<FString> NodePrefixesToRemove;

	// Display Styles for nodes, keyed by Gameplay Tag
	UPROPERTY(EditAnywhere, config, Category = "Nodes", meta = (TitleProperty = "{Tag}}"))
	TArray<FFlowNodeDisplayStyleConfig> NodeDisplayStyles;

#if WITH_EDITORONLY_DATA
	// Tags in the NodeDisplayStylesMap, used to detect when the map needs updating
	UPROPERTY(Transient)
	FGameplayTagContainer NodeDisplayStylesAuthoredTags;

	// Cached map of the data in NodeDisplayStyles for GameplayTag-keyed lookup
	UPROPERTY(Transient)
	TMap<FGameplayTag, FFlowNodeDisplayStyleConfig> NodeDisplayStylesMap;
#endif

	UPROPERTY(EditAnywhere, config, Category = "Nodes", meta = (Deprecated))
	TMap<EFlowNodeStyle, FLinearColor> NodeTitleColors;

	UPROPERTY(Config, EditAnywhere, Category = "Nodes")
	TMap<TSubclassOf<UFlowNode>, FLinearColor> NodeSpecificColors;

	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	FLinearColor ExecPinColorModifier;

	UPROPERTY(EditAnywhere, config, Category = "NodePopups")
	FLinearColor NodeDescriptionBackground;

	UPROPERTY(EditAnywhere, config, Category = "NodePopups")
	FLinearColor NodeStatusBackground;

	UPROPERTY(EditAnywhere, config, Category = "NodePopups")
	FLinearColor NodePreloadedBackground;

	UPROPERTY(config, EditAnywhere, Category = "Wires")
	EFlowConnectionDrawType ConnectionDrawType;

	UPROPERTY(config, EditAnywhere, Category = "Wires", meta = (EditCondition = "ConnectionDrawType == EFlowConnectionDrawType::Circuit"))
	float CircuitConnectionAngle;

	UPROPERTY(config, EditAnywhere, Category = "Wires", meta = (EditCondition = "ConnectionDrawType == EFlowConnectionDrawType::Circuit"))
	FVector2D CircuitConnectionSpacing;

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
	FLinearColor SelectedWireColor;

	UPROPERTY(EditAnywhere, config, Category = "Wires", meta = (ClampMin = 0.0f))
	float SelectedWireThickness;

public:
	virtual FName GetCategoryName() const override { return FName("Flow Graph"); }
	virtual FText GetSectionText() const override { return INVTEXT("Graph Settings"); }

#if WITH_EDITOR
	const TMap<FGameplayTag, FFlowNodeDisplayStyleConfig>& EnsureNodeDisplayStylesMap();
	bool TryAddDefaultNodeDisplayStyle(const FFlowNodeDisplayStyleConfig& StyleConfig);
	const FLinearColor* LookupNodeTitleColorForNode(const UFlowNodeBase& FlowNodeBase);
#endif
};
