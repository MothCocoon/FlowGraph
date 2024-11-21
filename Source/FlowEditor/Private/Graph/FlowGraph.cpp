// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "AddOns/FlowNodeAddOn.h"
#include "Nodes/FlowNode.h"
#include "FlowEditorLogChannels.h"

#include "Editor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraph/EdGraphPin.h"
#include "Logging/LogMacros.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowGraph)

void FFlowGraphInterface::OnInputTriggered(UEdGraphNode* GraphNode, const int32 Index) const
{
	CastChecked<UFlowGraphNode>(GraphNode)->OnInputTriggered(Index);
}

void FFlowGraphInterface::OnOutputTriggered(UEdGraphNode* GraphNode, const int32 Index) const
{
	CastChecked<UFlowGraphNode>(GraphNode)->OnOutputTriggered(Index);
}

UFlowGraph::UFlowGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bLockUpdates = false;
	bIsLoadingGraph = false;

	if (!UFlowAsset::GetFlowGraphInterface().IsValid())
	{
		UFlowAsset::SetFlowGraphInterface(MakeShared<FFlowGraphInterface>());
	}
}

UEdGraph* UFlowGraph::CreateGraph(UFlowAsset* InFlowAsset)
{
	return CreateGraph(InFlowAsset, UFlowGraphSchema::StaticClass());
}

UEdGraph* UFlowGraph::CreateGraph(UFlowAsset* InFlowAsset, TSubclassOf<UFlowGraphSchema> FlowSchema)
{
	check(FlowSchema);
	UEdGraph* NewGraph = CastChecked<UFlowGraph>(FBlueprintEditorUtils::CreateNewGraph(InFlowAsset, NAME_None, StaticClass(), FlowSchema));
	NewGraph->bAllowDeletion = false;

	InFlowAsset->FlowGraph = NewGraph;
	NewGraph->GetSchema()->CreateDefaultNodesForGraph(*NewGraph);

	return NewGraph;
}

void UFlowGraph::RefreshGraph()
{
	if (!GEditor || GEditor->PlayWorld)
	{
		// don't run fixup in PIE
		return;
	}

	// Locking updates to the graph while we update it
	{
		LockUpdates();

		// check if all Graph Nodes have expected, up-to-date type
		const UFlowGraphSchema* FlowGraphSchema = CastChecked<UFlowGraphSchema>(GetSchema());
		FlowGraphSchema->GatherNodes();

		const TMap<FGuid, UFlowNode*>& FlowAssetNodesMap = GetFlowAsset()->GetNodes();
		for (const TPair<FGuid, UFlowNode*>& Node : FlowAssetNodesMap)
		{
			UFlowNode* FlowNode = Node.Value;
			if (!IsValid(FlowNode))
			{
				continue;
			}

			UFlowGraphNode* const ExistingFlowGraphNode = Cast<UFlowGraphNode>(FlowNode->GetGraphNode());

			UFlowGraphNode* RefreshedFlowGraphNode = ExistingFlowGraphNode;

			const TSubclassOf<UEdGraphNode> ExpectGraphNodeClass = UFlowGraphSchema::GetAssignedGraphNodeClass(FlowNode->GetClass());
			UClass* ExistingFlowGraphNodeClass = IsValid(ExistingFlowGraphNode) ? ExistingFlowGraphNode->GetClass() : nullptr;
			if (ExistingFlowGraphNodeClass != ExpectGraphNodeClass)
			{
				// Create a new Flow Graph Node of proper type
				RefreshedFlowGraphNode = FFlowGraphSchemaAction_NewNode::RecreateNode(this, ExistingFlowGraphNode, FlowNode);
			}

			RecursivelyRefreshAddOns(*RefreshedFlowGraphNode);
		}

		UnlockUpdates();
	}

	// refresh nodes
	TArray<UFlowGraphNode*> FlowGraphNodes;
	GetNodesOfClass<UFlowGraphNode>(FlowGraphNodes);
	for (UFlowGraphNode* GraphNode : FlowGraphNodes)
	{
		GraphNode->OnGraphRefresh();
	}
}

void UFlowGraph::NotifyGraphChanged()
{
	UFlowAsset* FlowAsset = GetFlowAsset();

	FlowAsset->HarvestNodeConnections();

	Super::NotifyGraphChanged();
}

