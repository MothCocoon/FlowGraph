// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowCollapseToSubGraphApply.h"
#include "FlowCollapseToSubGraphPlan.h"

#include "Asset/FlowAssetFactory.h"
#include "FlowAsset.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Nodes/FlowNode.h"
#include "Nodes/FlowPin.h"
#include "Nodes/Graph/FlowNode_CustomOutput.h"
#include "Nodes/Graph/FlowNode_Finish.h"
#include "Nodes/Graph/FlowNode_Start.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"

#include "AssetToolsModule.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphUtilities.h"
#include "GraphEditor.h"
#include "IAssetTools.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FlowCollapseToSubGraph"

namespace FlowCollapseToSubGraph
{
	namespace Internal
	{
		/** Horizontal gap between the Start node and the collapsed region in the new graph. */
		constexpr float ClonedRegionOffsetX = 400.0f;

		/** Horizontal gap between the collapsed region and the Finish or Custom Output nodes. */
		constexpr float ExitNodeOffsetX = 400.0f;

		/** Vertical gap between stacked interface nodes. */
		constexpr float InterfaceNodeSpacingY = 150.0f;

		/** Recursively gathers a node and its AddOns, stamping the parent indices that survive the copy. */
		void PrepareNodeForCopy(UFlowGraphNode& FlowGraphNode, const int32 ParentEdNodeIndex, FGraphPanelSelectionSet& OutNodes)
		{
			const int32 ThisNodeIndex = OutNodes.Num();
			bool bAlreadyInSet = false;
			OutNodes.Add(&FlowGraphNode, &bAlreadyInSet);
			if (bAlreadyInSet)
			{
				return;
			}

			FlowGraphNode.PrepareForCopying();
			FlowGraphNode.CopySubNodeParentIndex = ParentEdNodeIndex;
			FlowGraphNode.CopySubNodeIndex = ThisNodeIndex;
			for (UFlowGraphNode* SubNode : FlowGraphNode.SubNodes)
			{
				if (SubNode)
				{
					PrepareNodeForCopy(*SubNode, ThisNodeIndex, OutNodes);
				}
			}
		}

		/** Destroys a graph node and removes its runtime counterpart from the owning asset. */
		void DestroyNodeAndUnregister(UFlowGraphNode& GraphNode, UFlowAsset& OwningAsset, UEdGraph& OwningGraph)
		{
			FGuid RuntimeNodeGuid;
			if (const UFlowNode* FlowNode = Cast<UFlowNode>(GraphNode.GetFlowNodeBase()))
			{
				RuntimeNodeGuid = FlowNode->GetGuid();
			}

			OwningGraph.GetSchema()->BreakNodeLinks(GraphNode);
			GraphNode.DestroyNode();
			if (RuntimeNodeGuid.IsValid())
			{
				OwningAsset.UnregisterNode(RuntimeNodeGuid);
			}
		}

		/** Returns every graph node in the graph whose runtime node is of the given class. */
		TArray<UFlowGraphNode*> FindGraphNodesOfClass(const UFlowGraph& Graph, const UClass& FlowNodeClass)
		{
			TArray<UFlowGraphNode*> Found;
			for (UEdGraphNode* EdGraphNode : Graph.Nodes)
			{
				UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(EdGraphNode);
				const UFlowNodeBase* FlowNodeBase = IsValid(FlowGraphNode) ? FlowGraphNode->GetFlowNodeBase() : nullptr;
				if (FlowNodeBase && FlowNodeBase->IsA(&FlowNodeClass))
				{
					Found.Add(FlowGraphNode);
				}
			}
			return Found;
		}

		/** First exec pin in the requested direction, which is what the interface nodes expose. */
		UEdGraphPin* FindFirstExecPin(UFlowGraphNode& GraphNode, const EEdGraphPinDirection Direction)
		{
			for (UEdGraphPin* Pin : GraphNode.Pins)
			{
				if (Pin && Pin->Direction == Direction && FFlowPin::IsExecPinCategory(Pin->PinType.PinCategory))
				{
					return Pin;
				}
			}
			return nullptr;
		}

