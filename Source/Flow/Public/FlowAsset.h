// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "FlowSave.h"
#include "FlowTypes.h"
#include "Asset/FlowAssetParamsTypes.h"
#include "Asset/FlowDeferredTransitionScope.h"
#include "Nodes/FlowNode.h"

#if WITH_EDITOR
#include "FlowMessageLog.h"
#endif

#include "StructUtils/InstancedStruct.h"
#include "Templates/SharedPointer.h"
#include "UObject/ObjectKey.h"

#include "FlowAsset.generated.h"

class UFlowNode_CustomOutput;
class UFlowNode_CustomInput;
class UFlowNode_SubGraph;
class UFlowSubsystem;
struct FFlowPreloadPolicy;
struct FFlowPinConnectionPolicy;

class UEdGraph;
class UEdGraphNode;

#if !UE_BUILD_SHIPPING
DECLARE_DELEGATE(FFlowGraphEvent);
DECLARE_DELEGATE_TwoParams(FFlowSignalEvent, UFlowNode* /*FlowNode*/, const FName& /*PinName*/);
#endif

/**
 * Asset containing Flow nodes organized as non-linear graph.
 */
UCLASS(BlueprintType, hideCategories = Object)
class FLOW_API UFlowAsset : public UObject
{
	GENERATED_UCLASS_BODY()

public:	
	friend class UFlowNode;
	friend class UFlowNode_CustomOutput;
	friend class UFlowNode_SubGraph;
	friend class UFlowSubsystem;

	friend class FFlowAssetDetails;
	friend class FFlowNode_SubGraphDetails;
	friend class UFlowGraphSchema;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flow Asset")
	FGuid AssetGuid;

	/* Set it to False, if this asset is instantiated as Root Flow for owner that doesn't live in the world.
	 * This allows to SaveGame support works properly, if owner of Root Flow would be Game Instance or its subsystem. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flow Asset")
	bool bWorldBound;

//////////////////////////////////////////////////////////////////////////
// Graph (editor-only)

public:
#if WITH_EDITOR
public:	
	friend class UFlowGraph;

	// UObject
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	virtual void PostLoad() override;
	virtual void PreSaveRoot(FObjectPreSaveRootContext ObjectSaveContext) override;
	// --
#endif	

#if WITH_EDITORONLY_DATA
public:
	FSimpleDelegate OnDetailsRefreshRequested;

	static FString ValidationError_NodeClassNotAllowed;
	static FString ValidationError_AddOnNodeClassNotAllowed;
	static FString ValidationError_NullNodeInstance;
	static FString ValidationError_NullAddOnNodeInstance;

private:
	UPROPERTY()
	TObjectPtr<UEdGraph> FlowGraph;
#endif

#if WITH_EDITOR
public:
	void SetupForEditing();

	UEdGraph* GetGraph() const { return FlowGraph; }

	virtual EDataValidationResult ValidateAsset(FFlowMessageLog& MessageLog);

	/* Returns whether the node class is allowed in this flow asset. */
	bool IsNodeOrAddOnClassAllowed(const UClass* FlowNodeClass, FText* OutOptionalFailureReason = nullptr) const;

	virtual TSubclassOf<UFlowAsset> GetDefaultFlowAssetForSubgraphs() const { return GetClass(); }

protected:
	bool CanFlowNodeClassBeUsedByFlowAsset(const UClass& FlowNodeClass) const;
	bool CanFlowAssetUseFlowNodeClass(const UClass& FlowNodeClass) const;
	bool CanFlowAssetReferenceFlowNode(const UClass& FlowNodeClass, FText* OutOptionalFailureReason = nullptr) const;

	bool IsFlowNodeClassInAllowedClasses(const UClass& FlowNodeClass, const TSubclassOf<UFlowNodeBase>& RequiredAncestor = nullptr) const;
	bool IsFlowNodeClassInDeniedClasses(const UClass& FlowNodeClass) const;

private:
	/* Recursively validates the given addon and its children. */
	void ValidateAddOnTree(UFlowNodeAddOn& AddOn, FFlowMessageLog& MessageLog);
#endif

//////////////////////////////////////////////////////////////////////////
// Nodes

protected:
	TArray<TSubclassOf<UFlowNodeBase>> AllowedNodeClasses;
	TArray<TSubclassOf<UFlowNodeBase>> DeniedNodeClasses;

