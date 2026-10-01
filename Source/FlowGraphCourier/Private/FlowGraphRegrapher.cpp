// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowGraphRegrapher.h"
#include "FlowLogChannels.h"
#include "FlowAsset.h"
#include "FlowGraphImporter.h"
#include "AddOns/FlowNodeAddOn.h"
#include "AddOns/FlowNodeAddOn_SwitchCase.h"
#include "Nodes/FlowNode.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "Editor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"

namespace FlowGraphRegrapherDetail
{
	TArray<FFlowPin> CollectRequiredEditorPins(const TArray<FFlowPin>& StaticPins, const TArray<FFlowPin>& ContextPins)
	{
		TArray<FFlowPin> Pins = StaticPins;
		for (const FFlowPin& ContextPin : ContextPins)
		{
			if (!Pins.ContainsByPredicate([&ContextPin](const FFlowPin& Pin)
			{
				return Pin.PinName == ContextPin.PinName;
			}))
			{
				Pins.Add(ContextPin);
			}
		}
		return Pins;
	}

	void BreakPinLinksWithoutNotification(UEdGraphPin& Pin)
	{
		const TArray<UEdGraphPin*> LinkedPins = Pin.LinkedTo;
		for (UEdGraphPin* const LinkedPin : LinkedPins)
		{
			if (LinkedPin != nullptr)
			{
				LinkedPin->LinkedTo.RemoveSingle(&Pin);
			}
		}
		Pin.LinkedTo.Reset();
	}

	void BreakNodeLinksWithoutNotification(UFlowGraphNode& GraphNode)
	{
		for (UEdGraphPin* const Pin : GraphNode.Pins)
		{
			if (Pin != nullptr)
			{
				BreakPinLinksWithoutNotification(*Pin);
			}
		}
	}

	void LinkPinsWithoutNotification(UEdGraphPin& First, UEdGraphPin& Second)
	{
		First.LinkedTo.AddUnique(&Second);
		Second.LinkedTo.AddUnique(&First);
	}

