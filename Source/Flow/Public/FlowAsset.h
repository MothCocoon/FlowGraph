#pragma once

#include "CoreMinimal.h"
#include "FlowAsset.generated.h"

class UFlowNode;
class UFlowNode_CustomEvent;
class UFlowNode_Start;
class UFlowNode_SubGraph;
class UFlowSubsystem;

class UEdGraph;
class UEdGraphNode;
class UFlowAsset;

#if WITH_EDITOR

/** Interface for calling the graph editor methods */
class FLOW_API IFlowGraphInterface
{
public:
	IFlowGraphInterface() {}
	virtual ~IFlowGraphInterface() {}

	virtual void OnInputTriggered(UEdGraphNode* GraphNode, const int32 Index) const {}
	virtual void OnOutputTriggered(UEdGraphNode* GraphNode, const int32 Index) const {}
};

DECLARE_DELEGATE(FFlowAssetEvent);
#endif

/**
 * Single asset containing flow nodes.
 */
UCLASS(hideCategories = Object)
class FLOW_API UFlowAsset : public UObject
{
	GENERATED_UCLASS_BODY()

	friend class UFlowNode;
	friend class UFlowNode_CustomOutput;
	friend class UFlowNode_SubGraph;

	friend class FFlowAssetDetails;

//////////////////////////////////////////////////////////////////////////
// Graph

#if WITH_EDITOR
	friend class UFlowGraph;

	// UObject
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;
	// --
#endif

	// IFlowGraphInterface
#if WITH_EDITORONLY_DATA
private:
	UPROPERTY()
	UEdGraph* FlowGraph;

	static TSharedPtr<IFlowGraphInterface> FlowGraphInterface;
#endif

public:
#if WITH_EDITOR
	UEdGraph* GetGraph() const { return FlowGraph; };

	static void SetFlowGraphInterface(TSharedPtr<IFlowGraphInterface> InFlowAssetEditor);
	static TSharedPtr<IFlowGraphInterface> GetFlowGraphInterface() { return FlowGraphInterface; };
#endif
	// -- 

//////////////////////////////////////////////////////////////////////////
// Nodes

private:
	UPROPERTY()
	TMap<FGuid, UFlowNode*> Nodes;

	/**
	 * Custom Events define custom entry points in graph, it's similar to blueprint Custom Events
	 * Sub Graph node using this Flow Asset will generate context Input Pin for every valid Event name on this list
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Flow")
	TArray<FName> CustomEvents;

	/**
	 * Custom Outputs define custom graph outputs, this allow to send signals to the parent graph while executing this graph
	 * Sub Graph node using this Flow Asset will generate context Output Pin for every valid Event name on this list
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Flow")
	TArray<FName> CustomOutputs;

public:
#if WITH_EDITOR
	FFlowAssetEvent OnSubGraphReconstructionRequested;

	UFlowNode* CreateNode(const UClass* NodeClass, UEdGraphNode* GraphNode);

	void RegisterNode(const FGuid& NewGuid, UFlowNode* NewNode);
	void UnregisterNode(const FGuid& NodeGuid);
#endif

	// Processes all nodes and creates map of all pin connections
	void HarvestNodeConnections();

	UFlowNode* GetNode(const FGuid& Guid) const;
	TMap<FGuid, UFlowNode*> GetNodes() const { return Nodes; }

	TArray<FName> GetCustomEvents() const { return CustomEvents; }
	TArray<FName> GetCustomOutputs() const { return CustomOutputs; }

//////////////////////////////////////////////////////////////////////////
// Instanced asset

private:
	// Original object holds references to instances
	UPROPERTY(Transient)
	TArray<UFlowAsset*> ActiveInstances;

#if WITH_EDITORONLY_DATA
	TWeakObjectPtr<UFlowAsset> InspectedInstance;
#endif

public:
	void AddInstance(UFlowAsset* NewInstance);
	int32 RemoveInstance(UFlowAsset* Instance);

	void ClearInstances();
	int32 GetInstancesNum() const { return ActiveInstances.Num(); }

#if WITH_EDITOR
	void GetInstanceDisplayNames(TArray<TSharedPtr<FName>>& OutDisplayNames) const;

	void SetInspectedInstance(const FName& NewInspectedInstanceName);
	UFlowAsset* GetInspectedInstance() const { return InspectedInstance.IsValid() ? InspectedInstance.Get() : nullptr; }

	DECLARE_EVENT(UFlowAsset, FRegenerateToolbarsEvent);

	FRegenerateToolbarsEvent& OnRegenerateToolbars() { return RegenerateToolbarsEvent; }

	FRegenerateToolbarsEvent RegenerateToolbarsEvent;

private:
	void BroadcastRegenerateToolbars() const { RegenerateToolbarsEvent.Broadcast(); }
#endif

//////////////////////////////////////////////////////////////////////////
// Executing graph

public:
	UPROPERTY()
	UFlowAsset* TemplateAsset;

private:
	TWeakObjectPtr<UFlowNode_SubGraph> NodeOwningThisAssetInstance;
	TMap<TWeakObjectPtr<UFlowNode_SubGraph>, TWeakObjectPtr<UFlowAsset>> ActiveSubGraphs;

	UPROPERTY()
	UFlowNode_Start* StartNode;

	UPROPERTY()
	TMap<FName, UFlowNode_CustomEvent*> CustomEventNodes;

	UPROPERTY()
	TSet<UFlowNode*> PreloadedNodes;

	// Nodes that have any work left, not marked as Finished yet
	UPROPERTY()
	TArray<UFlowNode*> ActiveNodes;

	// All nodes active in the past, done their work
	UPROPERTY()
	TArray<UFlowNode*> RecordedNodes;

public:
	void InitInstance(UFlowAsset* InTemplateAsset);
	void PreloadNodes();

	virtual void StartFlow();
	virtual void StartAsSubFlow(UFlowNode_SubGraph* SubGraphNode);
	virtual void FinishFlow(const bool bFlowCompleted);

	TWeakObjectPtr<UFlowAsset> GetFlowInstance(UFlowNode_SubGraph* SubGraphNode) const;

private:
	void TriggerCustomEvent(UFlowNode_SubGraph* Node, const FName& EventName);
	void TriggerCustomOutput(const FName& EventName) const;

	void TriggerInput(const FGuid& NodeGuid, const FName& PinName);

	void FinishNode(UFlowNode* Node);
	void ResetNodes();

public:
	UFlowSubsystem* GetFlowSubsystem() const;
	FName GetDisplayName() const;

	UFlowNode_SubGraph* GetNodeOwningThisAssetInstance() const;
	UFlowAsset* GetMasterInstance() const;
	UFlowNode* GetNodeInstance(const FGuid Guid) const;

	// Are there any active nodes?
	UFUNCTION(BlueprintPure, Category = "Flow")
	bool IsActive() const { return ActiveNodes.Num() > 0; }

	// Returns nodes that have any work left, not marked as Finished yet
	UFUNCTION(BlueprintPure, Category = "Flow")
	TArray<UFlowNode*> GetActiveNodes() const { return ActiveNodes; }

	// Returns nodes active in the past, done their work
	UFUNCTION(BlueprintPure, Category = "Flow")
	TArray<UFlowNode*> GetRecordedNodes() const { return RecordedNodes; }
};