UFlowAsset* UFlowGraph::GetFlowAsset() const
{
	return GetTypedOuter<UFlowAsset>();
}

void UFlowGraph::RecursivelyRefreshAddOns(UFlowGraphNode& FromFlowGraphNode)
{
	// Refresh AddOns
	UFlowNodeBase* FromFlowNodeBase = FromFlowGraphNode.GetFlowNodeBase();

	const TArray<UFlowNodeAddOn*> FlowNodeAddOnChildren = FromFlowNodeBase->GetFlowNodeAddOnChildren();
	for (UFlowNodeAddOn* AddOn : FlowNodeAddOnChildren)
	{
		if (!AddOn)
		{
			UE_LOG(
				LogFlowEditor,
				Error,
				TEXT("Missing AddOn detected for node %s (parent %s)"),
				*FromFlowNodeBase->GetName(),
				FromFlowGraphNode.GetParentNode() ?
					*FromFlowGraphNode.GetParentNode()->GetName() :
					TEXT("<null>"));

			continue;
		}

		UFlowGraphNode* const AddOnFlowGraphNode = Cast<UFlowGraphNode>(AddOn->GetGraphNode());

		const TSubclassOf<UEdGraphNode> ExpectAddOnGraphNodeClass = UFlowGraphSchema::GetAssignedGraphNodeClass(AddOn->GetClass());
		UFlowGraphNode* RefreshedAddOnFlowGraphNode = AddOnFlowGraphNode;
		UClass* ExistingAddOnGraphNodeClass = IsValid(AddOnFlowGraphNode) ? AddOnFlowGraphNode->GetClass() : nullptr;

		if (ExistingAddOnGraphNodeClass != ExpectAddOnGraphNodeClass)
		{
			// Create a new Flow Graph Node of proper type for the AddOn
			RefreshedAddOnFlowGraphNode = FFlowSchemaAction_NewSubNode::RecreateNode(this, AddOnFlowGraphNode, &FromFlowGraphNode, AddOn);
		}

		// Recurse for the AddOn's AddOn's
		RecursivelyRefreshAddOns(*RefreshedAddOnFlowGraphNode);
	}
}

void UFlowGraph::RecursivelySetupAllFlowGraphNodesForEditing(UFlowGraphNode& FromFlowGraphNode)
{
	UFlowNodeBase* FromNodeInstance = FromFlowGraphNode.GetFlowNodeBase();
	if (IsValid(FromNodeInstance))
	{
		// Setup all of the flow node (and subnode) instances for editing
		FromNodeInstance->SetupForEditing(FromFlowGraphNode);
	}
	else
	{
		// Reconstruct the node if the NodeInstance is missing
		FromFlowGraphNode.ReconstructNode();
	}

	for (UFlowGraphNode* SubNode : FromFlowGraphNode.SubNodes)
	{
		// Setup all of the flow subnodes for editing
		if (IsValid(SubNode))
		{
			SubNode->SetParentNodeForSubNode(&FromFlowGraphNode);

			RecursivelySetupAllFlowGraphNodesForEditing(*SubNode);
		}
	}
}

void UFlowGraph::UpdateAsset(int32 UpdateFlags)
{
	if (IsLocked())
	{
		return;
	}

	// UpdateAsset is called to do any reconciliation from the editor-version of the 
	//  graph to the runtime version of the graph data.
	// In our case, it will copy the AddOns from their editor-side UFlowGraphNode containers to
	//  their runtime UFlowNode and/or UFlowNodeAddOn ::AddOn array entry (via OnUpdateAsset)
	for (UEdGraphNode* EdNode : Nodes)
	{
		UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(EdNode);
		if (FlowGraphNode)
		{
			FlowGraphNode->OnUpdateAsset(UpdateFlags);
		}
	}
}

void UFlowGraph::OnCreated()
{
	MarkVersion();
}

void UFlowGraph::OnLoaded()
{
	check(GEditor);

	bIsLoadingGraph = true;

	// Setup all the Nodes in the graph for editing
	for (UEdGraphNode* EdNode : Nodes)
	{
		UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(EdNode);
		if (IsValid(FlowGraphNode))
		{
			RecursivelySetupAllFlowGraphNodesForEditing(*FlowGraphNode);
		}
	}

	UpdateDeprecatedClasses();

	if (UpdateUnknownNodeClasses())
	{
		NotifyGraphChanged();
	}

	RefreshGraph();

	bIsLoadingGraph = false;
}