	bool ValidateRuntimeAddOnTree(const UFlowAsset& FlowAsset, const UFlowNodeBase& RuntimeParent,
		TSet<const UFlowNodeAddOn*>& ActiveAddOns, TSet<const UFlowNodeAddOn*>& VisitedAddOns)
	{
		const TArray<UFlowNodeAddOn*>& RuntimeAddOns = RuntimeParent.GetFlowNodeAddOnChildren();
		for (int32 AddOnIndex = 0; AddOnIndex < RuntimeAddOns.Num(); ++AddOnIndex)
		{
			const UFlowNodeAddOn* const RuntimeAddOn = RuntimeAddOns[AddOnIndex];
			if (!IsValid(RuntimeAddOn))
			{
				UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Runtime AddOn index %d under '%s' is invalid in asset '%s'"),
					AddOnIndex, *RuntimeParent.GetPathName(), *FlowAsset.GetPathName());
				return false;
			}
			const TSubclassOf<UEdGraphNode> GraphNodeClass = UFlowGraphSchema::GetAssignedGraphNodeClass(RuntimeAddOn->GetClass());
			if (!GraphNodeClass)
			{
				UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Runtime AddOn '%s' of class '%s' has no assigned graph node class in asset '%s'"),
					*RuntimeAddOn->GetPathName(), *RuntimeAddOn->GetClass()->GetPathName(), *FlowAsset.GetPathName());
				return false;
			}
			if (!GraphNodeClass->IsChildOf(UFlowGraphNode::StaticClass()))
			{
				UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Runtime AddOn '%s' resolves incompatible graph node class '%s' in asset '%s'"),
					*RuntimeAddOn->GetPathName(), *GraphNodeClass->GetPathName(), *FlowAsset.GetPathName());
				return false;
			}
			if (ActiveAddOns.Contains(RuntimeAddOn))
			{
				UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Recursive runtime AddOn reference at '%s' in asset '%s'"),
					*RuntimeAddOn->GetPathName(), *FlowAsset.GetPathName());
				return false;
			}
			if (VisitedAddOns.Contains(RuntimeAddOn))
			{
				UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Runtime AddOn '%s' has more than one parent in asset '%s'"),
					*RuntimeAddOn->GetPathName(), *FlowAsset.GetPathName());
				return false;
			}

			ActiveAddOns.Add(RuntimeAddOn);
			if (!ValidateRuntimeAddOnTree(FlowAsset, *RuntimeAddOn, ActiveAddOns, VisitedAddOns))
			{
				return false;
			}
			ActiveAddOns.Remove(RuntimeAddOn);
			VisitedAddOns.Add(RuntimeAddOn);
		}
		return true;
	}

	bool ValidateRuntimeGraphForRegraph(const UFlowAsset& FlowAsset)
	{
		TSet<const UFlowNodeAddOn*> ActiveAddOns;
		TSet<const UFlowNodeAddOn*> VisitedAddOns;
		for (const TPair<FGuid, UFlowNode*>& NodePair : FlowAsset.GetNodes())
		{
			if (!IsValid(NodePair.Value))
			{
				UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Runtime node map entry '%s' is invalid in asset '%s'"),
					*NodePair.Key.ToString(), *FlowAsset.GetPathName());
				return false;
			}
			if (!ValidateRuntimeAddOnTree(FlowAsset, *NodePair.Value, ActiveAddOns, VisitedAddOns))
			{
				return false;
			}
		}
		return true;
	}

	void CacheFlowNodeOwnerForAddOns(UFlowNodeBase& RuntimeParent, UFlowNode& FlowNodeOwner,
		TSet<UFlowNodeAddOn*>& ActiveAddOns, const bool bLogOwnerMismatch)
	{
		for (UFlowNodeAddOn* const RuntimeAddOn : RuntimeParent.GetFlowNodeAddOnChildren())
		{
			if (!IsValid(RuntimeAddOn) || ActiveAddOns.Contains(RuntimeAddOn))
			{
				continue;
			}
			UFlowNode* const OuterOwner = RuntimeAddOn->FindOwningFlowNode();
			if (bLogOwnerMismatch && RuntimeAddOn->IsA<UFlowNodeAddOn_SwitchCase>() && OuterOwner != &FlowNodeOwner)
			{
				UE_LOG(LogFlow, Warning,
					TEXT("Flow Courier SwitchCase owner mismatch: AddOn '%s', Guid '%s', Outer '%s', ")
					TEXT("OuterOwner '%s', CachedOwner '%s', AttachedParent '%s', AttachedRoot '%s'"),
					*RuntimeAddOn->GetPathName(), *RuntimeAddOn->GetGuid().ToString(),
					*GetNameSafe(RuntimeAddOn->GetOuter()), *GetNameSafe(OuterOwner),
					*GetNameSafe(RuntimeAddOn->GetFlowNodeSelfOrOwner()),
					*RuntimeParent.GetPathName(), *FlowNodeOwner.GetPathName());
			}
			RuntimeAddOn->SetFlowNodeForEditor(&FlowNodeOwner);
			ActiveAddOns.Add(RuntimeAddOn);
			CacheFlowNodeOwnerForAddOns(*RuntimeAddOn, FlowNodeOwner, ActiveAddOns, bLogOwnerMismatch);
			ActiveAddOns.Remove(RuntimeAddOn);
		}
	}

	void CacheFlowNodeOwnersForContextPins(const UFlowAsset& FlowAsset, const bool bLogOwnerMismatch = false)
	{
		TSet<UFlowNodeAddOn*> ActiveAddOns;
		for (const TPair<FGuid, UFlowNode*>& NodePair : FlowAsset.GetNodes())
		{
			if (IsValid(NodePair.Value))
			{
				CacheFlowNodeOwnerForAddOns(*NodePair.Value, *NodePair.Value, ActiveAddOns, bLogOwnerMismatch);
			}
		}
	}

	bool ValidateRuntimePinMetadata(const UFlowAsset& FlowAsset)
	{
		for (const TPair<FGuid, UFlowNode*>& NodePair : FlowAsset.GetNodes())
		{
			TArray<FFlowPin> Pins = CollectRequiredEditorPins(
				NodePair.Value->GetInputPins(), NodePair.Value->GetContextInputs());
			Pins.Append(CollectRequiredEditorPins(
				NodePair.Value->GetOutputPins(), NodePair.Value->GetContextOutputs()));
			for (const FFlowPin& Pin : Pins)
			{
				if (Pin.PinName.IsNone() || Pin.BuildEdGraphPinType().PinCategory.IsNone())
				{
					UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Runtime node '%s' has invalid editor pin metadata in asset '%s'"),
						*NodePair.Value->GetPathName(), *FlowAsset.GetPathName());
					return false;
				}
			}
		}
		return true;
	}

	bool ContainsPin(const TArray<FFlowPin>& Pins, const FName PinName, const bool bExecPin)
	{
		return Pins.ContainsByPredicate([PinName, bExecPin](const FFlowPin& Pin)
		{
			return Pin.PinName == PinName && Pin.IsExecPin() == bExecPin;
		});
	}

	bool HasExistingEditorLink(const UFlowNode& RuntimeNode, const FName PinName,
		const EEdGraphPinDirection Direction)
	{
		const UFlowGraphNode* const GraphNode = Cast<UFlowGraphNode>(RuntimeNode.GetGraphNode());
		const UEdGraphPin* const Pin = IsValid(GraphNode) ? GraphNode->FindPin(PinName, Direction) : nullptr;
		return Pin != nullptr && !Pin->LinkedTo.IsEmpty();
	}

	bool ValidateRuntimeConnectionsForRegraph(const UFlowAsset& FlowAsset)
	{
		const TMap<FGuid, UFlowNode*>& RuntimeNodes = FlowAsset.GetNodes();
		for (const TPair<FGuid, UFlowNode*>& NodePair : RuntimeNodes)
		{
			const UFlowNode& RuntimeNode = *NodePair.Value;
			const TArray<FFlowPin> OutputPins = CollectRequiredEditorPins(
				RuntimeNode.GetOutputPins(), RuntimeNode.GetContextOutputs());
			for (const FFlowPin& OutputPin : OutputPins)
			{
				if (!OutputPin.IsExecPin())
				{
					continue;
				}
				const FConnectedPin Connection = RuntimeNode.GetConnection(OutputPin.PinName);
				if (!Connection.NodeGuid.IsValid())
				{
					continue;
				}
				const UFlowNode* const TargetNode = RuntimeNodes.FindRef(Connection.NodeGuid);
				if (!IsValid(TargetNode))
				{
					if (HasExistingEditorLink(RuntimeNode, OutputPin.PinName, EGPD_Output))
					{
						continue;
					}
					UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Exec connection '%s.%s' has no runtime target or recoverable editor link in asset '%s'"),
						*NodePair.Key.ToString(), *OutputPin.PinName.ToString(), *FlowAsset.GetPathName());
					return false;
				}
				const TArray<FFlowPin> TargetInputs = CollectRequiredEditorPins(
					TargetNode->GetInputPins(), TargetNode->GetContextInputs());
				if (!ContainsPin(TargetInputs, Connection.PinName, true))
				{
					UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Exec connection '%s.%s' targets missing exec input '%s.%s' in asset '%s'"),
						*NodePair.Key.ToString(), *OutputPin.PinName.ToString(), *Connection.NodeGuid.ToString(),
						*Connection.PinName.ToString(), *FlowAsset.GetPathName());
					return false;
				}
			}

			const TArray<FFlowPin> InputPins = CollectRequiredEditorPins(
				RuntimeNode.GetInputPins(), RuntimeNode.GetContextInputs());
			for (const FFlowPin& InputPin : InputPins)
			{
				if (InputPin.IsExecPin())
				{
					continue;
				}
				const FConnectedPin Connection = RuntimeNode.GetConnection(InputPin.PinName);
				if (!Connection.NodeGuid.IsValid())
				{
					continue;
				}
				const UFlowNode* const SourceNode = RuntimeNodes.FindRef(Connection.NodeGuid);
				if (!IsValid(SourceNode))
				{
					if (HasExistingEditorLink(RuntimeNode, InputPin.PinName, EGPD_Input))
					{
						continue;
					}
					UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Data connection '%s.%s' has no runtime source or recoverable editor link in asset '%s'"),
						*NodePair.Key.ToString(), *InputPin.PinName.ToString(), *FlowAsset.GetPathName());
					return false;
				}
				const TArray<FFlowPin> SourceOutputs = CollectRequiredEditorPins(
					SourceNode->GetOutputPins(), SourceNode->GetContextOutputs());
				if (!ContainsPin(SourceOutputs, Connection.PinName, false))
				{
					UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Data connection '%s.%s' references missing data output '%s.%s' in asset '%s'"),
						*NodePair.Key.ToString(), *InputPin.PinName.ToString(), *Connection.NodeGuid.ToString(),
						*Connection.PinName.ToString(), *FlowAsset.GetPathName());
					return false;
				}
			}
		}
		return true;
	}

	bool ReconcileTopLevelEditorNodes(UFlowAsset& FlowAsset, UFlowGraph& FlowGraph,
		const TMap<FGuid, FIntPoint>& NodePositions, const TMap<FGuid, FString>& NodeComments)
	{
		for (const TPair<FGuid, UFlowNode*>& NodePair : FlowAsset.GetNodes())
		{
			UFlowNode* const RuntimeNode = NodePair.Value;

			// Check if an editor node already exists for this runtime node
			UFlowGraphNode* ExistingGraphNode = nullptr;
			for (UEdGraphNode* const EditorNode : FlowGraph.Nodes)
			{
				UFlowGraphNode* const FlowGraphNode = Cast<UFlowGraphNode>(EditorNode);
				const UFlowNode* const EditorRuntimeNode = Cast<UFlowNode>(FlowGraphNode ? FlowGraphNode->GetFlowNodeBase() : nullptr);
				if (EditorRuntimeNode && EditorRuntimeNode->GetGuid() == RuntimeNode->GetGuid())
				{
					ExistingGraphNode = FlowGraphNode;
					break;
				}
			}
			if (ExistingGraphNode)
			{
				RuntimeNode->SetGraphNode(ExistingGraphNode);
				ExistingGraphNode->SetNodeTemplate(RuntimeNode);

#if WITH_EDITOR
				if (const FString* const Comment = NodeComments.Find(RuntimeNode->GetGuid()))
				{
					ExistingGraphNode->NodeComment = *Comment;
				}
#endif
				continue;
			}

			// Create a new UFlowGraphNode
			FGraphNodeCreator<UFlowGraphNode> NodeCreator(FlowGraph);
			UFlowGraphNode* const FlowGraphNode = NodeCreator.CreateNode();
			if (!FlowGraphNode)
			{
				UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Failed to create editor node for runtime node '%s' in asset '%s'"),
					*RuntimeNode->GetPathName(), *FlowAsset.GetPathName());
				return false;
			}

			// NOTE - SetNodeTemplate (below) only sets NodeInstance on the editor node; it does
			// NOT call SetGraphNode on the runtime node. Without this call here, any full-graph
			// HarvestNodeConnections triggered before RegraphFlowAsset finishes (e.g. from
			// CreateDefaultNodesForGraph) will crash on imported nodes whose GraphNode is null.
			RuntimeNode->SetGraphNode(FlowGraphNode);
			FlowGraphNode->SetNodeTemplate(RuntimeNode);
			FlowGraphNode->AllocateDefaultPins();
			if (const FIntPoint* const Position = NodePositions.Find(RuntimeNode->GetGuid()))
			{
				FlowGraphNode->NodePosX = Position->X;
				FlowGraphNode->NodePosY = Position->Y;
			}
			// Finalize() calls Node->CreateNewGuid() internally, assigning a random
			// UEdGraphNode::NodeGuid unrelated to RuntimeNode->GetGuid() - so the real
			// assignment below must happen AFTER Finalize(), not before, or it gets clobbered.
			NodeCreator.Finalize();

			// UFlowAsset::HarvestNodeConnections (run at the end of every regraph) rebuilds the
			// runtime Connections map from the editor graph's pin links using exactly this
			// NodeGuid (FlowAsset.cpp's LinkedNode->NodeGuid) - without this line, every
			// connection this node is a target of gets silently corrupted with a bogus GUID on
			// the very next regraph. Mirrors the same assignment already done for root nodes in
			// FFlowGraphSchemaAction_NewNode::CreateNode/ImportNode (which don't go through
			// FGraphNodeCreator and so don't have this clobbering problem).
			FlowGraphNode->NodeGuid = RuntimeNode->GetGuid();
#if WITH_EDITOR
			if (const FString* const Comment = NodeComments.Find(RuntimeNode->GetGuid()))
			{
				FlowGraphNode->NodeComment = *Comment;
			}
#endif
		}
		return true;
	}

	void CollectEditorAddOnWrappers(UFlowGraphNode& ParentGraphNode, TArray<UFlowGraphNode*>& OutWrappers,
		TSet<UFlowGraphNode*>& VisitedWrappers)
	{
		for (UFlowGraphNode* const SubNode : ParentGraphNode.SubNodes)
		{
			if (!IsValid(SubNode) || VisitedWrappers.Contains(SubNode))
			{
				continue;
			}
			VisitedWrappers.Add(SubNode);
			OutWrappers.Add(SubNode);
			CollectEditorAddOnWrappers(*SubNode, OutWrappers, VisitedWrappers);
		}
	}

	void DiscardEditorAddOnWrapper(UFlowGraphNode* AddOnGraphNode, TSet<UFlowGraphNode*>& DiscardedWrappers)
	{
		if (!IsValid(AddOnGraphNode) || DiscardedWrappers.Contains(AddOnGraphNode))
		{
			return;
		}
		DiscardedWrappers.Add(AddOnGraphNode);
		const TArray<TObjectPtr<UFlowGraphNode>> Children = AddOnGraphNode->SubNodes;
		for (UFlowGraphNode* const Child : Children)
		{
			DiscardEditorAddOnWrapper(Child, DiscardedWrappers);
		}
		AddOnGraphNode->SubNodes.Reset();
		if (UFlowGraphNode* const ParentGraphNode = AddOnGraphNode->GetParentNode())
		{
			ParentGraphNode->RemoveSubNodeForRegraph(AddOnGraphNode);
		}
		if (UFlowNodeAddOn* const RuntimeAddOn = Cast<UFlowNodeAddOn>(AddOnGraphNode->GetFlowNodeBase()))
		{
			if (RuntimeAddOn->GetGraphNode() == AddOnGraphNode)
			{
				RuntimeAddOn->SetGraphNode(nullptr);
			}
		}
		BreakNodeLinksWithoutNotification(*AddOnGraphNode);
		AddOnGraphNode->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
		AddOnGraphNode->MarkAsGarbage();
	}

	bool IsUsableAddOnWrapper(const UFlowGraphNode* Wrapper, const UFlowNodeAddOn& RuntimeAddOn,
		const UFlowGraph& FlowGraph, const TSubclassOf<UEdGraphNode>& GraphNodeClass,
		const TSet<UFlowGraphNode*>& RetainedWrappers)
	{
		return IsValid(Wrapper) && Wrapper->GetOuter() == &FlowGraph
			&& Wrapper->GetFlowNodeBase() == &RuntimeAddOn
			&& Wrapper->GetClass() == GraphNodeClass.Get() && !RetainedWrappers.Contains(Wrapper);
	}

	bool ReconcileRuntimeAddOnChildren(UFlowGraph& FlowGraph, UFlowGraphNode& ParentGraphNode,
		UFlowNodeBase& RuntimeParent, const TMap<UFlowNodeAddOn*, UFlowGraphNode*>& ExistingWrapperByAddOn,
		TSet<UFlowGraphNode*>& RetainedWrappers, TSet<UFlowNodeAddOn*>& ActiveAddOns)
	{
		TArray<TObjectPtr<UFlowGraphNode>> OrderedSubNodes;
		OrderedSubNodes.Reserve(RuntimeParent.GetFlowNodeAddOnChildren().Num());
		for (UFlowNodeAddOn* const RuntimeAddOn : RuntimeParent.GetFlowNodeAddOnChildren())
		{
			if (ActiveAddOns.Contains(RuntimeAddOn))
			{
				continue;
			}

			const TSubclassOf<UEdGraphNode> GraphNodeClass =
				UFlowGraphSchema::GetAssignedGraphNodeClass(RuntimeAddOn->GetClass());
			UFlowGraphNode* AddOnGraphNode = Cast<UFlowGraphNode>(RuntimeAddOn->GetGraphNode());
			if (!IsUsableAddOnWrapper(AddOnGraphNode, *RuntimeAddOn, FlowGraph,
				GraphNodeClass, RetainedWrappers))
			{
				AddOnGraphNode = ExistingWrapperByAddOn.FindRef(RuntimeAddOn);
			}
			if (!IsUsableAddOnWrapper(AddOnGraphNode, *RuntimeAddOn, FlowGraph,
				GraphNodeClass, RetainedWrappers))
			{
				AddOnGraphNode = NewObject<UFlowGraphNode>(&FlowGraph, GraphNodeClass, NAME_None, RF_Transactional);
				if (!IsValid(AddOnGraphNode))
				{
					UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Failed to create editor AddOn wrapper for '%s' in asset '%s'"),
						*RuntimeAddOn->GetPathName(), *FlowGraph.GetFlowAsset()->GetPathName());
					return false;
				}
				RuntimeAddOn->SetGraphNode(AddOnGraphNode);
				AddOnGraphNode->SetNodeTemplate(RuntimeAddOn);
				if (!ParentGraphNode.AddSubNodeForRegraph(AddOnGraphNode, &FlowGraph))
				{
					UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Failed to attach editor AddOn wrapper for '%s' in asset '%s'"),
						*RuntimeAddOn->GetPathName(), *FlowGraph.GetFlowAsset()->GetPathName());
					return false;
				}
			}
			else
			{
				RuntimeAddOn->SetGraphNode(AddOnGraphNode);
				AddOnGraphNode->SetNodeTemplate(RuntimeAddOn);
				ParentGraphNode.ReparentSubNodeForRegraph(AddOnGraphNode);
			}

			RetainedWrappers.Add(AddOnGraphNode);
			OrderedSubNodes.Add(AddOnGraphNode);
			ActiveAddOns.Add(RuntimeAddOn);
			if (!ReconcileRuntimeAddOnChildren(FlowGraph, *AddOnGraphNode, *RuntimeAddOn,
				ExistingWrapperByAddOn, RetainedWrappers, ActiveAddOns))
			{
				return false;
			}
			ActiveAddOns.Remove(RuntimeAddOn);
		}
		ParentGraphNode.SubNodes = MoveTemp(OrderedSubNodes);
		return true;
	}

	bool ReconcileEditorAddOnTrees(UFlowAsset& FlowAsset, UFlowGraph& FlowGraph)
	{
		TArray<UFlowGraphNode*> ExistingWrappers;
		TSet<UFlowGraphNode*> VisitedWrappers;
		for (UEdGraphNode* const EditorNode : FlowGraph.Nodes)
		{
			if (UFlowGraphNode* const FlowGraphNode = Cast<UFlowGraphNode>(EditorNode))
			{
				CollectEditorAddOnWrappers(*FlowGraphNode, ExistingWrappers, VisitedWrappers);
			}
		}

		TMap<UFlowNodeAddOn*, UFlowGraphNode*> ExistingWrapperByAddOn;
		for (UFlowGraphNode* const ExistingWrapper : ExistingWrappers)
		{
			if (UFlowNodeAddOn* const RuntimeAddOn = Cast<UFlowNodeAddOn>(ExistingWrapper->GetFlowNodeBase()))
			{
				if (!ExistingWrapperByAddOn.Contains(RuntimeAddOn))
				{
					ExistingWrapperByAddOn.Add(RuntimeAddOn, ExistingWrapper);
				}
			}
		}

		TSet<UFlowGraphNode*> RetainedWrappers;
		TSet<UFlowNodeAddOn*> ActiveAddOns;
		for (const TPair<FGuid, UFlowNode*>& NodePair : FlowAsset.GetNodes())
		{
			UFlowGraphNode* const ParentGraphNode = Cast<UFlowGraphNode>(NodePair.Value->GetGraphNode());
			if (!IsValid(ParentGraphNode))
			{
				UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Runtime node '%s' has no editor node while reconciling AddOns in asset '%s'"),
					*NodePair.Value->GetPathName(), *FlowAsset.GetPathName());
				return false;
			}
			if (!ReconcileRuntimeAddOnChildren(FlowGraph, *ParentGraphNode, *NodePair.Value,
				ExistingWrapperByAddOn, RetainedWrappers, ActiveAddOns))
			{
				return false;
			}
		}

		TSet<UFlowGraphNode*> DiscardedWrappers;
		for (UFlowGraphNode* const ExistingWrapper : ExistingWrappers)
		{
			if (!RetainedWrappers.Contains(ExistingWrapper))
			{
				DiscardEditorAddOnWrapper(ExistingWrapper, DiscardedWrappers);
			}
		}
		return true;
	}

	bool AbortLockedRegraph(UFlowGraph& FlowGraph)
	{
		FlowGraph.UnlockUpdatesWithoutReconcile();
		return false;
	}

	bool CreateRequiredEditorPins(UFlowAsset& FlowAsset);
	bool ReconcileEditorPinConnections(UFlowAsset& FlowAsset, UFlowGraph& FlowGraph);
}