		/**
		 * Restores the parent/AddOn hierarchy flattened by ImportNodesFromText.
		 *
		 * Text import initially places every UFlowGraphNode in UEdGraph::Nodes. Flow AddOns are not standalone
		 * graph nodes, however; they are owned through their parent's SubNodes array. CopySubNodeParentIndex
		 * identifies the imported parent after object references have been remapped by the text importer.
		 */
		bool ReattachImportedAddOns(
			const TSet<UEdGraphNode*>& ImportedNodes,
			const TMap<int32, UFlowGraphNode*>& ClonedByCopyIndex,
			UFlowGraph& NewGraph)
		{
			for (UEdGraphNode* ImportedNode : ImportedNodes)
			{
				UFlowGraphNode* ClonedAddOn = Cast<UFlowGraphNode>(ImportedNode);
				if (!ClonedAddOn || !ClonedAddOn->IsSubNode())
				{
					continue;
				}

				UFlowGraphNode* ClonedParent = ClonedByCopyIndex.FindRef(ClonedAddOn->CopySubNodeParentIndex);
				if (!ClonedParent)
				{
					return false;
				}

				// AddOns are displayed relative to their parent, so standalone graph coordinates are meaningless.
				ClonedAddOn->NodePosX = 0;
				ClonedAddOn->NodePosY = 0;

				// DestroyNode removes the AddOn from UEdGraph::Nodes. It remains alive through ImportedNodes until
				// AddSubNode establishes the durable parent reference immediately below.
				ClonedAddOn->DestroyNode();
				ClonedParent->AddSubNode(ClonedAddOn, &NewGraph);
			}

			return true;
		}

		/**
		 * Clones the planned nodes into the new graph, preserving node Guids.
		 *
		 * Paste into the same graph has to re-Guid to avoid collisions, but a freshly created asset has
		 * none, and a Flow node's Guid is its identity for its whole life. Preserving it keeps debugger
		 * bookmarks, saved state and the Guid-keyed Connections map meaningful, and it is what lets the
		 * boundary pins be matched back to their clones below.
		 */
		bool CloneSelectionIntoGraph(
			const FCollapsePlan& Plan,
			UFlowAsset& NewAsset,
			UFlowGraph& NewGraph,
			TMap<FGuid, UFlowGraphNode*>& OutClonedByOriginalGuid)
		{
			// Exporting through Unreal's graph text format performs the same deep UObject duplication used by
			// ordinary copy/paste, including instanced runtime nodes and recursively selected AddOns.
			FGraphPanelSelectionSet NodesToExport;
			for (UFlowGraphNode* SelectedNode : Plan.SelectedNodes)
			{
				PrepareNodeForCopy(*SelectedNode, INDEX_NONE, NodesToExport);
			}

			FString ExportedText;
			FEdGraphUtilities::ExportNodesToText(NodesToExport, ExportedText);

			// PrepareForCopying moved each runtime node under its graph node; hand it back to the source asset.
			for (FGraphPanelSelectionSet::TIterator NodeIterator(NodesToExport); NodeIterator; ++NodeIterator)
			{
				if (UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(*NodeIterator))
				{
					FlowGraphNode->PostCopyNode();
				}
			}

			TSet<UEdGraphNode*> ImportedNodes;
			FEdGraphUtilities::ImportNodesFromText(&NewGraph, ExportedText, ImportedNodes);

			// The copy indices are stable identifiers written before export. Build this lookup before clearing
			// imported child arrays so each AddOn can later recover its imported parent without relying on stale
			// object references from the source graph.
			TMap<int32, UFlowGraphNode*> ClonedByCopyIndex;
			for (UEdGraphNode* ImportedNode : ImportedNodes)
			{
				UFlowGraphNode* ClonedFlowNode = Cast<UFlowGraphNode>(ImportedNode);
				if (!ClonedFlowNode)
				{
					continue;
				}

				ClonedByCopyIndex.Add(ClonedFlowNode->CopySubNodeIndex, ClonedFlowNode);
				if (!ClonedFlowNode->IsSubNode())
				{
					OutClonedByOriginalGuid.Add(ClonedFlowNode->NodeGuid, ClonedFlowNode);
					if (UFlowNode* FlowNode = Cast<UFlowNode>(ClonedFlowNode->GetFlowNodeBase()))
					{
						NewAsset.RegisterNode(ClonedFlowNode->NodeGuid, FlowNode);
					}
				}

				// Rebuild every parent/AddOn relationship from the copied indices below rather than retaining the
				// hierarchy serialized by the source graph.
				ClonedFlowNode->RemoveAllSubNodes();
			}

			return ReattachImportedAddOns(ImportedNodes, ClonedByCopyIndex, NewGraph);
		}

