// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "FlowGraphConnectionDrawingPolicy.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"

#include "FlowTypes.h"
#include "Graph/FlowGraphNodesPolicy.h"
#include "FlowGraphSettings.generated.h"

class UFlowComponent;
class UFlowNode;
class UFlowNodeBase;

USTRUCT()
struct FFlowNodeDisplayStyleConfig
{
	GENERATED_BODY()

public:
	FFlowNodeDisplayStyleConfig() = default;

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
	FLinearColor TitleColor = FLinearColor::White;
};

/**
 * Editor-only graph settings.
 */
UCLASS(Config = Editor, defaultconfig, meta = (DisplayName = "Flow Graph"))
class FLOWEDITOR_API UFlowGraphSettings : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()

	virtual void PostInitProperties() override;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/* Show Flow Asset in Flow category of "Create Asset" menu?
	 * Requires restart after making a change. */
	UPROPERTY(EditAnywhere, config, Category = "Default UI", meta = (ConfigRestartRequired = true))
	bool bExposeFlowAssetCreation = true;

	/* Show Flow Node blueprint in Flow category of "Create Asset" menu?
	 * Requires restart after making a change. */
	UPROPERTY(EditAnywhere, config, Category = "Default UI", meta = (ConfigRestartRequired = true))
	bool bExposeFlowNodeCreation = true;

	/* Show Flow Asset toolbar?
	 * Requires restart after making a change. */
	UPROPERTY(EditAnywhere, config, Category = "Default UI", meta = (ConfigRestartRequired = true))
	bool bShowAssetToolbarAboveLevelEditor = true;

	UPROPERTY(EditAnywhere, config, Category = "Default UI", meta = (ConfigRestartRequired = true))
	FText FlowAssetCategoryName;

	/* Use this class to create new assets. Class picker will show up if None. */
	UPROPERTY(EditAnywhere, config, Category = "Default UI")
	TSoftClassPtr<class UFlowAsset> DefaultFlowAssetClass;

	/* Flow Asset class allowed to be assigned via Level Editor toolbar. */
	UPROPERTY(EditAnywhere, config, Category = "Default UI", meta = (EditCondition = "bShowAssetToolbarAboveLevelEditor"))
	TSoftClassPtr<class UFlowAsset> WorldAssetClass;

	UPROPERTY(EditAnywhere, config, Category = "Details")
	bool bShowFlowTagsInActorDetails = true;

	/** Flow Component only.
	 * Show "Flow" category above regular categories in component's Details panel.
	 * Requires refreshing the Details panel (reselect the object) to see the change. */
	UPROPERTY(EditAnywhere, config, Category = "Details")
	bool bMarkFlowCategoryImportant = true;

	/** Default tag categories used by Flow Identity, and Identity Tag picker in the Flow Component.
	 * Requires refreshing the Details panel (reselect the object) to see the updated filter. */
	UPROPERTY(EditAnywhere, config, Category = "Details")
	TArray<FGameplayTag> DefaultIdentityTagCategories;

	/** Tag categories set per Flow Component class.
	 * The most derived matching class is used.
	 * Overrides DefaultIdentityTagCategories, but only in Identity Tag picker in the Flow Component.
	 * Requires refreshing the Details panel (reselect the object) to see the updated filter. */
	UPROPERTY(EditAnywhere, config, Category = "Details")
	TMap<TSoftClassPtr<UFlowComponent>, FGameplayTagContainer> ComponentIdentityTagCategories;

	/* Hide specific nodes from the Flow Palette without changing the source code.
	 * Requires restart after making a change. */
	UPROPERTY(EditAnywhere, config, Category = "Nodes", meta = (ConfigRestartRequired = true))
	TArray<TSoftClassPtr<class UFlowNodeBase>> NodesHiddenFromPalette;

	/* Configurable map of FlowAsset subclasses to the FlowAssetNodePolicy for that subclass. */
	UPROPERTY(EditAnywhere, Config, Category = "Nodes", meta = (ConfigRestartRequired = true, AllowedClasses = "/Script/Flow.FlowAsset"))
	TMap<FSoftClassPath, FFlowGraphNodesPolicy> PerAssetSubclassFlowNodePolicies;

	/* Allows anyone to override Flow Palette category for specific nodes without modifying source code. */
	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	TMap<TSoftClassPtr<class UFlowNodeBase>, FString> OverridenNodeCategories;

	/* Hide default pin names on simple nodes, reduces UI clutter. */
	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	bool bShowDefaultPinNames = false;

	/* List of prefixes to hide on node titles and palette without need to add custom DisplayName.
	 * If node class has meta = (DisplayName = ... ) or BlueprintDisplayName, those texts will be displayed. */
	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	TArray<FString> NodePrefixesToRemove;

	/* Display Styles for nodes, keyed by Gameplay Tag. */
	UPROPERTY(EditAnywhere, config, Category = "Nodes", meta = (TitleProperty = "{Tag}"))
	TArray<FFlowNodeDisplayStyleConfig> NodeDisplayStyles;

