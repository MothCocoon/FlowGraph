// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "EdGraph/EdGraphSchema.h"
#include "FlowGraphSchema.generated.h"

class UFlowAsset;
class UFlowNode;

DECLARE_MULTICAST_DELEGATE(FFlowGraphSchemaRefresh);

UCLASS()
class FLOWEDITOR_API UFlowGraphSchema : public UEdGraphSchema
{
	GENERATED_UCLASS_BODY()

	friend class UFlowGraph;

private:
	static bool bInitialGatherPerformed;
	static TArray<UClass*> NativeFlowNodes;
	static TMap<FName, FAssetData> BlueprintFlowNodes;
	static TMap<UClass*, UClass*> GraphNodesByFlowNodes;

	static bool bBlueprintCompilationPending;

public:
	static void SubscribeToAssetChanges();
	static void GetPaletteActions(FGraphActionMenuBuilder& ActionMenuBuilder, const UClass* AssetClass, const FString& CategoryName);

	// EdGraphSchema
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const override;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;
	virtual bool ShouldHidePinDefaultValue(UEdGraphPin* Pin) const override;
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;
	virtual void BreakNodeLinks(UEdGraphNode& TargetNode) const override;
	virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const override;
	virtual int32 GetNodeSelectionCount(const UEdGraph* Graph) const override;
	virtual TSharedPtr<FEdGraphSchemaAction> GetCreateCommentAction() const override;
	virtual void OnPinConnectionDoubleCicked(UEdGraphPin* PinA, UEdGraphPin* PinB, const FVector2D& GraphPosition) const override;
	// --

	static TArray<TSharedPtr<FString>> GetFlowNodeCategories();
	static UClass* GetAssignedGraphNodeClass(const UClass* FlowNodeClass);

private:
	static void ApplyNodeFilter(const UFlowAsset* AssetClassDefaults, const UClass* FlowNodeClass, TArray<UFlowNode*>& FilteredNodes);
	static void GetFlowNodeActions(FGraphActionMenuBuilder& ActionMenuBuilder, const UFlowAsset* AssetClassDefaults, const FString& CategoryName);
	static void GetCommentAction(FGraphActionMenuBuilder& ActionMenuBuilder, const UEdGraph* CurrentGraph = nullptr);

	static bool IsFlowNodePlaceable(const UClass* Class);

	static void OnBlueprintPreCompile(UBlueprint* Blueprint);
	static void OnBlueprintCompiled();
	static void OnHotReload(EReloadCompleteReason ReloadCompleteReason);

	static void GatherNativeNodes();
	static void GatherNodes();

	static void OnAssetAdded(const FAssetData& AssetData);
	static void AddAsset(const FAssetData& AssetData, const bool bBatch);
	static void OnAssetRemoved(const FAssetData& AssetData);

public:
	static FFlowGraphSchemaRefresh OnNodeListChanged;
	static UBlueprint* GetPlaceableNodeBlueprint(const FAssetData& AssetData);

	static const UFlowAsset* GetAssetClassDefaults(const UEdGraph* Graph);
};
