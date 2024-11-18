// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "SearchSerializer.h"
#include "Templates/SubclassOf.h"

#include "FlowTypes.h"
#include "Nodes/FlowPin.h"
#include "FlowGraphNode.generated.h"

class UEdGraphSchema;
class UFlowGraph;
class UFlowNodeBase;
class UFlowNode;
class UFlowAsset;
class FFlowMessageLog;

DECLARE_DELEGATE(FFlowGraphNodeEvent);

/**
 * Graph representation of the Flow Node
 */
UCLASS()
class FLOWEDITOR_API UFlowGraphNode : public UEdGraphNode
{
	GENERATED_UCLASS_BODY()

//////////////////////////////////////////////////////////////////////////
// Flow node

private:
	// The FlowNode or FlowNodeAddOn runtime instance that is being edited by this UFlowGraphNode
	UPROPERTY(Instanced)
	UFlowNodeBase* NodeInstance;

	bool bBlueprintCompilationPending;
	bool bIsReconstructingNode;
	bool bNeedsFullReconstruction;
	static bool bFlowAssetsLoaded;

public:
	// It would be intuitive to assign a custom Graph Node class in Flow Node class
	// However, we shouldn't assign class from editor module to runtime module class
	UPROPERTY()
	TArray<TSubclassOf<UFlowNodeBase>> AssignedNodeClasses;
	
	void SetNodeTemplate(UFlowNodeBase* InFlowNodeBase);
	const UFlowNodeBase* GetNodeTemplate() const;

	UFlowNodeBase* GetFlowNodeBase() const;

	// UObject
	virtual void PostLoad() override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	virtual void PostEditImport() override;
	// --

	// UEdGraphNode
	virtual void PostPlacedNewNode() override;
	virtual void PrepareForCopying() override;
    // --
	
	void PostCopyNode();

private:
	void SubscribeToExternalChanges();
	void OnExternalChange();

public:
	virtual void OnGraphRefresh();

//////////////////////////////////////////////////////////////////////////
// Graph node

public:
	UPROPERTY()
	FFlowPinTrait NodeBreakpoint;

	// UEdGraphNode
	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
	// --

	/**
	 * Handles inserting the node between the FromPin and what the FromPin was original connected to
	 *
	 * @param FromPin			The pin this node is being spawned from
	 * @param NewLinkPin		The new pin the FromPin will connect to
	 * @param OutNodeList		Any nodes that are modified will get added to this list for notification purposes
	 */
	void InsertNewNode(UEdGraphPin* FromPin, UEdGraphPin* NewLinkPin, TSet<UEdGraphNode*>& OutNodeList);

	// UEdGraphNode
	virtual void ReconstructNode() override;
	virtual void AllocateDefaultPins() override;
	// --

	// variants of K2Node methods
	void RewireOldPinsToNewPins(TArray<UEdGraphPin*>& InOldPins);
	void ReconstructSinglePin(UEdGraphPin* NewPin, UEdGraphPin* OldPin);
	// --

	// UEdGraphNode
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	virtual bool CanUserDeleteNode() const override;
	virtual bool CanDuplicateNode() const override;
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual bool ShowPaletteIconOnNode() const override { return true; }
	virtual FText GetTooltipText() const override;
	// --

	void CreateAttachAddOnSubMenu(UToolMenu* Menu, UEdGraph* Graph) const;

	bool CanAcceptSubNodeAsChild(const UFlowGraphNode& OtherSubNode, const TSet<const UEdGraphNode*>& AllRootSubNodesToPaste, FString* OutReasonString = nullptr) const;

	bool IsAncestorNode(const UFlowGraphNode& OtherNode) const;

//////////////////////////////////////////////////////////////////////////
// Utils

public:
	// Short summary of node's content
	FString GetNodeDescription() const;

	// Get flow node for the inspected asset instance
	UFlowNode* GetInspectedNodeInstance() const;

	UFlowAsset* GetFlowAsset() const;

	// Used for highlighting active nodes of the inspected asset instance
	EFlowNodeState GetActivationState() const;

	// Information displayed while node is active
	FString GetStatusString() const;
	FLinearColor GetStatusBackgroundColor() const;

	// Check this to display information while node is preloaded
	bool IsContentPreloaded() const;

	bool CanFocusViewport() const;

	// Index properties that are not indexed by default
	virtual void AdditionalNodeIndexing(FSearchSerializer& Serializer) const {}

	// UEdGraphNode
	virtual bool CanJumpToDefinition() const override;
	virtual void JumpToDefinition() const override;
	virtual bool SupportsCommentBubble() const override;
	// --

	/** check if node has any errors, used for assigning colors on graph */
	virtual bool HasErrors() const;

	void ValidateGraphNode(FFlowMessageLog& MessageLog) const;

//////////////////////////////////////////////////////////////////////////
// Pins

public:
	TArray<UEdGraphPin*> InputPins;
	TArray<UEdGraphPin*> OutputPins;

	UPROPERTY()
	TMap<FEdGraphPinReference, FFlowPinTrait> PinBreakpoints;

	void CreateInputPin(const FFlowPin& FlowPin, const int32 Index = INDEX_NONE);
	void CreateOutputPin(const FFlowPin& FlowPin, const int32 Index = INDEX_NONE);

	void RemoveOrphanedPin(UEdGraphPin* Pin);

	bool SupportsContextPins() const;

	bool CanUserAddInput() const;
	bool CanUserAddOutput() const;

	bool CanUserRemoveInput(const UEdGraphPin* Pin) const;
	bool CanUserRemoveOutput(const UEdGraphPin* Pin) const;

	void AddUserInput();
	void AddUserOutput();

