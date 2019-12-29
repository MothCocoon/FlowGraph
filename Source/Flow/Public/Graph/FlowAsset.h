#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "UObject/ObjectMacros.h"
#include "FlowAsset.generated.h"

class UFlowNode;
class UFlowNodeIn;
class UFlowNodeSubFlow;
class UFlowSubsystem;

#if WITH_EDITOR
class UEdGraph;
class UFlowAsset;

/** Interface for flow graph interaction with the FlowEditor module */
class IFlowAssetEditorInterface
{
public:
	virtual ~IFlowAssetEditorInterface() {}

	virtual UEdGraph* CreateGraph(UFlowAsset* InFlowAsset) = 0;

	virtual FGuid CreateGraphNode(UEdGraph* FlowGraph, UFlowNode* FlowNode, bool bSelectNewNode) = 0;
	virtual void CompileNodeConnections(UFlowAsset* FlowAsset) = 0;
};
#endif

/**
 * Single asset containing flow nodes.
 */
UCLASS(hideCategories = Object)
class FLOW_API UFlowAsset : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	friend class UFlowNode;

//////////////////////////////////////////////////////////////////////////
// Graph

#if WITH_EDITOR
public:
	//~ Begin UObject Interface
	virtual void PostInitProperties() override;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	//~ End UObject Interface
#endif

	//~ Begin IFlowAssetEditorInterface
#if WITH_EDITORONLY_DATA
private:
	UPROPERTY()
	UEdGraph* FlowGraph;
#endif

#if WITH_EDITOR
	static TSharedPtr<IFlowAssetEditorInterface> FlowGraphEditor;

public:
	void CreateGraph();
	UEdGraph* GetGraph() { return FlowGraph; };

	FGuid CreateGraphNode(UFlowNode* InFlowNode, bool bSelectNewNode = true);
	void CompileNodeConnections();

	static void SetFlowAssetEditor(TSharedPtr<IFlowAssetEditorInterface> InFlowAssetEditor);
	static TSharedPtr<IFlowAssetEditorInterface> GetFlowAssetEditor() { return FlowGraphEditor; };
	//~ End IFlowAssetEditorInterface
#endif

//////////////////////////////////////////////////////////////////////////
// Nodes

private:
	UPROPERTY()
	TMap<FGuid, UFlowNode*> Nodes;

public:
#if WITH_EDITOR
	/**
	 * Construct flow node
	 */
	template<class T>
	T* CreateNode(TSubclassOf<UFlowNode> FlowNodeClass = T::StaticClass(), bool bSelectNewNode = true)
	{
		T* NewNode = NewObject<T>(this, FlowNodeClass, NAME_None, RF_Transactional);
		const FGuid NodeGuid = CreateGraphNode(NewNode, bSelectNewNode);

		RegisterNode(NodeGuid, Cast<UFlowNode>(NewNode));
		return NewNode;
	}

	void RegisterNode(const FGuid& NewGuid, UFlowNode* NewNode);
	void UnregisterNode(FGuid NodeGuid);
#endif

	UFlowNode* GetNode(const FGuid& Guid) const;

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
	UFlowAsset* GetInspectedInstance() const { return InspectedInstance.IsValid() ? InspectedInstance.Get() : nullptr; };
#endif

//////////////////////////////////////////////////////////////////////////
// Executing graph

public:
	UFlowAsset* TemplateAsset;

private:
	TWeakObjectPtr<UFlowNodeSubFlow> OwningFlowNode;
	TMap<UFlowNodeSubFlow*, TWeakObjectPtr<UFlowAsset>> ChildFlows;

	UPROPERTY()
	TArray<UFlowNodeIn*> InNodes;

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
	void StartSubFlow(UFlowNodeSubFlow* FlowNode);

private:
	void AddChildFlow(UFlowNodeSubFlow* Node, const UFlowAsset* Asset);

	void TriggerInput(const FGuid& NodeGuid, const FName& PinName);

	void FinishNode(UFlowNode* Node);
	void ResetNodes();

public:
	UFlowSubsystem* GetFlowSubsystem() const;
	UFlowNodeSubFlow* GetOwningFlowNode() const { return OwningFlowNode.IsValid() ? OwningFlowNode.Get() : nullptr; };
	UFlowNode* GetNodeInstance(const FGuid Guid) const { return Nodes.FindRef(Guid); };

	bool IsActive() const { return RecordedNodes.Num() > 0; };
	void GetActiveNodes(TArray<UFlowNode*>& OutNodes) const { OutNodes = ActiveNodes; };
	void GetRecordedNodes(TArray<UFlowNode*>& OutNodes) const { OutNodes = RecordedNodes; };
};