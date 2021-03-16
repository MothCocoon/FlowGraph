#pragma once

#include "EdGraph/EdGraphSchema.h"
#include "FlowGraphSchema.generated.h"

DECLARE_MULTICAST_DELEGATE(FFlowGraphSchemaRefresh);

UCLASS()
class FLOWEDITOR_API UFlowGraphSchema : public UEdGraphSchema
{
	GENERATED_UCLASS_BODY()

private:
	static TArray<UClass*> NativeFlowNodes;
	static TMap<FName, FAssetData> BlueprintFlowNodes;
	
	static TSet<FString> UnsortedCategories;
	static TArray<TSharedPtr<FString>> FlowNodeCategories;

	static TMap<UClass*, UClass*> AssignedGraphNodeClasses;

public:
	static void SubscribeToAssetChanges();
	static void GetPaletteActions(FGraphActionMenuBuilder& ActionMenuBuilder, const FString& CategoryName);

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
	// --

	static TArray<TSharedPtr<FString>> GetFlowNodeCategories();
	static UClass* GetAssignedGraphNodeClass(const UClass* FlowNodeClass);

private:
	static void GetFlowNodeActions(FGraphActionMenuBuilder& ActionMenuBuilder, const FString& CategoryName);
	static void GetCommentAction(FGraphActionMenuBuilder& ActionMenuBuilder, const UEdGraph* CurrentGraph = nullptr);

	static bool IsFlowNodePlaceable(const UClass* Class);
	static void GatherFlowNodes();
	static void OnHotReload(bool bWasTriggeredAutomatically);

	static void OnAssetAdded(const FAssetData& AssetData);
	static void AddAsset(const FAssetData& AssetData, const bool bBatch);
	static void RemoveAsset(const FAssetData& AssetData);
	static void RefreshNodeList();

public:
	static FFlowGraphSchemaRefresh OnNodeListChanged;
	static UBlueprint* GetNodeBlueprint(const FAssetData& AssetData);
};