UFlowAsset* UFlowGraphRegrapher::ImportAndRegraphFromDocument(
	const FString& AssetPath,
	const FString& AssetClassPath,
	bool bWorldBound,
	const TArray<FFlowGraphParsedNode>& ParsedNodes,
	const TArray<FFlowGraphParsedConnection>& ParsedConnections)
{
	if (AssetPath.IsEmpty())
	{
		UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::ImportAndRegraphFromDocument: AssetPath is empty"));
		return nullptr;
	}

	FTopLevelAssetPath ParsedPath(AssetPath);
	if (ParsedPath.GetAssetName().IsNone())
	{
		ParsedPath.TrySetPath(*AssetPath, FPackageName::GetShortFName(AssetPath));
	}

	if (ParsedPath.GetAssetName().IsNone())
	{
		UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::ImportAndRegraphFromDocument: Failed to parse AssetName from path %s"), *AssetPath);
		return nullptr;
	}

	FString ErrorMessage;
	UFlowAsset* FlowAsset = UFlowGraphImporter::ImportFlowGraphFromDocument(AssetPath, AssetClassPath, bWorldBound, ParsedNodes, ParsedConnections, ErrorMessage);

	if (!FlowAsset)
	{
		UE_LOG(LogFlow, Warning, TEXT("UFlowGraphRegrapher::ImportAndRegraphFromDocument: Import failed: %s"), *ErrorMessage);
		return nullptr;
	}

	// Restore editor positions and comments for nodes that carried Position/Comment fields.
	// Nodes without one are left at origin; the caller may run auto-placement separately.
	TMap<FGuid, FIntPoint> NodePositions;
	TMap<FGuid, FString> NodeComments;
	for (const FFlowGraphParsedNode& ParsedNode : ParsedNodes)
	{
		if (ParsedNode.bIsDeleteMarker)
		{
			continue;
		}
		if (ParsedNode.bHasPos)
		{
			NodePositions.Add(ParsedNode.NodeGuid, ParsedNode.Pos);
		}
		if (ParsedNode.bHasComment)
		{
			NodeComments.Add(ParsedNode.NodeGuid, ParsedNode.NodeComment);
		}
	}

	if (!RegraphFlowAsset(FlowAsset, NodePositions, NodeComments))
	{
		UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::ImportAndRegraphFromDocument: Regraph failed for asset: %s"), *AssetPath);
		return nullptr;
	}

	return FlowAsset;
}

