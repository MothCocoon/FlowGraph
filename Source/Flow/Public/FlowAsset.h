// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowMessageLog.h"
#include "FlowSave.h"
#include "FlowTypes.h"
#include "Nodes/Route/FlowNode_Start.h"
#include "FlowAsset.generated.h"

class UFlowNode_CustomInput;
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

DECLARE_DELEGATE(FFlowGraphEvent);

#endif

/**
 * Single asset containing flow nodes.
 */
UCLASS(BlueprintType, hideCategories = Object)
class FLOW_API UFlowAsset : public UObject
{
	GENERATED_UCLASS_BODY()
	friend class UFlowNode;
	friend class UFlowNode_CustomOutput;
	friend class UFlowNode_SubGraph;
	friend class UFlowSubsystem;

	friend class FFlowAssetDetails;
	friend class UFlowGraphSchema;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flow Asset")
	FGuid AssetGuid;

	// Set it to False, if this asset is instantiated as Root Flow for owner that doesn't live in the world
	// This allow to SaveGame support works properly, if owner of Root Flow would be Game Instance or its subsystem
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flow Asset")
	bool bWorldBound;

//////////////////////////////////////////////////////////////////////////
// Graph

#if WITH_EDITOR
	friend class UFlowGraph;

	// UObject
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	// --

	virtual EDataValidationResult ValidateAsset(FFlowMessageLog& MessageLog);
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

protected:
	TArray<TSubclassOf<UFlowNode>> AllowedNodeClasses;
	TArray<TSubclassOf<UFlowNode>> DeniedNodeClasses;

	bool bStartNodePlacedAsGhostNode;

private:
	UPROPERTY()
	TMap<FGuid, UFlowNode*> Nodes;

	/**
	 * Custom Inputs define custom entry points in graph, it's similar to blueprint Custom Events
	 * Sub Graph node using this Flow Asset will generate context Input Pin for every valid Event name on this list
	 */
	UPROPERTY(EditAnywhere, Category = "Sub Graph")
	TArray<FName> CustomInputs;

	/**
	 * Custom Outputs define custom graph outputs, this allow to send signals to the parent graph while executing this graph
	 * Sub Graph node using this Flow Asset will generate context Output Pin for every valid Event name on this list
	 */
	UPROPERTY(EditAnywhere, Category = "Sub Graph")
	TArray<FName> CustomOutputs;

public:
#if WITH_EDITOR
	FFlowGraphEvent OnSubGraphReconstructionRequested;

	UFlowNode* CreateNode(const UClass* NodeClass, UEdGraphNode* GraphNode);

	void RegisterNode(const FGuid& NewGuid, UFlowNode* NewNode);
	void UnregisterNode(const FGuid& NodeGuid);

	// Processes all nodes and creates map of all pin connections
	void HarvestNodeConnections();
#endif

	TMap<FGuid, UFlowNode*> GetNodes() const { return Nodes; }
	UFlowNode* GetNode(const FGuid& Guid) const { return Nodes.FindRef(Guid); }

	template <class T>
	T* GetNode(const FGuid& Guid) const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UFlowNode>::Value, "'T' template parameter to GetNode must be derived from UFlowNode");

		if (UFlowNode* Node = Nodes.FindRef(Guid))
		{
			return Cast<T>(Node);
		}

		return nullptr;
	}

	UFlowNode_Start* GetStartNode() const;

	template <class T>
	void GetNodesInExecutionOrder(TArray<T*>& OutNodes)
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UFlowNode>::Value, "'T' template parameter to GetNodesInExecutionOrder must be derived from UFlowNode");

		if (UFlowNode_Start* FoundStartNode = GetStartNode())
		{
			TSet<TObjectKey<UFlowNode>> IteratedNodes;
			GetNodesInExecutionOrder_Recursive(FoundStartNode, IteratedNodes, OutNodes);
		}
	}

protected:
	template <class T>
	void GetNodesInExecutionOrder_Recursive(UFlowNode* Node, TSet<TObjectKey<UFlowNode>>& IteratedNodes, TArray<T*>& OutNodes)
	{
		IteratedNodes.Add(Node);

		if (T* NodeOfRequiredType = Cast<T>(Node))
		{
			OutNodes.Emplace(NodeOfRequiredType);
		}

		for (UFlowNode* ConnectedNode : Node->GetConnectedNodes())
		{
			if (ConnectedNode && !IteratedNodes.Contains(ConnectedNode))
			{
				GetNodesInExecutionOrder_Recursive(ConnectedNode, IteratedNodes, OutNodes);
			}
		}
	}

public:	
	TArray<FName> GetCustomInputs() const { return CustomInputs; }
	TArray<FName> GetCustomOutputs() const { return CustomOutputs; }

//////////////////////////////////////////////////////////////////////////
// Instances of the template asset

private:
	// Original object holds references to instances
	UPROPERTY(Transient)
	TArray<UFlowAsset*> ActiveInstances;

#if WITH_EDITORONLY_DATA
	TWeakObjectPtr<UFlowAsset> InspectedInstance;

	// Message log for storing runtime errors/notes/warnings that will only last until the next game run
	// Log lives in the asset template, so it can be inspected after ending the PIE
	TSharedPtr<class FFlowMessageLog> RuntimeLog;
#endif

public:
	void AddInstance(UFlowAsset* Instance);
	int32 RemoveInstance(UFlowAsset* Instance);

	void ClearInstances();
	int32 GetInstancesNum() const { return ActiveInstances.Num(); }