		/** Shifts the cloned region so it sits to the right of the Start node instead of at its original coordinates. */
		void RepositionClonedRegion(const TMap<FGuid, UFlowGraphNode*>& ClonedNodes, const FVector2f& Origin)
		{
			if (ClonedNodes.IsEmpty())
			{
				return;
			}

			int32 MinPosX = MAX_int32;
			int32 MinPosY = MAX_int32;
			for (const TPair<FGuid, UFlowGraphNode*>& ClonedPair : ClonedNodes)
			{
				MinPosX = FMath::Min(MinPosX, ClonedPair.Value->NodePosX);
				MinPosY = FMath::Min(MinPosY, ClonedPair.Value->NodePosY);
			}

			const int32 DeltaX = FMath::RoundToInt(Origin.X) - MinPosX;
			const int32 DeltaY = FMath::RoundToInt(Origin.Y) - MinPosY;
			for (const TPair<FGuid, UFlowGraphNode*>& ClonedPair : ClonedNodes)
			{
				ClonedPair.Value->NodePosX += DeltaX;
				ClonedPair.Value->NodePosY += DeltaY;
			}
		}

		/** Average position of the selection, where the replacement SubGraph node is placed. */
		FVector2f ComputeSelectionCentroid(const TSet<UFlowGraphNode*>& SelectedNodes)
		{
			FVector2f Sum = FVector2f::ZeroVector;
			for (const UFlowGraphNode* SelectedNode : SelectedNodes)
			{
				Sum.X += static_cast<float>(SelectedNode->NodePosX);
				Sum.Y += static_cast<float>(SelectedNode->NodePosY);
			}
			return SelectedNodes.IsEmpty() ? FVector2f::ZeroVector : Sum / static_cast<float>(SelectedNodes.Num());
		}
	}

	FCollapseApply::FCollapseApply(
		const FCollapsePlan& InPlan,
		UFlowAsset& InSourceAsset,
		const FString& InNewAssetName)
		: Plan(InPlan)
		, SourceAsset(InSourceAsset)
		, NewAssetName(InNewAssetName)
	{
	}

	FCollapseResult FCollapseApply::Run()
	{
		if (!InitializeSource() ||
			!CreateAsset() ||
			!CloneNodes() ||
			!PrepareLayout() ||
			!ConnectEntry() ||
			!BuildExits())
		{
			return Result;
		}

		FinalizeNewAsset();
		if (!ReplaceSourceSelection())
		{
			return Result;
		}

		FinalizeSourceAsset();
		Result.NewAsset = NewAsset;
		Result.SubGraphNode = SubGraphGraphNode;
		return Result;
	}

	bool FCollapseApply::Fail(const FText& Error)
	{
		Result.Error = Error;
		return false;
	}

	bool FCollapseApply::InitializeSource()
	{
		if (!Plan.CanApply())
		{
			return Fail(LOCTEXT("PlanNotApplicable", "The selection cannot be collapsed."));
		}

		const FText NameError = ValidateNewAssetName(SourceAsset, NewAssetName);
		if (!NameError.IsEmpty())
		{
			return Fail(NameError);
		}

		SourceGraph = Cast<UFlowGraph>(SourceAsset.GetGraph());
		if (!IsValid(SourceGraph))
		{
			return Fail(LOCTEXT("NoSourceGraph", "The source asset has no graph."));
		}

		SourceGraph->Modify();
		SourceAsset.Modify();
		Result.bTransactionRecorded = true;
		return true;
	}

	bool FCollapseApply::CreateAsset()
	{
		// UFlowAsset::GetDefaultFlowAssetForSubgraphs is the framework's own hook for this decision. The base
		// returns GetClass(), so the sub-graph keeps the source's class by default, but a subclass that is not
		// meaningful as a fragment can nominate a better one.
		const TSubclassOf<UFlowAsset> NewAssetClass = SourceAsset.GetDefaultFlowAssetForSubgraphs();
		const FString PackageFolder = FPackageName::GetLongPackagePath(SourceAsset.GetOutermost()->GetName());
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UFactory* FlowAssetFactory = Cast<UFactory>(UFlowAssetFactory::StaticClass()->GetDefaultObject());
		NewAsset = Cast<UFlowAsset>(AssetTools.CreateAsset(NewAssetName, PackageFolder, NewAssetClass, FlowAssetFactory));
		if (!IsValid(NewAsset))
		{
			return Fail(FText::Format(LOCTEXT("CreateAssetFailed", "Could not create '{0}' in {1}."),
				FText::FromString(NewAssetName),
				FText::FromString(PackageFolder)));
		}

		Result.NewAsset = NewAsset;
		NewGraph = Cast<UFlowGraph>(NewAsset->GetGraph());
		return IsValid(NewGraph) || Fail(LOCTEXT("NoNewGraph", "The new asset was created without a graph."));
	}