#if WITH_EDITORONLY_DATA
	/* Tags in the NodeDisplayStylesMap, used to detect when the map needs updating. */
	UPROPERTY(Transient)
	FGameplayTagContainer NodeDisplayStylesAuthoredTags;

	/* Cached map of the data in NodeDisplayStyles for GameplayTag-keyed lookup. */
	UPROPERTY(Transient)
	TMap<FGameplayTag, FFlowNodeDisplayStyleConfig> NodeDisplayStylesMap;
#endif

	UPROPERTY(EditAnywhere, config, Category = "Nodes", meta = (Deprecated))
	TMap<EFlowNodeStyle, FLinearColor> NodeTitleColors;

	UPROPERTY(Config, EditAnywhere, Category = "Nodes")
	TMap<TSoftClassPtr<UFlowNode>, FLinearColor> NodeSpecificColors;

	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	FLinearColor ExecPinColorModifier = FLinearColor(0.75f, 0.75f, 0.75f, 1.0f);

	UPROPERTY(EditAnywhere, config, Category = "NodePopups")
	FLinearColor NodeDescriptionBackground = FLinearColor(0.0625f, 0.0625f, 0.0625f, 1.0f);

	UPROPERTY(EditAnywhere, config, Category = "NodePopups")
	FLinearColor NodeStatusBackground = FLinearColor(0.12f, 0.12f, 0.12f, 1.0f);

	UPROPERTY(EditAnywhere, config, Category = "NodePopups")
	FLinearColor NodePreloadedBackground = FLinearColor(0.12f, 0.12f, 0.12f, 1.0f);

	/* Prefix required in the name of an asset created by "Extract to SubGraph".
	 * No requirement if empty. */
	UPROPERTY(EditAnywhere, config, Category = "SubGraph")
	FString SubGraphAssetNamePrefix;

	UPROPERTY(config, EditAnywhere, Category = "Wires")
	EFlowConnectionDrawType ConnectionDrawType = EFlowConnectionDrawType::Default;

	UPROPERTY(config, EditAnywhere, Category = "Wires", meta = (EditCondition = "ConnectionDrawType == EFlowConnectionDrawType::Circuit"))
	float CircuitConnectionAngle = 45.f;

	UPROPERTY(config, EditAnywhere, Category = "Wires", meta = (EditCondition = "ConnectionDrawType == EFlowConnectionDrawType::Circuit"))
	FVector2D CircuitConnectionSpacing = FVector2D(30.f);

	UPROPERTY(EditAnywhere, config, Category = "Wires")
	FLinearColor InactiveWireColor = FLinearColor(0.364f, 0.364f, 0.364f, 1.0f);

	UPROPERTY(EditAnywhere, config, Category = "Wires", meta = (ClampMin = 0.0f))
	float InactiveWireThickness = 1.5f;

	UPROPERTY(EditAnywhere, config, Category = "Wires", meta = (ClampMin = 1.0f))
	float RecentWireDuration = 3.0f;

	/* The color to display execution wires that were just executed. */
	UPROPERTY(EditAnywhere, config, Category = "Wires")
	FLinearColor RecentWireColor = FLinearColor(1.0f, 0.05f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, config, Category = "Wires", meta = (ClampMin = 0.0f))
	float RecentWireThickness = 6.0f;

	UPROPERTY(EditAnywhere, config, Category = "Wires")
	FLinearColor RecordedWireColor = FLinearColor(0.432f, 0.258f, 0.096f, 1.0f);

	UPROPERTY(EditAnywhere, config, Category = "Wires", meta = (ClampMin = 0.0f))
	float RecordedWireThickness = 3.5f;

	UPROPERTY(EditAnywhere, config, Category = "Wires")
	FLinearColor SelectedWireColor = FLinearColor(0.984f, 0.482f, 0.010f, 1.0f);

	UPROPERTY(EditAnywhere, config, Category = "Wires", meta = (ClampMin = 0.0f))
	float SelectedWireThickness = 1.5f;

public:
	virtual FName GetCategoryName() const override { return FName("Flow Graph"); }
	virtual FText GetSectionText() const override { return INVTEXT("Graph Settings"); }

	/* Override-safe category query for Flow Node. */
	static FString GetNodeCategoryForNode(const UFlowNodeBase& FlowNodeBase);

	/* Categories meta string for Identity Tag picker of the given Flow Component class. */
	FString GetIdentityTagCategories(const UClass* ComponentClass) const;

	const TMap<FGameplayTag, FFlowNodeDisplayStyleConfig>& EnsureNodeDisplayStylesMap();
	void TryAddDefaultNodeDisplayStyle(const FFlowNodeDisplayStyleConfig& StyleConfig);
	const FLinearColor* LookupNodeTitleColorForNode(const UFlowNodeBase& FlowNodeBase);
};
