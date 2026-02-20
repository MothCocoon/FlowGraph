// FlowGraph.h
// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "EdGraph/EdGraph.h"

#include "FlowAsset.h"
#include "FlowGraph.generated.h"

class SFlowGraphEditor;
class UFlowGraphNode;
class UFlowGraphNode_Reroute;
class UFlowGraphSchema;

/**
 * Flow-specific implementation of engine's EdGraph.
 */
UCLASS()
class FLOWEDITOR_API UFlowGraph : public UEdGraph
{
	GENERATED_UCLASS_BODY()

protected:
	/* Graph version number. */
	UPROPERTY()
	int32 GraphVersion;

	static constexpr int32 CurrentGraphVersion = 2;

	/* If set, graph modifications won't cause updates in internal tree structure.
	 * Flag allows freezing update during heavy changes like pasting new nodes. */
	uint32 bLockUpdates : 1;

	/* Is currently loading the Flow Graph (used to suppress some work during load)? */
	uint32 bIsLoadingGraph : 1;

	bool bIsSavingGraph = false;

	// Reroute nodes that requested a post-unlock type fixup (avoids reconstruct storms on paste)
	UPROPERTY(Transient)
	TSet<TObjectPtr<UFlowGraphNode_Reroute>> PendingRerouteTypeFixups;

public:
	static void CreateGraph(UFlowAsset* InFlowAsset);
	static void CreateGraph(UFlowAsset* InFlowAsset, TSubclassOf<UFlowGraphSchema> FlowSchema);
	void RefreshGraph();

protected:
	void UpgradeAllFlowNodePins();

	void RecursivelyRefreshAddOns(UFlowGraphNode& FromFlowGraphNode);
	static void RecursivelySetupAllFlowGraphNodesForEditing(UFlowGraphNode& FromFlowGraphNode);

	// Run deferred reroute retyping after unlocking updates
	void ProcessPendingRerouteTypeFixups();

public:
	// Called by reroute nodes when graph is locked and type changes should be deferred
	void EnqueueRerouteTypeFixup(UFlowGraphNode_Reroute* RerouteNode);

public:	
	// UEdGraph
	virtual void NotifyGraphChanged() override;
	// --

	UFlowAsset* GetFlowAsset() const;
	void ValidateAsset(FFlowMessageLog& MessageLog);

	// UObject
	virtual void Serialize(FArchive& Ar) override;
	// --
	
public:
	virtual void OnCreated();
	virtual void OnLoaded();
	virtual void OnSave();

	virtual void Initialize();
	virtual void UpdateVersion();
	virtual void MarkVersion();

	void UpdateClassData();
	virtual void UpdateAsset(const int32 UpdateFlags = 0);
	bool UpdateUnknownNodeClasses();
	void UpdateDeprecatedClasses();

protected:
	static void UpdateFlowGraphNodeErrorMessage(UFlowGraphNode& Node);
	static FString GetDeprecationMessage(const UClass* Class);

public:	
	virtual void OnSubNodeDropped();
	virtual void OnNodesPasted(const FString& ImportStr) {}

	void RemoveOrphanedNodes();
	virtual void CollectAllNodeInstances(TSet<UObject*>& NodeInstances);
	virtual bool CanRemoveNestedObject(UObject* TestObject) const;
	virtual void OnNodeInstanceRemoved(UObject* NodeInstance) {}

	static UEdGraphPin* FindGraphNodePin(UEdGraphNode* Node, const EEdGraphPinDirection Direction);

	bool IsLocked() const;
	void LockUpdates();
	void UnlockUpdates();

	bool IsLoadingGraph() const { return bIsLoadingGraph; }
	bool IsSavingGraph() const { return bIsSavingGraph; }
};