bool UFlowGraphRegrapher::RegraphFlowAsset(UFlowAsset* FlowAsset)
{
	static const TMap<FGuid, FIntPoint> NoPositions;
	return RegraphFlowAsset(FlowAsset, NoPositions);
}

bool UFlowGraphRegrapher::RegraphFlowAsset(UFlowAsset* FlowAsset, const TMap<FGuid, FIntPoint>& NodePositions)
{
	static const TMap<FGuid, FString> NoComments;
	return RegraphFlowAsset(FlowAsset, NodePositions, NoComments);
}

bool UFlowGraphRegrapher::RegraphFlowAsset(UFlowAsset* FlowAsset, const TMap<FGuid, FIntPoint>& NodePositions, const TMap<FGuid, FString>& NodeComments)
{
	if (!FlowAsset)
	{
		UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: FlowAsset is null"));
		return false;
	}
	if (!GEditor)
	{
		UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: GEditor is not available"));
		return false;
	}
	if (GEditor->IsPlaySessionInProgress())
	{
		UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Cannot regraph during PIE"));
		return false;
	}
	if (!FlowGraphRegrapherDetail::ValidateRuntimeGraphForRegraph(*FlowAsset))
	{
		return false;
	}

	FlowGraphRegrapherDetail::CacheFlowNodeOwnersForContextPins(*FlowAsset);
	if (!FlowGraphRegrapherDetail::ValidateRuntimePinMetadata(*FlowAsset))
	{
		return false;
	}
	if (!FlowGraphRegrapherDetail::ValidateRuntimeConnectionsForRegraph(*FlowAsset))
	{
		return false;
	}

	// Ensure the FlowAsset has a UFlowGraph. If the asset already has real runtime nodes (e.g. from
	// a Flow Courier import/reconcile), skip default node seeding - CreateDefaultNodesForGraph is only
	// appropriate for a brand new, genuinely empty FlowAsset and would otherwise inject an unwanted
	// extra Start node into the runtime Nodes map on this first regraph.
	UFlowGraph* FlowGraph = Cast<UFlowGraph>(FlowAsset->GetGraph());
	if (!FlowGraph)
	{
		const bool bCreateDefaultNodes = FlowAsset->GetNodes().IsEmpty();
		UFlowGraph::CreateGraph(FlowAsset, UFlowGraphSchema::StaticClass(), bCreateDefaultNodes);
		FlowGraph = Cast<UFlowGraph>(FlowAsset->GetGraph());
		if (!FlowGraph)
		{
			UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Failed to create editor graph for asset '%s'"),
				*FlowAsset->GetPathName());
			return false;
		}
	}

	if (FlowGraph->IsLocked())
	{
		UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Editor graph is already locked for asset '%s'"),
			*FlowAsset->GetPathName());
		return false;
	}
	FlowGraph->LockUpdates();

	// Before anything is wired: drop editor nodes whose runtime node is gone. Their pin links are
	// still live, and CreateEditorPinConnections' HarvestNodeConnections would otherwise harvest
	// them straight back into the runtime Connections map, resurrecting connections to a node that
	// no longer exists.
	PruneOrphanedEditorNodes(FlowAsset);
	if (!FlowGraphRegrapherDetail::ReconcileTopLevelEditorNodes(*FlowAsset, *FlowGraph, NodePositions, NodeComments))
	{
		return FlowGraphRegrapherDetail::AbortLockedRegraph(*FlowGraph);
	}
	if (!FlowGraphRegrapherDetail::ReconcileEditorAddOnTrees(*FlowAsset, *FlowGraph))
	{
		return FlowGraphRegrapherDetail::AbortLockedRegraph(*FlowGraph);
	}
	if (!FlowGraphRegrapherDetail::CreateRequiredEditorPins(*FlowAsset))
	{
		return FlowGraphRegrapherDetail::AbortLockedRegraph(*FlowGraph);
	}
	if (!FlowGraphRegrapherDetail::ReconcileEditorPinConnections(*FlowAsset, *FlowGraph))
	{
		return FlowGraphRegrapherDetail::AbortLockedRegraph(*FlowGraph);
	}

	FlowGraph->UnlockUpdatesWithoutReconcile();
	FlowGraph->NotifyGraphChanged();
	return true;
}

