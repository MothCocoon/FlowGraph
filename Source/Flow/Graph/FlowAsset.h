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

	/**
	 * Recursively finds nodes of type T
	 */
	template<typename T>
	void RecursiveFindChildNodes(const UFlowNode* Node, TArray<const T*>& OutNodes)
	{
		if (Node)
		{
			// Record the node if it is the desired type
			if (T* FoundNode = Cast<T>(Node))
			{
				OutNodes.AddUnique(FoundNode);
			}

			// Recurse
			for (const UFlowNode* ChildNode : Node->GetChildNodes())
			{
				RecursiveFindNode<T>(ChildNode, OutNodes);
			}
		}
	}

//////////////////////////////////////////////////////////////////////////
// Runtime

private:
	// Original object holds references to instances
	UPROPERTY(Transient)
	TArray<UFlowAsset*> ActiveInstances;

#if WITH_EDITORONLY_DATA
	TWeakObjectPtr<UFlowAsset> InspectedInstance;
#endif

public:
	void AddInstance(UFlowAsset* NewInstance);
	void ClearInstances();

#if WITH_EDITOR
	UFlowAsset* GetInspectedInstance() const { return InspectedInstance.IsValid() ? InspectedInstance.Get() : nullptr; };
#endif

//////////////////////////////////////////////////////////////////////////
// Instanced asset
protected:
	TWeakObjectPtr<UFlowSubsystem> FlowSubsystem;
	TWeakObjectPtr<UFlowNodeSubFlow> OwningFlowNode;
	TMap<UFlowNodeSubFlow*, TWeakObjectPtr<UFlowAsset>> ChildFlows;

	TArray<UFlowNodeIn*> InNodes;

	TArray<UFlowNode*> ActiveNodes;
	TArray<UFlowNode*> RecordedNodes;

public:
	void CreateNodeInstances();
	void StartFlow(UFlowSubsystem* Subsystem);
	void StartSubFlow(UFlowSubsystem* Subsystem, UFlowNodeSubFlow* FlowNode, UFlowAsset* ParentAsset);

private:
	void AddChildFlow(UFlowNodeSubFlow* Node, const UFlowAsset* Asset);
	void ActivateInput(UFlowNode* Node, const uint8 Pin);
	void ActivateInput(const FGuid& NodeGuid, const uint8 Pin);
	void FinishNode(UFlowNode* Node);
	void ResetNodes();

public:
	UFlowSubsystem* GetFlowSubsystem() const { return FlowSubsystem.IsValid() ? FlowSubsystem.Get() : nullptr; };
	UFlowNode* GetNodeInstance(const FGuid Guid) const { return Nodes.FindRef(Guid); };

	bool IsActive() const { return RecordedNodes.Num() > 0; };
	void GetActiveNodes(TArray<UFlowNode*>& OutNodes) const { OutNodes = ActiveNodes; };
	void GetRecordedNodes(TArray<UFlowNode*>& OutNodes) const { OutNodes = RecordedNodes; };
};