	// Add pin only on this instance of node, under default pins
	void AddInstancePin(const EEdGraphPinDirection Direction, const uint8 NumberedPinsAmount);

	// Call node and graph updates manually, if using bBatchRemoval
	void RemoveInstancePin(UEdGraphPin* Pin);

	// Create pins from the context asset, i.e. Sequencer events
	void RefreshContextPins(const bool bReconstructNode);

	// UEdGraphNode
	virtual void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const override;
	// --

	// @return true, if pins cannot be connected due to node's inner logic, put message for user in OutReason
	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const { return false; }

protected:
	// Gets the PinCategory from the FlowPin
	// (accounting for FFlowPin structs that predate the PinCategory field)
	const FName& GetPinCategoryFromFlowPin(const FFlowPin& FlowPin) const;

//////////////////////////////////////////////////////////////////////////
// Breakpoints

public:
	void OnInputTriggered(const int32 Index);
	void OnOutputTriggered(const int32 Index);

private:
	void TryPausingSession(bool bPauseSession);

	void OnResumePIE(const bool bIsSimulating);
	void OnEndPIE(const bool bIsSimulating);
	void ResetBreakpoints();

//////////////////////////////////////////////////////////////////////////
// Execution Override

public:
	FFlowGraphNodeEvent OnSignalModeChanged;
	
	// Pin activation forced by user during PIE
	virtual void ForcePinActivation(const FEdGraphPinReference PinReference) const;

	// Pass-through forced by designer, set per node instance
	virtual void SetSignalMode(const EFlowSignalMode Mode);

	virtual EFlowSignalMode GetSignalMode() const;
	virtual bool CanSetSignalMode(const EFlowSignalMode Mode) const;

//////////////////////////////////////////////////////////////////////////
// SubNode Support

	//~ Begin UEdGraphNode Interface
	UFlowGraph* GetFlowGraph() const;
	virtual void DestroyNode() override;
	virtual void NodeConnectionListChanged() override;
	virtual void FindDiffs(class UEdGraphNode* OtherNode, struct FDiffResults& Results) override;
	virtual FString GetPropertyNameAndValueForDiff(const FProperty* Prop, const uint8* PropertyAddr) const override;
	//~ End UEdGraphNode Interface

	void SetParentNodeForSubNode(UFlowGraphNode* InParentNode);
	UFlowGraphNode* GetParentNode() const { return ParentNode; }

	void OnUpdateAsset(int32 UpdateFlags) { RebuildRuntimeAddOnsFromEditorSubNodes(); }
	void RebuildRuntimeAddOnsFromEditorSubNodes();

	static void DiffSubNodes(
		const FText& NodeTypeDisplayName,
		const TArray<UFlowGraphNode*>& LhsSubNodes,
		const TArray<UFlowGraphNode*>& RhsSubNodes,
		FDiffResults& Results);

	//~ Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditUndo() override;
#endif
	// End UObject

	// @return the input pin for this state
	virtual UEdGraphPin* GetInputPin(int32 InputIndex = 0) const;
	// @return the output pin for this state
	virtual UEdGraphPin* GetOutputPin(int32 InputIndex = 0) const;
	virtual UEdGraph* GetBoundGraph() const { return NULL; }

	virtual FText GetDescription() const;

	void AddSubNode(UFlowGraphNode* SubNode, class UEdGraph* ParentGraph);
	void RemoveSubNode(UFlowGraphNode* SubNode);
	virtual void RemoveAllSubNodes();
	virtual void OnSubNodeRemoved(UFlowGraphNode* SubNode);
	virtual void OnSubNodeAdded(UFlowGraphNode* SubNode);

	virtual int32 FindSubNodeDropIndex(UFlowGraphNode* SubNode) const;
	virtual void InsertSubNodeAt(UFlowGraphNode* SubNode, int32 DropIndex);

	/** check if node is subnode */
	virtual bool IsSubNode() const;

	/** initialize instance object  */
	virtual void InitializeInstance();

	/** reinitialize node instance */
	virtual bool RefreshNodeClass();

	/** updates ClassData from node instance */
	virtual void UpdateNodeClassData();

	/** Check if node instance uses blueprint for its implementation */
	bool UsesBlueprint() const;

protected:

	virtual void ResetNodeOwner();

	void LogError(const FString& MessageToLog, const UFlowNodeBase* FlowNodeBase) const;

	bool HavePinsChanged();

public:
	
	/** instance class */
	UPROPERTY()
	TSoftClassPtr<UFlowNodeBase> NodeInstanceClass;

	/** SubNodes that are owned by this UFlowGraphNode */
	UPROPERTY()
	TArray<TObjectPtr<UFlowGraphNode>> SubNodes;

	/** subnode's parent index assigned during copy operation to connect nodes again on paste */
	UPROPERTY()
	int32 CopySubNodeParentIndex = INDEX_NONE;

	/** subnode index assigned during copy operation to connect nodes again on paste */
	UPROPERTY()
	int32 CopySubNodeIndex = INDEX_NONE;

	/** if set, this node will be always considered as subnode */
	UPROPERTY()
	bool bIsSubNode = false;

	/** if set, this node has context pins from the last RefreshContextPins */
	UPROPERTY()
	bool bHasContextPins = false;

	/** error message for node */
	UPROPERTY()
	FString ErrorMessage;

private:
	/** parent UFlowGraphNode for this node, 
	  * note, this is not saved, and is restored in when the graph is opened in the editor via 
	  * UFlowGraph::RecursivelySetParentNodeForAllSubNodes */
	UPROPERTY(Transient)
	TObjectPtr<UFlowGraphNode> ParentNode;
};