#if WITH_EDITOR
	void GetInstanceDisplayNames(TArray<TSharedPtr<FName>>& OutDisplayNames) const;

	void SetInspectedInstance(const FName& NewInspectedInstanceName);
	UFlowAsset* GetInspectedInstance() const { return InspectedInstance.IsValid() ? InspectedInstance.Get() : nullptr; }

	DECLARE_EVENT(UFlowAsset, FRefreshDebuggerEvent);

	FRefreshDebuggerEvent& OnDebuggerRefresh() { return RefreshDebuggerEvent; }
	FRefreshDebuggerEvent RefreshDebuggerEvent;

	DECLARE_EVENT_TwoParams(UFlowAsset, FRuntimeMessageEvent, UFlowAsset*, const TSharedRef<FTokenizedMessage>&);

	FRuntimeMessageEvent& OnRuntimeMessageAdded() { return RuntimeMessageEvent; }
	FRuntimeMessageEvent RuntimeMessageEvent;

private:
	void BroadcastDebuggerRefresh() const;
	void BroadcastRuntimeMessageAdded(const TSharedRef<FTokenizedMessage>& Message);
#endif

//////////////////////////////////////////////////////////////////////////
// Executing asset instance

private:
	UPROPERTY()
	UFlowAsset* TemplateAsset;

	// Object that spawned Root Flow instance, i.e. World Settings or Player Controller
	// This pointer is passed to child instances: Flow Asset instances created by the SubGraph nodes
	TWeakObjectPtr<UObject> Owner;

	// SubGraph node that created this Flow Asset instance
	TWeakObjectPtr<UFlowNode_SubGraph> NodeOwningThisAssetInstance;

	// Flow Asset instances created by SubGraph nodes placed in the current graph
	TMap<TWeakObjectPtr<UFlowNode_SubGraph>, TWeakObjectPtr<UFlowAsset>> ActiveSubGraphs;

	// Execution of the graph always starts from this node, there can be only one StartNode in the graph
	UPROPERTY()
	UFlowNode_Start* StartNode;

	// Optional entry points to the graph, similar to blueprint Custom Events
	UPROPERTY()
	TSet<UFlowNode_CustomInput*> CustomInputNodes;

	UPROPERTY()
	TSet<UFlowNode*> PreloadedNodes;

	// Nodes that have any work left, not marked as Finished yet
	UPROPERTY()
	TArray<UFlowNode*> ActiveNodes;

	// All nodes active in the past, done their work
	UPROPERTY()
	TArray<UFlowNode*> RecordedNodes;

	EFlowFinishPolicy FinishPolicy;

public:
	virtual void InitializeInstance(const TWeakObjectPtr<UObject> InOwner, UFlowAsset* InTemplateAsset);
	virtual void DeinitializeInstance();

	UFlowAsset* GetTemplateAsset() const { return TemplateAsset; }

	// Object that spawned Root Flow instance, i.e. World Settings or Player Controller
	// This pointer is passed to child instances: Flow Asset instances created by the SubGraph nodes
	UFUNCTION(BlueprintPure, Category = "Flow")
	UObject* GetOwner() const { return Owner.Get(); }

	template <class T>
	TWeakObjectPtr<T> GetOwner() const
	{
		return Owner.IsValid() ? Cast<T>(Owner) : nullptr;
	}

	virtual void PreloadNodes();

	virtual void PreStartFlow();
	virtual void StartFlow();

	virtual void FinishFlow(const EFlowFinishPolicy InFinishPolicy, const bool bRemoveInstance = true);

	// Get Flow Asset instance created by the given SubGraph node
	TWeakObjectPtr<UFlowAsset> GetFlowInstance(UFlowNode_SubGraph* SubGraphNode) const;

private:
	void TriggerCustomEvent(UFlowNode_SubGraph* Node, const FName& EventName) const;
	void TriggerCustomOutput(const FName& EventName) const;

	void TriggerInput(const FGuid& NodeGuid, const FName& PinName);

	void FinishNode(UFlowNode* Node);
	void ResetNodes();

public:
	UFlowSubsystem* GetFlowSubsystem() const;
	FName GetDisplayName() const;

	UFlowNode_SubGraph* GetNodeOwningThisAssetInstance() const;
	UFlowAsset* GetParentInstance() const;

	// Are there any active nodes?
	UFUNCTION(BlueprintPure, Category = "Flow")
	bool IsActive() const { return ActiveNodes.Num() > 0; }

	// Returns nodes that have any work left, not marked as Finished yet
	UFUNCTION(BlueprintPure, Category = "Flow")
	TArray<UFlowNode*> GetActiveNodes() const { return ActiveNodes; }

	// Returns nodes active in the past, done their work
	UFUNCTION(BlueprintPure, Category = "Flow")
	TArray<UFlowNode*> GetRecordedNodes() const { return RecordedNodes; }

//////////////////////////////////////////////////////////////////////////
// SaveGame support

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	FFlowAssetSaveData SaveInstance(TArray<FFlowAssetSaveData>& SavedFlowInstances);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void LoadInstance(const FFlowAssetSaveData& AssetRecord);

private:
	void OnActivationStateLoaded(UFlowNode* Node);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "SaveGame")
	void OnSave();

	UFUNCTION(BlueprintNativeEvent, Category = "SaveGame")
	void OnLoad();

public:
	UFUNCTION(BlueprintNativeEvent, Category = "SaveGame")
	bool IsBoundToWorld();

//////////////////////////////////////////////////////////////////////////
// Utils

#if WITH_EDITOR
public:
	void LogError(const FString& MessageToLog, UFlowNode* Node);
	void LogWarning(const FString& MessageToLog, UFlowNode* Node);
	void LogNote(const FString& MessageToLog, UFlowNode* Node);
#endif
};
