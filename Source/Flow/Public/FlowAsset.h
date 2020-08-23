#pragma once

#include "CoreMinimal.h"
#include "FlowAsset.generated.h"

class UFlowNode;
class UFlowNode_CustomEvent;
class UFlowNode_Start;
class UFlowNode_SubGraph;
class UFlowSubsystem;

class UEdGraph;
class UFlowAsset;

/** Interface for calling the graph editor methods */
class IFlowGraphInterface
{
public:
	virtual ~IFlowGraphInterface() {}

	virtual void OnInputTriggered(UEdGraphNode* GraphNode, const int32 Index) = 0;
	virtual void OnOutputTriggered(UEdGraphNode* GraphNode, const int32 Index) = 0;
};

#if WITH_EDITOR
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

//////////////////////////////////////////////////////////////////////////
// Graph

#if WITH_EDITOR
	friend class UFlowGraph;

	// UObject
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
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

	UPROPERTY(EditDefaultsOnly, Category = "Flow")
	TArray<FName> CustomEvents;
	
	UPROPERTY(EditDefaultsOnly, Category = "Flow")
	TArray<FName> CustomOutputs;

public:
#if WITH_EDITOR
	UFlowNode* CreateNode(const UClass* NodeClass, UEdGraphNode* GraphNode);
	
	void RegisterNode(const FGuid& NewGuid, UFlowNode* NewNode);
	void UnregisterNode(const FGuid& NodeGuid);
#endif

	void HarvestNodeConnections();
	
	UFlowNode* GetNode(const FGuid& Guid) const;
	TMap<FGuid, UFlowNode*> GetNodes() const { return Nodes; }

	TArray<FName> GetCustomEvents() const { return CustomEvents; }
	TArray<FName> GetCustomOutputs() const { return CustomOutputs; }

	/**
	 * Recursively finds nodes of type T
	 */
	template<typename T>
	void RecursiveFindNodes(UFlowNode* Node, const uint8 Depth, TArray<UFlowNode*>& OutNodes)
	{
		if (Node)
		{
			// Record the node if it is the desired type
			if (T* FoundNode = Cast<T>(Node))
			{
				OutNodes.AddUnique(FoundNode);
			}

			if (OutNodes.Num() == Depth)
			{
				return;
			}

			// Recurse
			for (const FGuid& Guid : Node->GetConnectedNodes())
			{
				if (UFlowNode* ConnectedNode = GetNode(Guid))
				{
					RecursiveFindNodes<T>(ConnectedNode, Depth, OutNodes);
				}
			}
		}
	}

	void RecursiveFindNodesByClass(UFlowNode* Node, const TSubclassOf<UFlowNode> Class, uint8 Depth, TArray<UFlowNode*>& OutNodes) const;

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
	int32 GetInstancesNum() const { return ActiveInstances.Num(); };

#if WITH_EDITOR
	void GetInstanceDisplayNames(TArray<TSharedPtr<FName>>& OutDisplayNames) const;
	
	void SetInspectedInstance(const FName& NewInspectedInstanceName);
	UFlowAsset* GetInspectedInstance() const { return InspectedInstance.IsValid() ? InspectedInstance.Get() : nullptr; }

	DECLARE_EVENT(UFlowAsset, FRegenerateToolbarsEvent);
	FRegenerateToolbarsEvent& OnRegenerateToolbars() { return RenegateToolbarsEvent; }

	FRegenerateToolbarsEvent RenegateToolbarsEvent;

private:
	void BroadcastRegenerateToolbars() { RenegateToolbarsEvent.Broadcast(); }
#endif

//////////////////////////////////////////////////////////////////////////
// Executing graph

public:
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

	UPROPERTY()
	TArray<UFlowNode*> ActiveNodes;

	UPROPERTY()
	TArray<UFlowNode*> RecordedNodes;

public:
	void InitInstance(UFlowAsset* InTemplateAsset);

	void PreloadNodes();
	void FlushPreload();

	void StartFlow();
	void StartSubFlow(UFlowNode_SubGraph* SubGraphNode);
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

	bool IsActive() const { return RecordedNodes.Num() > 0; }
	void GetActiveNodes(TArray<UFlowNode*>& OutNodes) const { OutNodes = ActiveNodes; }
	void GetRecordedNodes(TArray<UFlowNode*>& OutNodes) const { OutNodes = RecordedNodes; }
};