namespace FlowGraphRegrapherDetail
{
	// Returns the top-level editor node representing RuntimeGuid, or null. Only nodes standing in
	// for a UFlowNode are considered: an addon sub-node's UFlowNodeAddOn and a comment node have no
	// entry in UFlowAsset::Nodes by design, and neither is an orphan.
	UFlowGraphNode* FindTopLevelEditorNodeForGuid(const UFlowGraph& FlowGraph, const FGuid& RuntimeGuid)
	{
		for (UEdGraphNode* EdNode : FlowGraph.Nodes)
		{
			UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(EdNode);
			if (!FlowGraphNode || FlowGraphNode->GetParentNode())
			{
				continue;
			}

			const UFlowNode* EdFlowNode = Cast<UFlowNode>(FlowGraphNode->GetFlowNodeBase());
			if (EdFlowNode && EdFlowNode->GetGuid() == RuntimeGuid)
			{
				return FlowGraphNode;
			}
		}

		return nullptr;
	}

	// Renders a connection as an order-independent key, so the same wire produces the same string
	// whichever endpoint it was discovered from.
	FString MakeWireKey(const FGuid& NodeGuidA, const FName PinNameA, const FGuid& NodeGuidB, const FName PinNameB)
	{
		const FString EndpointA = FString::Printf(TEXT("%s.%s"), *NodeGuidA.ToString(), *PinNameA.ToString());
		const FString EndpointB = FString::Printf(TEXT("%s.%s"), *NodeGuidB.ToString(), *PinNameB.ToString());
		return EndpointA <= EndpointB
			? FString::Printf(TEXT("%s <-> %s"), *EndpointA, *EndpointB)
			: FString::Printf(TEXT("%s <-> %s"), *EndpointB, *EndpointA);
	}

	// Breaks every link then destroys the node. Links must go first: DestroyNode alone leaves the
	// pins of the nodes on the other end still pointing at it.
	void BreakLinksAndDestroy(UFlowGraphNode& FlowGraphNode)
	{
		BreakNodeLinksWithoutNotification(FlowGraphNode);
		FlowGraphNode.DestroyNode();
	}
}