	TArray<TSubclassOf<UFlowNodeBase>> AllowedInSubgraphNodeClasses;
	TArray<TSubclassOf<UFlowNodeBase>> DeniedInSubgraphNodeClasses;

	bool bStartNodePlacedAsGhostNode;

private:
	UPROPERTY()
	TMap<FGuid, TObjectPtr<UFlowNode>> Nodes;

public:
#if WITH_EDITOR
	FFlowGraphEvent OnSubGraphReconstructionRequested;

	UFlowNode* CreateNode(const UClass* NodeClass, UEdGraphNode* GraphNode);

	void RegisterNode(const FGuid& NewGuid, UFlowNode* NewNode);
	void UnregisterNode(const FGuid& NodeGuid);

	/* Processes nodes and updates pin connections from the graph to the UFlowNode (processes all nodes in the graph if passed nullptr). */
	void HarvestNodeConnections(UFlowNode* TargetNode = nullptr);

	static bool TryGetDefaultForInputPinName(const FStructProperty& StructProperty, const void* Container, FString& OutString);
#endif

public:
	const TMap<FGuid, UFlowNode*>& GetNodes() const { return ObjectPtrDecay(Nodes); }
	TArray<UFlowNode*> GetAllNodes() const;

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

	UFUNCTION(BlueprintPure, Category = "FlowAsset", meta = (DeterminesOutputType = "FlowNodeClass"))
	TArray<UFlowNode*> GetNodesInExecutionOrder(UFlowNode* FirstIteratedNode, const TSubclassOf<UFlowNode> FlowNodeClass) const;

	template <class T>
	void GetNodesInExecutionOrder(UFlowNode* FirstIteratedNode, TArray<T*>& OutNodes) const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UFlowNode>::Value, "'T' template parameter to GetNodesInExecutionOrder must be derived from UFlowNode");

		if (FirstIteratedNode)
		{
			TSet<TObjectKey<UFlowNode>> IteratedNodes;
			GetNodesInExecutionOrder_Recursive(FirstIteratedNode, IteratedNodes, OutNodes);
		}
	}

protected:
	template <class T>
	void GetNodesInExecutionOrder_Recursive(UFlowNode* Node, TSet<TObjectKey<UFlowNode>>& IteratedNodes, TArray<T*>& OutNodes) const
	{
		IteratedNodes.Add(Node);

		if (T* NodeOfRequiredType = Cast<T>(Node))
		{
			OutNodes.Emplace(NodeOfRequiredType);
		}

		for (UFlowNode* ConnectedNode : Node->GatherConnectedNodes())
		{
			if (ConnectedNode && !IteratedNodes.Contains(ConnectedNode))
			{
				GetNodesInExecutionOrder_Recursive(ConnectedNode, IteratedNodes, OutNodes);
			}
		}
	}

public:
	UFUNCTION(BlueprintPure, Category = "FlowAsset")
	virtual UFlowNode* GetDefaultEntryNode() const;

//////////////////////////////////////////////////////////////////////////
// Custom Inputs/Outputs
	
#if WITH_EDITORONLY_DATA
protected:
	/* Custom Inputs define custom entry points in graph, it's similar to blueprint Custom Events.
	 * Sub Graph node using this Flow Asset will generate context Input Pin for every valid Event name on this list. */
	UPROPERTY(EditAnywhere, Category = "Sub Graph")
	TArray<FName> CustomInputs;

	/* Custom Outputs define custom graph outputs, this allows to send signals to the parent graph while executing this graph.
	 * Sub Graph node using this Flow Asset will generate context Output Pin for every valid Event name on this list. */
	UPROPERTY(EditAnywhere, Category = "Sub Graph")
	TArray<FName> CustomOutputs;
#endif
	
public:	
	/* Gathers all the nodes that are connected to the Start & Custom Inputs of the flow graph. */
	TArray<UFlowNode*> GatherNodesConnectedToAllInputs() const;
	
	UFlowNode_CustomInput* TryFindCustomInputNodeByEventName(const FName& EventName) const;
	UFlowNode_CustomOutput* TryFindCustomOutputNodeByEventName(const FName& EventName) const;

	TArray<FName> GatherCustomInputNodeEventNames() const;
	TArray<FName> GatherCustomOutputNodeEventNames() const;