	bool FCollapseApply::CloneNodes()
	{
		// The lock only spans the bulk clone, which is what it exists for. It must be released before any
		// interface node is created: UFlowGraphNode::CanReconstructNode returns false while the graph is
		// locked, FFlowGraphSchemaAction_NewNode::CreateNode builds its pins through ReconstructNode, and a
		// node created inside the lock therefore ends up with no pins at all and cannot be connected.
		NewGraph->LockUpdates();
		const bool bAddOnHierarchyRestored = Internal::CloneSelectionIntoGraph(
			Plan,
			*NewAsset,
			*NewGraph,
			ClonedByOriginalGuid);
		NewGraph->UpdateClassData();
		NewGraph->UnlockUpdates();
		if (!bAddOnHierarchyRestored)
		{
			return Fail(LOCTEXT("AddOnHierarchyNotCloned", "An AddOn could not be attached to its cloned parent."));
		}

		for (const UFlowGraphNode* SelectedNode : Plan.SelectedNodes)
		{
			if (!ClonedByOriginalGuid.Contains(SelectedNode->NodeGuid))
			{
				return Fail(FText::Format(
					LOCTEXT("NodeNotCloned", "'{0}' could not be cloned into the new sub-graph."),
					SelectedNode->GetNodeTitle(ENodeTitleType::ListView)));
			}
		}
		return true;
	}

	bool FCollapseApply::PrepareLayout()
	{
		// The factory seeds a Start node. If the selection brought its own, keep that one and drop the seed.
		TArray<UFlowGraphNode*> StartNodes = Internal::FindGraphNodesOfClass(*NewGraph, *UFlowNode_Start::StaticClass());
		if (StartNodes.Num() > 1)
		{
			for (UFlowGraphNode* Candidate : StartNodes)
			{
				if (!ClonedByOriginalGuid.Contains(Candidate->NodeGuid))
				{
					Internal::DestroyNodeAndUnregister(*Candidate, *NewAsset, *NewGraph);
					break;
				}
			}
			StartNodes = Internal::FindGraphNodesOfClass(*NewGraph, *UFlowNode_Start::StaticClass());
		}

		StartNode = StartNodes.IsEmpty() ? nullptr : StartNodes[0];
		if (!StartNode)
		{
			return Fail(LOCTEXT("NoStartNode", "The new sub-graph has no Start node."));
		}

		ClonedRegionOrigin = FVector2f(
			static_cast<float>(StartNode->NodePosX) + Internal::ClonedRegionOffsetX,
			static_cast<float>(StartNode->NodePosY));
		Internal::RepositionClonedRegion(ClonedByOriginalGuid, ClonedRegionOrigin);
		ClonedRegionMaxX = FMath::RoundToInt(ClonedRegionOrigin.X);
		for (const TPair<FGuid, UFlowGraphNode*>& ClonedPair : ClonedByOriginalGuid)
		{
			ClonedRegionMaxX = FMath::Max(ClonedRegionMaxX, ClonedPair.Value->NodePosX);
		}
		NewGraphSchema = NewGraph->GetSchema();
		return NewGraphSchema != nullptr || Fail(LOCTEXT("NoNewGraphSchema", "The new sub-graph has no graph schema."));
	}

	bool FCollapseApply::ConnectEntry()
	{
		check(!Plan.Entries.IsEmpty());

		const FBoundaryPin& Entry = Plan.Entries[0];
		UFlowGraphNode* ClonedEntryNode = ClonedByOriginalGuid.FindRef(Entry.InnerNodeGuid);
		UEdGraphPin* ClonedEntryPin = ClonedEntryNode ? ClonedEntryNode->FindPin(Entry.InnerPinName, EGPD_Input) : nullptr;
		UEdGraphPin* StartOutputPin = Internal::FindFirstExecPin(*StartNode, EGPD_Output);
		if (!ClonedEntryPin || !StartOutputPin || !NewGraphSchema->TryCreateConnection(StartOutputPin, ClonedEntryPin))
		{
			return Fail(LOCTEXT("EntryNotWired", "The selection's entry could not be connected to the new sub-graph's Start node."));
		}
		return true;
	}