bool UFlowGraphRegrapher::DestroyEditorNodeForRuntimeNode(UFlowAsset* FlowAsset, const FGuid& NodeGuid)
{
	UFlowGraph* FlowGraph = FlowAsset ? Cast<UFlowGraph>(FlowAsset->GetGraph()) : nullptr;
	if (!FlowGraph)
	{
		// An asset built purely programmatically may never have had an editor graph; there is no
		// editor node to destroy and nothing is wrong.
		return false;
	}

	UFlowGraphNode* FlowGraphNode = FlowGraphRegrapherDetail::FindTopLevelEditorNodeForGuid(*FlowGraph, NodeGuid);
	if (!FlowGraphNode)
	{
		return false;
	}

	FlowGraph->Modify();
	FlowGraphRegrapherDetail::BreakLinksAndDestroy(*FlowGraphNode);
	return true;
}

int32 UFlowGraphRegrapher::PruneOrphanedEditorNodes(UFlowAsset* FlowAsset)
{
	UFlowGraph* FlowGraph = FlowAsset ? Cast<UFlowGraph>(FlowAsset->GetGraph()) : nullptr;
	if (!FlowGraph)
	{
		return 0;
	}

	const TMap<FGuid, UFlowNode*>& RuntimeNodes = FlowAsset->GetNodes();

	// Collect first: DestroyNode removes the entry from FlowGraph->Nodes, so the array cannot be
	// mutated while it is being walked.
	TArray<UFlowGraphNode*> OrphanedNodes;
	for (UEdGraphNode* EdNode : FlowGraph->Nodes)
	{
		UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(EdNode);
		if (!FlowGraphNode || FlowGraphNode->GetParentNode())
		{
			continue;
		}

		// A null instance is left alone deliberately: the regraph's own creation loop links a
		// freshly-created editor node to its runtime node only after FGraphNodeCreator finalizes it,
		// so a momentarily instance-less node is not evidence of an orphan.
		const UFlowNode* EdFlowNode = Cast<UFlowNode>(FlowGraphNode->GetFlowNodeBase());
		if (!EdFlowNode)
		{
			continue;
		}

		if (!RuntimeNodes.Contains(EdFlowNode->GetGuid()))
		{
			OrphanedNodes.Add(FlowGraphNode);
		}
	}

	if (OrphanedNodes.IsEmpty())
	{
		return 0;
	}

	FlowGraph->Modify();
	for (UFlowGraphNode* OrphanedNode : OrphanedNodes)
	{
		UE_LOG(LogFlow, Warning, TEXT("UFlowGraphRegrapher::PruneOrphanedEditorNodes: destroying editor node '%s' - its runtime node is not registered on '%s'"),
			*OrphanedNode->GetName(), *FlowAsset->GetPathName());
		FlowGraphRegrapherDetail::BreakLinksAndDestroy(*OrphanedNode);
	}
	if (!FlowGraph->IsLocked())
	{
		FlowAsset->HarvestNodeConnections();
	}

	return OrphanedNodes.Num();
}

void UFlowGraphRegrapher::CollectGraphParityIssues(const UFlowAsset* FlowAsset, TArray<FString>& OutIssues)
{
	OutIssues.Reset();
	if (!FlowAsset)
	{
		OutIssues.Add(TEXT("FlowAsset is null"));
		return;
	}

	FlowGraphRegrapherDetail::CacheFlowNodeOwnersForContextPins(*FlowAsset, true);
	const TMap<FGuid, UFlowNode*>& RuntimeNodes = FlowAsset->GetNodes();

	const UFlowGraph* FlowGraph = Cast<UFlowGraph>(FlowAsset->GetGraph());
	if (FlowGraph)
	{
		TSet<FGuid> EditorNodeGuids;
		EditorNodeGuids.Reserve(FlowGraph->Nodes.Num());

		for (const UEdGraphNode* EdNode : FlowGraph->Nodes)
		{
			const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(EdNode);
			if (!FlowGraphNode || FlowGraphNode->GetParentNode())
			{
				continue;
			}

			const UFlowNode* EdFlowNode = Cast<UFlowNode>(FlowGraphNode->GetFlowNodeBase());
			if (!EdFlowNode)
			{
				continue;
			}

			const FGuid EditorNodeGuid = EdFlowNode->GetGuid();
			bool bAlreadySeen = false;
			EditorNodeGuids.Add(EditorNodeGuid, &bAlreadySeen);
			if (bAlreadySeen)
			{
				OutIssues.Add(FString::Printf(TEXT("Two editor nodes represent the same runtime node %s (%s)"),
					*EditorNodeGuid.ToString(), *EdFlowNode->GetClass()->GetName()));
			}

			if (!RuntimeNodes.Contains(EditorNodeGuid))
			{
				OutIssues.Add(FString::Printf(TEXT("Editor node %s (%s) has no runtime node - it is drawn but cannot execute"),
					*EditorNodeGuid.ToString(), *EdFlowNode->GetClass()->GetName()));
			}
		}

		for (const TPair<FGuid, UFlowNode*>& NodePair : RuntimeNodes)
		{
			if (NodePair.Value && !EditorNodeGuids.Contains(NodePair.Key))
			{
				OutIssues.Add(FString::Printf(TEXT("Runtime node %s (%s) has no editor node - it is invisible in the graph editor"),
					*NodePair.Key.ToString(), *NodePair.Value->GetClass()->GetName()));
			}
		}
	}
	else if (!RuntimeNodes.IsEmpty())
	{
		OutIssues.Add(TEXT("The asset has runtime nodes but no editor graph"));
	}

	// Runtime wires, as unordered endpoint pairs. Unordered is deliberate: Flow stores an exec
	// connection on the source node keyed by its output pin, and a data connection on the target
	// node keyed by its input pin. Comparing unordered pairs means this check never has to
	// re-derive that rule, so it cannot disagree with the code that writes it.
	TSet<FString> RuntimeWires;
	for (const TPair<FGuid, UFlowNode*>& NodePair : RuntimeNodes)
	{
		const UFlowNode* Node = NodePair.Value;
		if (!Node)
		{
			OutIssues.Add(FString::Printf(TEXT("Runtime node map entry %s is null"), *NodePair.Key.ToString()));
			continue;
		}

		TArray<FFlowPin> AllPins = FlowGraphRegrapherDetail::CollectRequiredEditorPins(
			Node->GetInputPins(), Node->GetContextInputs());
		AllPins.Append(FlowGraphRegrapherDetail::CollectRequiredEditorPins(
			Node->GetOutputPins(), Node->GetContextOutputs()));
		for (const FFlowPin& Pin : AllPins)
		{
			const FConnectedPin Connection = Node->GetConnection(Pin.PinName);
			if (!Connection.NodeGuid.IsValid())
			{
				continue;
			}

			if (!RuntimeNodes.Contains(Connection.NodeGuid))
			{
				OutIssues.Add(FString::Printf(TEXT("Connection %s.%s targets node %s, which is not in the runtime node map"),
					*NodePair.Key.ToString(), *Pin.PinName.ToString(), *Connection.NodeGuid.ToString()));
				continue;
			}

			RuntimeWires.Add(FlowGraphRegrapherDetail::MakeWireKey(
				NodePair.Key, Pin.PinName, Connection.NodeGuid, Connection.PinName));
		}
	}

	const UFlowGraph* WireGraph = Cast<UFlowGraph>(FlowAsset->GetGraph());
	if (!WireGraph)
	{
		return;
	}

	TSet<FString> EditorWires;
	for (const UEdGraphNode* EdNode : WireGraph->Nodes)
	{
		const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(EdNode);
		if (!FlowGraphNode || FlowGraphNode->GetParentNode())
		{
			continue;
		}

		const UFlowNode* EdFlowNode = Cast<UFlowNode>(FlowGraphNode->GetFlowNodeBase());
		if (!EdFlowNode || !RuntimeNodes.Contains(EdFlowNode->GetGuid()))
		{
			// Already reported above as a node-level mismatch; reporting its every wire as well
			// would bury that one real finding.
			continue;
		}

		for (const UEdGraphPin* Pin : FlowGraphNode->Pins)
		{
			if (!Pin)
			{
				continue;
			}

			for (const UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				const UFlowGraphNode* LinkedFlowGraphNode = LinkedPin ? Cast<UFlowGraphNode>(LinkedPin->GetOwningNodeUnchecked()) : nullptr;
				const UFlowNode* LinkedFlowNode = LinkedFlowGraphNode ? Cast<UFlowNode>(LinkedFlowGraphNode->GetFlowNodeBase()) : nullptr;
				if (!LinkedFlowNode || !RuntimeNodes.Contains(LinkedFlowNode->GetGuid()))
				{
					continue;
				}

				EditorWires.Add(FlowGraphRegrapherDetail::MakeWireKey(
					EdFlowNode->GetGuid(), Pin->PinName, LinkedFlowNode->GetGuid(), LinkedPin->PinName));
			}
		}
	}

	for (const FString& EditorWire : EditorWires)
	{
		if (!RuntimeWires.Contains(EditorWire))
		{
			OutIssues.Add(FString::Printf(TEXT("Editor link %s has no runtime connection - it is drawn but will not execute"), *EditorWire));
		}
	}

	for (const FString& RuntimeWire : RuntimeWires)
	{
		if (!EditorWires.Contains(RuntimeWire))
		{
			OutIssues.Add(FString::Printf(TEXT("Runtime connection %s has no editor link - it executes but is not drawn"), *RuntimeWire));
		}
	}
}