void UFlowGraph::OnSave()
{
	UpdateAsset();
}

void UFlowGraph::Initialize()
{
	UpdateVersion();
}

void UFlowGraph::UpdateVersion()
{
	if (GraphVersion == 1)
	{
		return;
	}

	MarkVersion();
	Modify();

	// Insert any Version updating code here
}

void UFlowGraph::MarkVersion()
{
	GraphVersion = 1;
}

bool UFlowGraph::UpdateUnknownNodeClasses()
{
	bool bUpdated = false;
	for (int32 NodeIdx = 0; NodeIdx < Nodes.Num(); NodeIdx++)
	{
		UFlowGraphNode* MyNode = Cast<UFlowGraphNode>(Nodes[NodeIdx]);
		if (MyNode)
		{
			const bool bUpdatedNode = MyNode->RefreshNodeClass();
			bUpdated = bUpdated || bUpdatedNode;

			for (int32 SubNodeIdx = 0; SubNodeIdx < MyNode->SubNodes.Num(); SubNodeIdx++)
			{
				if (MyNode->SubNodes[SubNodeIdx])
				{
					const bool bUpdatedSubNode = MyNode->SubNodes[SubNodeIdx]->RefreshNodeClass();
					bUpdated = bUpdated || bUpdatedSubNode;
				}
			}
		}
	}

	return bUpdated;
}

FString UFlowGraph::GetDeprecationMessage(const UClass* Class)
{
	static FName MetaDeprecated = TEXT("DeprecatedNode");
	static FName MetaDeprecatedMessage = TEXT("DeprecationMessage");
	FString DefDeprecatedMessage("Please remove it!");
	FString DeprecatedPrefix("DEPRECATED");
	FString DeprecatedMessage;

	if (Class && Class->HasAnyClassFlags(CLASS_Native) && Class->HasMetaData(MetaDeprecated))
	{
		DeprecatedMessage = DeprecatedPrefix + TEXT(": ");
		DeprecatedMessage += Class->HasMetaData(MetaDeprecatedMessage) ? Class->GetMetaData(MetaDeprecatedMessage) : DefDeprecatedMessage;
	}

	return DeprecatedMessage;
}

void UFlowGraph::UpdateFlowGraphNodeErrorMessage(UFlowGraphNode& Node)
{
	// Broke out setting error message in to own function so it can be reused when iterating nodes collection.
	if (Node.GetFlowNodeBase())
	{
		Node.ErrorMessage = GetDeprecationMessage(Node.GetFlowNodeBase()->GetClass());
	}
	else
	{
		// Null instance. Do we have any meaningful class data?
		FString StoredClassName = Node.NodeInstanceClass.GetAssetName();
		StoredClassName.RemoveFromEnd(TEXT("_C"));

		if (!StoredClassName.IsEmpty())
		{
			// There is class data here but the instance was not be created.
			static const FString IsMissingClassMessage(" class missing. Referenced by ");
			Node.ErrorMessage = StoredClassName + IsMissingClassMessage + Node.GetFullName();
		}
	}

	if (Node.HasErrors())
	{
		UE_LOG(LogFlowEditor, Error, TEXT("%s"), *Node.ErrorMessage);
	}
}

void UFlowGraph::ValidateAsset(FFlowMessageLog& MessageLog)
{
	UFlowAsset* FlowAsset = GetFlowAsset();
	if (FlowAsset)
	{
		FlowAsset->ValidateAsset(MessageLog);
	}

	for (int32 Idx = 0, IdxNum = Nodes.Num(); Idx < IdxNum; ++Idx)
	{
		UFlowGraphNode* Node = Cast<UFlowGraphNode>(Nodes[Idx]);
		if (Node != nullptr)
		{
			Node->ValidateGraphNode(MessageLog);
		}
	}
}

void UFlowGraph::UpdateDeprecatedClasses()
{
	// This function sets error messages and logs errors about nodes.

	for (int32 Idx = 0, IdxNum = Nodes.Num(); Idx < IdxNum; ++Idx)
	{
		UFlowGraphNode* Node = Cast<UFlowGraphNode>(Nodes[Idx]);
		if (Node != nullptr)
		{
			UpdateFlowGraphNodeErrorMessage(*Node);
			
			for (int32 SubIdx = 0, SubIdxNum = Node->SubNodes.Num(); SubIdx < SubIdxNum; ++SubIdx)
			{
				if (Node->SubNodes[SubIdx] != nullptr)
				{
					UpdateFlowGraphNodeErrorMessage(*Node->SubNodes[SubIdx]);
				}
			}
		}
	}
}