#if WITH_EDITOR
	const TArray<FName>& GetCustomInputs() const { return CustomInputs; }
	const TArray<FName>& GetCustomOutputs() const { return CustomOutputs; }

protected:
	void AddCustomInput(const FName& EventName);
	void RemoveCustomInput(const FName& EventName);

	void AddCustomOutput(const FName& EventName);
	void RemoveCustomOutput(const FName& EventName);
#endif

//////////////////////////////////////////////////////////////////////////
// Pin connections

protected:
	/* Policy for UFlowGraphSchema (and others) to use to enforce pin connectivity.
	 * Also used at runtime by predicates (e.g., CompareValues) for type classification queries. */
	UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category = PinConnection)
	TInstancedStruct<FFlowPinConnectionPolicy> PinConnectionPolicy;
	
public:
#if WITH_EDITOR
	/* Override these functions to set up unique policy(ies) for a UFlowAsset subclass */
	virtual void InitializePinConnectionPolicy();
#endif

	const FFlowPinConnectionPolicy& GetPinConnectionPolicy() const;
	
	/*  Return all other Pins connected to the passed Pin. */
	TArray<FConnectedPin> GatherPinsConnectedToPin(const FConnectedPin& Pin) const;

//////////////////////////////////////////////////////////////////////////
// FlowAssetParams support (Start node params for a Flow graph)

	/* Default parameters asset for this Flow Asset (optional). */
	UPROPERTY(EditAnywhere, Category = FlowAssetParams, meta = (ShowCreateNew, HideChildParams))
	FFlowAssetParamsPtr BaseAssetParams;

#if WITH_EDITOR
	/* Generates a new params asset from the Start node. */
	UFlowAssetParams* GenerateParamsFromStartNode();

	/* Generates the FlowAssetParams name for the 'base' (root) asset, used when creating the params asset. */
	virtual FString GenerateParamsAssetName() const;

protected:

	void ReconcileBaseAssetParams(const FDateTime& AssetLastSavedTimestamp);		
#endif

//////////////////////////////////////////////////////////////////////////
// Instances of the template asset

private:
	/* Original object holds references to instances. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UFlowAsset>> ActiveInstances;

#if WITH_EDITORONLY_DATA
	TWeakObjectPtr<const UFlowAsset> InspectedInstance;

	/* Message log for storing runtime errors/notes/warnings that will only last until the next game run.
	 * Log lives in the asset template, so it can be inspected after ending the PIE. */
	TSharedPtr<class FFlowMessageLog> RuntimeLog;
#endif

public:
	void AddInstance(UFlowAsset* Instance);
	int32 RemoveInstance(UFlowAsset* Instance);
	TConstArrayView<TObjectPtr<UFlowAsset>> GetActiveInstances() const { return ActiveInstances; }

	void ClearInstances();
	int32 GetInstancesNum() const { return ActiveInstances.Num(); }

#if WITH_EDITOR
	void SetInspectedInstance(TWeakObjectPtr<const UFlowAsset> NewInspectedInstance);
	const UFlowAsset* GetInspectedInstance() const { return InspectedInstance.IsValid() ? InspectedInstance.Get() : nullptr; }

	DECLARE_EVENT(UFlowAsset, FRefreshDebuggerEvent);

	FRefreshDebuggerEvent& OnDebuggerRefresh() { return RefreshDebuggerEvent; }
	FRefreshDebuggerEvent RefreshDebuggerEvent;

	DECLARE_EVENT_TwoParams(UFlowAsset, FRuntimeMessageEvent, const UFlowAsset*, const TSharedRef<FTokenizedMessage>&);

	FRuntimeMessageEvent& OnRuntimeMessageAdded() { return RuntimeMessageEvent; }
	FRuntimeMessageEvent RuntimeMessageEvent;

private:
	void BroadcastDebuggerRefresh() const;
	void BroadcastRuntimeMessageAdded(const TSharedRef<FTokenizedMessage>& Message) const;
#endif

//////////////////////////////////////////////////////////////////////////
// Executing asset instance