namespace FlowGraphRegrapherDetail
{
	bool CreateRequiredEditorPins(UFlowAsset& FlowAsset)
	{
		for (const TPair<FGuid, UFlowNode*>& NodePair : FlowAsset.GetNodes())
		{
			UFlowNode* const RuntimeNode = NodePair.Value;
			if (!IsValid(RuntimeNode))
			{
				UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::CreateMissingDataPins: Runtime node map entry '%s' is invalid in asset '%s'"),
					*NodePair.Key.ToString(), *FlowAsset.GetPathName());
				return false;
			}
			UFlowGraphNode* const FlowGraphNode = Cast<UFlowGraphNode>(RuntimeNode->GetGraphNode());
			if (!IsValid(FlowGraphNode))
			{
				UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Runtime node '%s' has no editor node while creating pins in asset '%s'"),
					*RuntimeNode->GetPathName(), *FlowAsset.GetPathName());
				return false;
			}

			// Check all runtime input pins - create editor pins if missing
			const TArray<FFlowPin> InputPins = CollectRequiredEditorPins(
				RuntimeNode->GetInputPins(), RuntimeNode->GetContextInputs());
			for (const FFlowPin& RuntimePin : InputPins)
			{
				if (!FlowGraphNode->FindPin(RuntimePin.PinName, EGPD_Input))
				{
					FlowGraphNode->CreateInputPin(RuntimePin);
					if (!FlowGraphNode->FindPin(RuntimePin.PinName, EGPD_Input))
					{
						UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Failed to create input pin '%s' on runtime node '%s' in asset '%s'"),
							*RuntimePin.PinName.ToString(), *RuntimeNode->GetPathName(), *FlowAsset.GetPathName());
						return false;
					}
					UE_LOG(LogFlow, Log, TEXT("CreateMissingDataPins: Created input data pin %s on node %s"),
						*RuntimePin.PinName.ToString(), *RuntimeNode->GetClass()->GetName());
				}
			}

			// Check all runtime output pins - create editor pins if missing
			const TArray<FFlowPin> OutputPins = CollectRequiredEditorPins(
				RuntimeNode->GetOutputPins(), RuntimeNode->GetContextOutputs());
			for (const FFlowPin& RuntimePin : OutputPins)
			{
				if (!FlowGraphNode->FindPin(RuntimePin.PinName, EGPD_Output))
				{
					FlowGraphNode->CreateOutputPin(RuntimePin);
					if (!FlowGraphNode->FindPin(RuntimePin.PinName, EGPD_Output))
					{
						UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Failed to create output pin '%s' on runtime node '%s' in asset '%s'"),
							*RuntimePin.PinName.ToString(), *RuntimeNode->GetPathName(), *FlowAsset.GetPathName());
						return false;
					}
					UE_LOG(LogFlow, Log, TEXT("CreateMissingDataPins: Created output data pin %s on node %s"),
						*RuntimePin.PinName.ToString(), *RuntimeNode->GetClass()->GetName());
				}
			}
		}
		return true;
	}

	void BuildTopLevelEditorNodeLookup(UFlowGraph& FlowGraph, TMap<FGuid, UFlowGraphNode*>& OutEditorNodes)
	{
		for (UEdGraphNode* const EditorNode : FlowGraph.Nodes)
		{
			UFlowGraphNode* const FlowGraphNode = Cast<UFlowGraphNode>(EditorNode);
			if (UFlowNode* const FlowNode = Cast<UFlowNode>(FlowGraphNode ? FlowGraphNode->GetFlowNodeBase() : nullptr))
			{
				OutEditorNodes.Add(FlowNode->GetGuid(), FlowGraphNode);
			}
		}
	}

	bool ReconcileEditorPinConnections(UFlowAsset& FlowAsset, UFlowGraph& FlowGraph)
	{
		TMap<FGuid, UFlowGraphNode*> NodeGuidToEditorNode;
		BuildTopLevelEditorNodeLookup(FlowGraph, NodeGuidToEditorNode);
		const TMap<FGuid, UFlowNode*>& RuntimeNodes = FlowAsset.GetNodes();

		for (const TPair<FGuid, UFlowNode*>& NodePair : RuntimeNodes)
		{
			UFlowNode* const SourceRuntimeNode = NodePair.Value;
			if (!IsValid(SourceRuntimeNode))
			{
				UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::CreateEditorPinConnections: Runtime node map entry '%s' is invalid in asset '%s'"),
					*NodePair.Key.ToString(), *FlowAsset.GetPathName());
				return false;
			}
			UFlowGraphNode* const* SourceEditorNodePtr = NodeGuidToEditorNode.Find(NodePair.Key);
			if (!SourceEditorNodePtr || !IsValid(*SourceEditorNodePtr))
			{
				UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Runtime node '%s' has no editor node while wiring asset '%s'"),
					*SourceRuntimeNode->GetPathName(), *FlowAsset.GetPathName());
				return false;
			}
			UFlowGraphNode* const SourceEditorNode = *SourceEditorNodePtr;

			// Wire up exec output pin connections (stored on source node). Exec output pins are
			// one-target-only at runtime (UFlowNode::Connections is keyed by source pin name) - the
			// editor graph must match exactly, so any stale link left over from a previous regraph is
			// broken before establishing the runtime connection (or leaving the pin disconnected).
			// Otherwise HarvestNodeConnections() can copy the stale editor link into the runtime map.
			const TArray<FFlowPin> OutputPins = CollectRequiredEditorPins(
				SourceRuntimeNode->GetOutputPins(), SourceRuntimeNode->GetContextOutputs());
			for (const FFlowPin& OutputPin : OutputPins)
			{
				if (!OutputPin.IsExecPin())
				{
					continue;
				}
				UEdGraphPin* const SourcePin = SourceEditorNode->FindPin(OutputPin.PinName, EGPD_Output);
				if (!SourcePin)
				{
					UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Missing editor output pin '%s' for runtime node '%s' while wiring asset '%s'"),
						*OutputPin.PinName.ToString(), *SourceRuntimeNode->GetPathName(), *FlowAsset.GetPathName());
					return false;
				}

				const FConnectedPin Connection = SourceRuntimeNode->GetConnection(OutputPin.PinName);
				// Break an existing editor link only when the runtime connection is genuinely gone
				// (an invalid GUID means the connection was removed or retargeted). A valid GUID
				// that fails to resolve to a node is a
				// dangling/stale reference, not a removal: AssetTools duplication regenerates node
				// guids but copies UFlowNode::Connections verbatim, so a freshly-cloned asset carries
				// connection targets pointing at the pre-duplication guids. In that case the editor
				// link (whose pointers duplication *did* remap correctly) is the trustworthy record -
				// leaving it intact lets HarvestNodeConnections() rebuild the runtime map from it and
				// repoint the stale GUID. Breaking it would discard a connection that the editor
				// still knows how to restore.
				if (!Connection.NodeGuid.IsValid())
				{
					BreakPinLinksWithoutNotification(*SourcePin);
					continue;
				}
				UFlowGraphNode* const* TargetEditorNodePtr = NodeGuidToEditorNode.Find(Connection.NodeGuid);
				if (!TargetEditorNodePtr || !IsValid(*TargetEditorNodePtr))
				{
					if (!RuntimeNodes.Contains(Connection.NodeGuid) && !SourcePin->LinkedTo.IsEmpty())
					{
						continue;
					}
					UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Exec connection '%s.%s' targets missing editor node '%s' in asset '%s'"),
						*NodePair.Key.ToString(), *OutputPin.PinName.ToString(), *Connection.NodeGuid.ToString(), *FlowAsset.GetPathName());
					return false;
				}
				UEdGraphPin* const TargetPin = (*TargetEditorNodePtr)->FindPin(Connection.PinName, EGPD_Input);
				if (!TargetPin)
				{
					UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Exec connection '%s.%s' targets missing editor input pin '%s.%s' in asset '%s'"),
						*NodePair.Key.ToString(), *OutputPin.PinName.ToString(), *Connection.NodeGuid.ToString(),
						*Connection.PinName.ToString(), *FlowAsset.GetPathName());
					return false;
				}
				if (!SourcePin->LinkedTo.Contains(TargetPin))
				{
					BreakPinLinksWithoutNotification(*SourcePin);
					LinkPinsWithoutNotification(*SourcePin, *TargetPin);
				}
			}

			// Wire up data input pin connections (stored on target/receiving node). Symmetric to the
			// exec case above: a data input pin is one-source-only at runtime, so stale links are
			// broken the same way before (re-)establishing the correct one.
			const TArray<FFlowPin> InputPins = CollectRequiredEditorPins(
				SourceRuntimeNode->GetInputPins(), SourceRuntimeNode->GetContextInputs());
			for (const FFlowPin& InputPin : InputPins)
			{
				if (InputPin.IsExecPin())
				{
					continue;
				}
				UEdGraphPin* const TargetInputPin = SourceEditorNode->FindPin(InputPin.PinName, EGPD_Input);
				if (!TargetInputPin)
				{
					UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Missing editor input pin '%s' for runtime node '%s' while wiring asset '%s'"),
						*InputPin.PinName.ToString(), *SourceRuntimeNode->GetPathName(), *FlowAsset.GetPathName());
					return false;
				}

				// Connection stores: source node guid and source output pin name
				const FConnectedPin Connection = SourceRuntimeNode->GetConnection(InputPin.PinName);
				// See the exec-output loop above: only break the existing editor link when the
				// runtime connection is genuinely gone (invalid guid). A valid-but-unresolvable
				// guid is a stale post-duplication reference; keep the editor link so
				// HarvestNodeConnections() can rebuild and repoint the runtime map from it.
				if (!Connection.NodeGuid.IsValid())
				{
					BreakPinLinksWithoutNotification(*TargetInputPin);
					continue;
				}
				UFlowGraphNode* const* DataSourceEditorNodePtr = NodeGuidToEditorNode.Find(Connection.NodeGuid);
				if (!DataSourceEditorNodePtr || !IsValid(*DataSourceEditorNodePtr))
				{
					if (!RuntimeNodes.Contains(Connection.NodeGuid) && !TargetInputPin->LinkedTo.IsEmpty())
					{
						continue;
					}
					UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Data connection '%s.%s' references missing editor source node '%s' in asset '%s'"),
						*NodePair.Key.ToString(), *InputPin.PinName.ToString(), *Connection.NodeGuid.ToString(), *FlowAsset.GetPathName());
					return false;
				}
				UEdGraphPin* const SourceOutputPin = (*DataSourceEditorNodePtr)->FindPin(Connection.PinName, EGPD_Output);
				if (!SourceOutputPin)
				{
					UE_LOG(LogFlow, Error, TEXT("UFlowGraphRegrapher::RegraphFlowAsset: Data connection '%s.%s' references missing editor output pin '%s.%s' in asset '%s'"),
						*NodePair.Key.ToString(), *InputPin.PinName.ToString(), *Connection.NodeGuid.ToString(),
						*Connection.PinName.ToString(), *FlowAsset.GetPathName());
					return false;
				}
				if (!SourceOutputPin->LinkedTo.Contains(TargetInputPin))
				{
					BreakPinLinksWithoutNotification(*TargetInputPin);
					LinkPinsWithoutNotification(*SourceOutputPin, *TargetInputPin);
				}
			}
		}
		return true;
	}
}