void UFlowGraph::Serialize(FArchive& Ar)
{
	// Overridden to flags up errors in the behavior tree while cooking.
	Super::Serialize(Ar);

	if (Ar.IsSaving() || Ar.IsCooking())
	{
		// Logging of errors happens in UpdateDeprecatedClasses
		UpdateDeprecatedClasses();
	}
}

void UFlowGraph::UpdateClassData()
{
	for (int32 Idx = 0; Idx < Nodes.Num(); Idx++)
	{
		UFlowGraphNode* Node = Cast<UFlowGraphNode>(Nodes[Idx]);
		if (Node)
		{
			Node->UpdateNodeClassData();

			for (int32 SubIdx = 0; SubIdx < Node->SubNodes.Num(); SubIdx++)
			{
				if (UFlowGraphNode* SubNode = Node->SubNodes[SubIdx])
				{
					SubNode->UpdateNodeClassData();
				}
			}
		}
	}
}

void UFlowGraph::CollectAllNodeInstances(TSet<UObject*>& NodeInstances)
{
	for (int32 Idx = 0; Idx < Nodes.Num(); Idx++)
	{
		UFlowGraphNode* MyNode = Cast<UFlowGraphNode>(Nodes[Idx]);
		if (MyNode)
		{
			NodeInstances.Add(MyNode->GetFlowNodeBase());

			for (int32 SubIdx = 0; SubIdx < MyNode->SubNodes.Num(); SubIdx++)
			{
				if (MyNode->SubNodes[SubIdx])
				{
					NodeInstances.Add(MyNode->SubNodes[SubIdx]->GetFlowNodeBase());
				}
			}
		}
	}
}

bool UFlowGraph::CanRemoveNestedObject(UObject* TestObject) const
{
	return !TestObject->IsA(UEdGraphNode::StaticClass()) &&
		!TestObject->IsA(UEdGraph::StaticClass()) &&
		!TestObject->IsA(UEdGraphSchema::StaticClass());
}

void UFlowGraph::RemoveOrphanedNodes()
{
	TSet<UObject*> NodeInstances;
	CollectAllNodeInstances(NodeInstances);

	NodeInstances.Remove(nullptr);

	// Obtain a list of all nodes actually in the asset and discard unused nodes
	TArray<UObject*> AllInners;
	const bool bIncludeNestedObjects = false;
	GetObjectsWithOuter(GetOuter(), AllInners, bIncludeNestedObjects);
	for (auto InnerIt = AllInners.CreateConstIterator(); InnerIt; ++InnerIt)
	{
		UObject* TestObject = *InnerIt;
		if (!NodeInstances.Contains(TestObject) && CanRemoveNestedObject(TestObject))
		{
			OnNodeInstanceRemoved(TestObject);

			TestObject->SetFlags(RF_Transient);
			TestObject->Rename(NULL, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional | REN_ForceNoResetLoaders);
		}
	}
}

void UFlowGraph::OnNodeInstanceRemoved(UObject* NodeInstance)
{
	// empty in base class
}

void UFlowGraph::OnNodesPasted(const FString& ImportStr)
{
	// empty in base class
}

UEdGraphPin* UFlowGraph::FindGraphNodePin(UEdGraphNode* Node, EEdGraphPinDirection Dir)
{
	UEdGraphPin* Pin = nullptr;
	for (int32 Idx = 0; Idx < Node->Pins.Num(); Idx++)
	{
		if (Node->Pins[Idx]->Direction == Dir)
		{
			Pin = Node->Pins[Idx];
			break;
		}
	}

	return Pin;
}

bool UFlowGraph::IsLocked() const
{
	return bLockUpdates;
}

void UFlowGraph::LockUpdates()
{
	bLockUpdates = true;
}

void UFlowGraph::UnlockUpdates()
{
	bLockUpdates = false;
	UpdateAsset();
}

void UFlowGraph::OnSubNodeDropped()
{
	NotifyGraphChanged();
}