	bool FCollapseApply::BuildExits()
	{
		// A Finish node always exists. An exit leaving through an unnamed pin runs into it; everything else
		// gets a Custom Output. When nothing routes to it, it stays unconnected for the designer to wire.
		const float ExitColumnPosX = static_cast<float>(ClonedRegionMaxX) + Internal::ExitNodeOffsetX;
		float NextExitNodePosY = ClonedRegionOrigin.Y;
		FinishGraphNode = FFlowGraphSchemaAction_NewNode::CreateNode(
			NewGraph, nullptr, UFlowNode_Finish::StaticClass(), FVector2f(ExitColumnPosX, NextExitNodePosY), false);
		if (!FinishGraphNode)
		{
			return Fail(LOCTEXT("CreateFinishFailed", "The Finish node could not be created in the new sub-graph."));
		}
		NextExitNodePosY += Internal::InterfaceNodeSpacingY;

		for (const FBoundaryPin& Exit : Plan.Exits)
		{
			if (!BuildExit(Exit, ExitColumnPosX, NextExitNodePosY))
			{
				return false;
			}
		}
		return true;
	}

	bool FCollapseApply::BuildExit(const FBoundaryPin& Exit, const float ExitColumnPosX, float& NextExitNodePosY)
	{
		UFlowGraphNode* ClonedNode = ClonedByOriginalGuid.FindRef(Exit.InnerNodeGuid);
		UEdGraphPin* ClonedInnerPin = ClonedNode ? ClonedNode->FindPin(Exit.InnerPinName, EGPD_Output) : nullptr;
		if (!ClonedInnerPin)
		{
			return Fail(FText::Format(
				LOCTEXT("ExitPinNotCloned", "Exit '{0}' could not be found on its cloned node."),
				FText::FromName(Exit.InterfaceName)));
		}

		UFlowGraphNode* ExitGraphNode = Exit.bUsesBuiltInPin
			? FinishGraphNode
			: CreateCustomOutput(Exit, ExitColumnPosX, NextExitNodePosY);
		if (!ExitGraphNode)
		{
			return false;
		}

		UEdGraphPin* ExitNodeInputPin = Internal::FindFirstExecPin(*ExitGraphNode, EGPD_Input);
		if (!ExitNodeInputPin || !NewGraphSchema->TryCreateConnection(ClonedInnerPin, ExitNodeInputPin))
		{
			return Fail(FText::Format(
				LOCTEXT("ExitNotWired", "Exit '{0}' could not be connected to its {1} node."),
				FText::FromName(Exit.InterfaceName),
				Exit.bUsesBuiltInPin ? LOCTEXT("FinishNodeLabel", "Finish") : LOCTEXT("CustomOutputNodeLabel", "Custom Output")));
		}
		return true;
	}

	UFlowGraphNode* FCollapseApply::CreateCustomOutput(
		const FBoundaryPin& Exit,
		const float ExitColumnPosX,
		float& NextExitNodePosY)
	{
		constexpr bool bSelectNewNode = false;
		const FVector2f NewNodePosition(ExitColumnPosX, NextExitNodePosY);
		UEdGraphPin* FromPin = nullptr;
		UFlowGraphNode* GraphNode = FFlowGraphSchemaAction_NewNode::CreateNode(
			NewGraph,
			FromPin,
			UFlowNode_CustomOutput::StaticClass(),
			NewNodePosition,
			bSelectNewNode);

		UFlowNode_CustomOutput* CustomOutputNode = GraphNode
			? Cast<UFlowNode_CustomOutput>(GraphNode->GetFlowNodeBase())
			: nullptr;
		if (!CustomOutputNode)
		{
			Fail(FText::Format(
				LOCTEXT("CreateCustomOutputFailed", "Custom Output '{0}' could not be created in the new sub-graph."),
				FText::FromName(Exit.InterfaceName)));
			return nullptr;
		}

		NextExitNodePosY += Internal::InterfaceNodeSpacingY;
		CustomOutputNode->SetEventName(Exit.InterfaceName);
		GraphNode->ReconstructNode();
		return GraphNode;
	}