protected:
	UPROPERTY()
	TObjectPtr<UFlowAsset> TemplateAsset;

	/* Object that spawned Root Flow instance, i.e. World Settings or Player Controller.
	 * This pointer is passed to child instances: Flow Asset instances created by the SubGraph nodes. */
	TWeakObjectPtr<UObject> Owner;

	/* SubGraph node that created this Flow Asset instance. */
	TWeakObjectPtr<UFlowNode_SubGraph> NodeOwningThisAssetInstance;

	/* Flow Asset instances created by SubGraph nodes placed in the current graph. */
	TMap<TWeakObjectPtr<UFlowNode_SubGraph>, TWeakObjectPtr<UFlowAsset>> ActiveSubGraphs;

	/* Optional entry points to the graph, similar to blueprint Custom Events.
	 * Contains nodes only if it is initialized instance (see InitializeInstance, IsInstanceInitialized), empty otherwise. */
	UPROPERTY()
	TSet<TObjectPtr<UFlowNode_CustomInput>> CustomInputNodes;

	/* Nodes that have any work left, not marked as Finished yet. */
	UPROPERTY()
	TArray<TObjectPtr<UFlowNode>> ActiveNodes;

	/* All nodes active in the past, done their work. */
	UPROPERTY()
	TArray<TObjectPtr<UFlowNode>> RecordedNodes;

	UPROPERTY(Transient)
	EFlowFinishPolicy FinishPolicy;

public:
	virtual void InitializeInstance(const TWeakObjectPtr<UObject> InOwner, UFlowAsset& InTemplateAsset);
	virtual void DeinitializeInstance();
	bool IsInstanceInitialized() const { return IsValid(TemplateAsset); }

	UFlowAsset* GetTemplateAsset() const { return TemplateAsset; }

	/* Object that spawned Root Flow instance, i.e. World Settings or Player Controller.
	 * This pointer is passed to child instances: Flow Asset instances created by the SubGraph nodes. */
	UFUNCTION(BlueprintPure, Category = "Flow")
	UObject* GetOwner() const { return Owner.Get(); }

	template <class T>
	TWeakObjectPtr<T> GetOwner() const
	{
		return Owner.IsValid() ? Cast<T>(Owner) : nullptr;
	}

	/* Returns the Owner as an Actor, or if Owner is a Component, return its Owner as an Actor. */
	UFUNCTION(BlueprintPure, Category = "Flow")
	AActor* TryFindActorOwner() const;

	virtual void PreStartFlow();
	virtual void StartFlow(IFlowDataPinValueSupplierInterface* DataPinValueSupplier = nullptr);
	bool HasStartedFlow() const;

protected:
	virtual void FinishNode(UFlowNode* Node);
	void ResetNodes();
	
public:	
	virtual void FinishFlow(const EFlowFinishPolicy InFinishPolicy, const bool bRemoveInstance = true);

public:
	UFlowSubsystem* GetFlowSubsystem() const;

	UFlowNode_SubGraph* GetNodeOwningThisAssetInstance() const;
	UFlowAsset* GetParentInstance() const;

	/* Get Flow Asset instance created by the given SubGraph node. */
	TWeakObjectPtr<UFlowAsset> GetFlowInstance(UFlowNode_SubGraph* SubGraphNode) const;

	/* Are there any active nodes? */
	UFUNCTION(BlueprintPure, Category = "Flow")
	bool IsActive() const { return ActiveNodes.Num() > 0; }

	/* Returns nodes that have any work left, not marked as Finished yet. */
	UFUNCTION(BlueprintPure, Category = "Flow")
	const TArray<UFlowNode*>& GetActiveNodes() const { return ActiveNodes; }

	/* Returns nodes active in the past, done their work. */
	UFUNCTION(BlueprintPure, Category = "Flow")
	const TArray<UFlowNode*>& GetRecordedNodes() const { return RecordedNodes; }

//////////////////////////////////////////////////////////////////////////
// Preload policy

protected:
	/* Policy controlling when nodes implementing IFlowPreloadableInterface preload and flush their content.
	 * Initialized from UFlowSettings defaults. Override InitializePreloadPolicy() in a subclass to set a unique policy. */
	UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category = Preload)
	TInstancedStruct<FFlowPreloadPolicy> PreloadPolicy;

	/* Override these functions to set up unique policy(ies) for a UFlowAsset subclass. */
	virtual void InitializePreloadPolicy();

public:
	const FFlowPreloadPolicy& GetPreloadPolicy() const;

//////////////////////////////////////////////////////////////////////////
// Trigger Input

#if !UE_BUILD_SHIPPING
public:	
	FFlowSignalEvent OnPinTriggered;
#endif
	
protected:
	/* Stack of active deferred transition scopes (innermost = top).
	 * Stored as TSharedPtr so callers can safely cache a reference to a specific scope
	 * without it being invalidated by array reallocations/resizes during nested triggers. */
	TArray<TSharedPtr<FFlowDeferredTransitionScope>> DeferredTransitionScopes;
	
public:	
	void TriggerCustomInput(const FName& EventName, IFlowDataPinValueSupplierInterface* DataPinValueSupplier = nullptr);

	void TriggerCustomInput_FromSubGraph(UFlowNode_SubGraph* Node, const FName& EventName) const;
	void TriggerCustomOutput(const FName& EventName);

	/* todo: Extend FromPin through to Node level Trigger functions. */
	virtual void TriggerInput(const FGuid& NodeGuid, const FName& PinName, const FConnectedPin& FromPin);
	
protected:
	/* Trigger the node directly (no deferral, no new scope). */
	void TriggerInputDirect(const FGuid& NodeGuid, const FName& PinName, const FConnectedPin& FromPin);
	
	/* Allow subclasses to disable the standard defer trigger mechanism */
	virtual bool ShouldDeferTriggers() const;

protected:
	void EnqueueDeferredTrigger(const FGuid& NodeGuid, const FName& PinName, const FConnectedPin& FromPin);
	TSharedPtr<FFlowDeferredTransitionScope> PushDeferredTransitionScope();
	void PopDeferredTransitionScope(const TSharedPtr<FFlowDeferredTransitionScope>& Scope);

	bool TryFlushAndRemoveDeferredTransitionScope(const TSharedPtr<FFlowDeferredTransitionScope>& Scope);

public:
	/* Try to flush (and clear) all Deferred Trigger scopes.
	 * Can fail to flush all if a FFlowExecutionGate causes a new halt. */
	bool TryFlushAllDeferredTriggerScopes();

	/* Clear (do not trigger) any remaining deferred transitions (for shutdown cases). */
	void ClearAllDeferredTriggerScopes();
	
protected:	
	void CancelAndWarnForUnflushedDeferredTriggers();

	/* Returns a shared pointer to the current top (innermost) deferred transition scope,
	 * or nullptr if there is no active scope. Safe to cache and use later. */
	TSharedPtr<FFlowDeferredTransitionScope> GetTopDeferredTransitionScope() const;

//////////////////////////////////////////////////////////////////////////
// Expected Owner Class support

protected:
	/* Expects to be owned (at runtime) by an object with this class (or one of its subclasses).
	 * If the class is an AActor, and the Flow Asset is owned by a component, it will consider the component's owner for the AActor. */
	UPROPERTY(EditAnywhere, Category = "Flow")
	TSubclassOf<UObject> ExpectedOwnerClass;
	
public:
	UClass* GetExpectedOwnerClass() const { return ExpectedOwnerClass; }

//////////////////////////////////////////////////////////////////////////
// SaveGame support

public:
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	FFlowAssetSaveData SaveInstance(TArray<FFlowAssetSaveData>& SavedFlowInstances);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void LoadInstance(const FFlowAssetSaveData& AssetRecord);

protected:
	virtual void OnActivationStateLoaded(UFlowNode* Node);

	UFUNCTION(BlueprintNativeEvent, Category = "SaveGame")
	void OnSave();

	UFUNCTION(BlueprintNativeEvent, Category = "SaveGame")
	void OnLoad();

public:
	UFUNCTION(BlueprintNativeEvent, Category = "SaveGame")
	bool IsBoundToWorld() const;

//////////////////////////////////////////////////////////////////////////
// Utils

#if WITH_EDITOR
public:
	void LogError(const FString& MessageToLog, const UFlowNodeBase* Node) const;
	void LogWarning(const FString& MessageToLog, const UFlowNodeBase* Node) const;
	void LogNote(const FString& MessageToLog, const UFlowNodeBase* Node) const;

private:
	/* Shared implementation for LogError/LogWarning/LogNote to avoid code duplication. */
	void LogRuntimeMessage(EMessageSeverity::Type Severity, const FString& MessageToLog, const UFlowNodeBase* Node) const;
#endif
};