	bool FCollapseApply::ReplaceSourceSelection()
	{
		// Replace the selection in the source graph.
		const FVector2f SubGraphNodePosition = Internal::ComputeSelectionCentroid(Plan.SelectedNodes);
		for (UFlowGraphNode* SelectedNode : Plan.SelectedNodes)
		{
			Internal::DestroyNodeAndUnregister(*SelectedNode, SourceAsset, *SourceGraph);
		}

		constexpr bool bSelectNewNode = false;
		UEdGraphPin* FromPin = nullptr;
		SubGraphGraphNode = FFlowGraphSchemaAction_NewNode::CreateNode(
			SourceGraph,
			FromPin,
			UFlowNode_SubGraph::StaticClass(),
			SubGraphNodePosition,
			bSelectNewNode);

		UFlowNode_SubGraph* SubGraphNode = SubGraphGraphNode
			? Cast<UFlowNode_SubGraph>(SubGraphGraphNode->GetFlowNodeBase())
			: nullptr;
		if (!SubGraphNode)
		{
			return Fail(LOCTEXT("CreateSubGraphNodeFailed", "The replacement Sub Graph node could not be created."));
		}

		SubGraphNode->SetAsset(NewAsset);
		SubGraphGraphNode->ReconstructNode();
		return ReconnectSourceEntry() && ReconnectSourceExits();
	}

	bool FCollapseApply::ReconnectSourceEntry()
	{
		check(!Plan.Entries.IsEmpty());

		const FBoundaryPin& Entry = Plan.Entries[0];
		UEdGraphPin* SubGraphInputPin = SubGraphGraphNode->FindPin(Entry.InterfaceName, EGPD_Input);
		if (!SubGraphInputPin)
		{
			return Fail(LOCTEXT("MissingSubGraphInput", "The replacement Sub Graph node has no Start input."));
		}

		for (UEdGraphPin* OuterPin : Entry.OuterPins)
		{
			if (!OuterPin || !SourceGraph->GetSchema()->TryCreateConnection(OuterPin, SubGraphInputPin))
			{
				return Fail(LOCTEXT("ReconnectSubGraphInputFailed", "The source graph could not be connected to the replacement Sub Graph node's Start input."));
			}
		}
		return true;
	}

	bool FCollapseApply::ReconnectSourceExits()
	{
		for (const FBoundaryPin& Exit : Plan.Exits)
		{
			UEdGraphPin* SubGraphOutputPin = SubGraphGraphNode->FindPin(Exit.InterfaceName, EGPD_Output);
			if (!SubGraphOutputPin)
			{
				return Fail(FText::Format(
					LOCTEXT("MissingSubGraphOutput", "The replacement Sub Graph node has no '{0}' output."),
					FText::FromName(Exit.InterfaceName)));
			}

			for (UEdGraphPin* OuterPin : Exit.OuterPins)
			{
				if (!OuterPin || !SourceGraph->GetSchema()->TryCreateConnection(SubGraphOutputPin, OuterPin))
				{
					return Fail(FText::Format(
						LOCTEXT("ReconnectSubGraphOutputFailed", "The replacement Sub Graph node's '{0}' output could not be reconnected in the source graph."),
						FText::FromName(Exit.InterfaceName)));
				}
			}
		}
		return true;
	}

	void FCollapseApply::FinalizeNewAsset() const
	{
		// The parent's SubGraph pins come from these arrays, not from the nodes, and nothing else populates them.
		NewAsset->RebuildCustomInterfaceLists();
		NewGraph->NotifyGraphChanged();
		NewAsset->PostEditChange();
		NewAsset->MarkPackageDirty();
	}

	void FCollapseApply::FinalizeSourceAsset() const
	{
		SourceGraph->NotifyGraphChanged();
		SourceAsset.PostEditChange();
		SourceAsset.MarkPackageDirty();
	}

	FCollapseResult ApplyCollapse(const FCollapsePlan& Plan, UFlowAsset& SourceAsset, const FString& NewAssetName)
	{
		return FCollapseApply(Plan, SourceAsset, NewAssetName).Run();
	}
}

#undef LOCTEXT_NAMESPACE
