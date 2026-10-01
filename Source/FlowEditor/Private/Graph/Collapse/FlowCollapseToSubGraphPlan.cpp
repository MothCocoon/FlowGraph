// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowCollapseToSubGraphPlan.h"

#include "FlowAsset.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Nodes/FlowNode.h"
#include "Nodes/FlowPin.h"
#include "Nodes/Graph/FlowNode_CustomInput.h"
#include "Nodes/Graph/FlowNode_CustomOutput.h"
#include "Nodes/Graph/FlowNode_Finish.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"

#include "EdGraph/EdGraphPin.h"
#include "Misc/PackageName.h"

#define LOCTEXT_NAMESPACE "FlowCollapseToSubGraph"

namespace FlowCollapseToSubGraph
{
	namespace Internal
	{
		const FString AssetNamePrefix = TEXT("Subgraph_");

		static FText GetNodeTitleText(const UEdGraphNode& Node)
		{
			return Node.GetNodeTitle(ENodeTitleType::ListView);
		}

		/** Strips everything that cannot appear in a pin name, so a node title can seed an event name. */
		FString SanitizeForEventName(const FString& RawName)
		{
			FString Sanitized;
			Sanitized.Reserve(RawName.Len());

			for (const TCHAR Character : RawName)
			{
				if (FChar::IsAlnum(Character) || Character == TEXT('_'))
				{
					Sanitized.AppendChar(Character);
				}
			}

			return Sanitized;
		}

		/** Names the SubGraph node already uses for its own pins, which a custom event must not collide with. */
		void ReserveBuiltInPinNames(TSet<FName>& UsedNames)
		{
			UsedNames.Add(UFlowNode_SubGraph::StartPin.PinName);
			UsedNames.Add(UFlowNode_SubGraph::FinishPin.PinName);
			UsedNames.Add(TEXT("Preload Content"));
			UsedNames.Add(TEXT("Flush Content"));
			UsedNames.Add(TEXT("All Preloads Complete"));
			UsedNames.Add(TEXT("AssetParams"));
		}

		/** Appends a numeric suffix until the name is free, matching UEdGraphNode::CreateUniquePinName. */
		FName MakeUniqueInterfaceName(const FString& BaseName, TSet<FName>& UsedNames)
		{
			const FString SafeBaseName = BaseName.IsEmpty() ? TEXT("Event") : BaseName;

			FName Candidate(*SafeBaseName);
			int32 Suffix = 1;
			while (UsedNames.Contains(Candidate))
			{
				++Suffix;
				Candidate = FName(*FString::Printf(TEXT("%s%d"), *SafeBaseName, Suffix));
			}

			UsedNames.Add(Candidate);
			return Candidate;
		}

		/**
		 * True when a pin carries no name of its own, i.e. it is one of the unnamed defaults every Flow node
		 * gets. Such a pin has nothing meaningful to call an interface pin after, so it maps to the built-in
		 * Start or Finish instead of inventing a custom event.
		 */
		bool IsUnnamedExecPin(const FName PinName)
		{
			return PinName.IsNone()
				|| PinName == UFlowNode::DefaultInputPin.PinName
				|| PinName == UFlowNode::DefaultOutputPin.PinName;
		}

		/** Seeds an event name from the pin's own name when it is meaningful, otherwise from its node's title. */
		FString DeriveBoundaryBaseName(const FBoundaryPin& Boundary)
		{
			if (!IsUnnamedExecPin(Boundary.InnerPinName))
			{
				return SanitizeForEventName(Boundary.InnerPinName.ToString());
			}

			if (const UEdGraphNode* OwningNode = Boundary.InnerPin ? Boundary.InnerPin->GetOwningNode() : nullptr)
			{
				return SanitizeForEventName(GetNodeTitleText(*OwningNode).ToString());
			}

			return FString();
		}
	}

	FCollapsePlan::FCollapsePlan(const TSet<UFlowGraphNode*>& InSelectedNodes)
	{
		GatherSelectedNodes(InSelectedNodes);
		if (SelectedNodes.IsEmpty())
		{
			Errors.Add(LOCTEXT("NothingSelected", "Select the nodes to collapse first."));
			return;
		}

		ValidateSelectedNodes();
		GatherBoundaryPins();
		SortBoundaryPinsByPosition(Entries);
		SortBoundaryPinsByPosition(Exits);
		ConfigureEntry();
		ConfigureExits();
	}

	void FCollapsePlan::GatherSelectedNodes(const TSet<UFlowGraphNode*>& InSelectedNodes)
	{
		for (UFlowGraphNode* SelectedNode : InSelectedNodes)
		{
			if (IsValid(SelectedNode) && !SelectedNode->IsSubNode())
			{
				SelectedNodes.Add(SelectedNode);
			}
		}
	}

	void FCollapsePlan::ValidateSelectedNodes()
	{
		const UEdGraph* CommonGraph = nullptr;
		for (UFlowGraphNode* SelectedNode : SelectedNodes)
		{
			if (CommonGraph == nullptr)
			{
				CommonGraph = SelectedNode->GetGraph();
			}
			else if (CommonGraph != SelectedNode->GetGraph())
			{
				Errors.Add(LOCTEXT("MultipleGraphs", "All selected nodes must belong to the same graph."));
				break;
			}
		}

		for (UFlowGraphNode* SelectedNode : SelectedNodes)
		{
			const UFlowNodeBase* FlowNode = SelectedNode->GetFlowNodeBase();

			if (!IsValid(FlowNode))
			{
				continue;
			}

			if (FlowNode->IsA<UFlowNode_CustomInput>() || FlowNode->IsA<UFlowNode_CustomOutput>())
			{
				Errors.Add(FText::Format(
					LOCTEXT("InterfaceNodeCannotMove", "'{0}' defines the source asset's interface and must remain outside the selection."),
					Internal::GetNodeTitleText(*SelectedNode)));
			}
			else if (FlowNode->IsA<UFlowNode_Finish>())
			{
				Errors.Add(FText::Format(
					LOCTEXT("FinishNodeCannotMove", "'{0}' finishes the source asset and must remain outside the selection."),
					Internal::GetNodeTitleText(*SelectedNode)));
			}
			else if (!SelectedNode->CanUserDeleteNode() || !SelectedNode->CanDuplicateNode())
			{
				Errors.Add(FText::Format(
					LOCTEXT("NodeCannotMove", "'{0}' cannot be moved into a sub-graph."),
					Internal::GetNodeTitleText(*SelectedNode)));
			}
		}
	}

	void FCollapsePlan::GatherBoundaryPins()
	{
		for (UFlowGraphNode* SelectedNode : SelectedNodes)
		{
			for (UEdGraphPin* Pin : SelectedNode->Pins)
			{
				if (!Pin || Pin->LinkedTo.IsEmpty())
				{
					continue;
				}

				FBoundaryPin Boundary;
				for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
				{
					UFlowGraphNode* LinkedFlowNode = LinkedPin ? Cast<UFlowGraphNode>(LinkedPin->GetOwningNode()) : nullptr;
					if (LinkedPin && (!LinkedFlowNode || !SelectedNodes.Contains(LinkedFlowNode)))
					{
						Boundary.OuterPins.Add(LinkedPin);
					}
				}

				if (Boundary.OuterPins.IsEmpty())
				{
					continue;
				}

				// A sub-graph's interface is exec-only. Data has to be routed through AssetParams and the
				// output data pin declarations, which cannot be derived from the selection.
				if (!FFlowPin::IsExecPinCategory(Pin->PinType.PinCategory))
				{
					Errors.Add(FText::Format(
						LOCTEXT("DataPinCrossesBoundary", "The data pin '{0}' on '{1}' is connected outside the selection. Data cannot cross a sub-graph boundary."),
						FText::FromName(Pin->PinName), Internal::GetNodeTitleText(*SelectedNode)));
					continue;
				}

				Boundary.InnerPin = Pin;
				Boundary.InnerNodeGuid = SelectedNode->NodeGuid;
				Boundary.InnerPinName = Pin->PinName;

				TArray<FBoundaryPin>& TargetArray = Pin->Direction == EGPD_Input ? Entries : Exits;
				TargetArray.Add(MoveTemp(Boundary));
			}
		}
	}

	void FCollapsePlan::ConfigureEntry()
	{
		int32 EntryConnectionCount = 0;
		for (const FBoundaryPin& Entry : Entries)
		{
			EntryConnectionCount += Entry.OuterPins.Num();
		}

		if (Entries.Num() != 1 || EntryConnectionCount != 1)
		{
			Errors.Add(FText::Format(
				LOCTEXT("InvalidEntryCount", "The selection must have exactly one incoming execution connection, but it has {0}."),
				FText::AsNumber(EntryConnectionCount)));
			return;
		}

		Entries[0].InterfaceName = UFlowNode_SubGraph::StartPin.PinName;
		Entries[0].bUsesBuiltInPin = true;
	}

	// An exit leaving through an unnamed default pin has nothing to call a Custom Output after, so it
	// routes to the built-in Finish instead. Only one exit can do that: Finish is a single exec output on
	// the parent, and an exec output drives exactly one target, so a second unnamed exit would collide
	// with the first and get its link broken. Later unnamed exits fall back to a name from their node.
	void FCollapsePlan::ConfigureExits()
	{
		TSet<FName> UsedOutputNames;
		Internal::ReserveBuiltInPinNames(UsedOutputNames);
		bool bFinishPinClaimed = false;
		for (FBoundaryPin& Exit : Exits)
		{
			if (!bFinishPinClaimed && Internal::IsUnnamedExecPin(Exit.InnerPinName))
			{
				Exit.InterfaceName = UFlowNode_SubGraph::FinishPin.PinName;
				Exit.bUsesBuiltInPin = true;
				bFinishPinClaimed = true;
			}
			else
			{
				Exit.InterfaceName = Internal::MakeUniqueInterfaceName(Internal::DeriveBoundaryBaseName(Exit), UsedOutputNames);
			}
		}

		if (Exits.IsEmpty())
		{
			Warnings.Add(LOCTEXT("NoExits", "Nothing leaves the selection, so the new Sub Graph node will have no Custom Output pins."));
		}
	}

	/** Orders boundary pins top-to-bottom then left-to-right, so generated names read like the graph. */
	void FCollapsePlan::SortBoundaryPinsByPosition(TArray<FBoundaryPin>& BoundaryPins)
	{
		BoundaryPins.Sort([](const FBoundaryPin& Left, const FBoundaryPin& Right)
		{
			const UEdGraphNode* LeftNode = Left.InnerPin->GetOwningNode();
			const UEdGraphNode* RightNode = Right.InnerPin->GetOwningNode();

			if (LeftNode->NodePosY != RightNode->NodePosY)
			{
				return LeftNode->NodePosY < RightNode->NodePosY;
			}

			return LeftNode->NodePosX < RightNode->NodePosX;
		});
	}

	const FString& GetRequiredAssetNamePrefix()
	{
		return Internal::AssetNamePrefix;
	}

	FText ValidateNewAssetName(const UFlowAsset& SourceAsset, const FString& NewAssetName)
	{
		if (NewAssetName.IsEmpty())
		{
			return LOCTEXT("EmptyAssetName", "Enter a name for the sub-graph asset.");
		}
		if (!NewAssetName.StartsWith(Internal::AssetNamePrefix))
		{
			return FText::Format(LOCTEXT("MissingPrefix", "The name must start with '{0}'."), FText::FromString(Internal::AssetNamePrefix));
		}
		if (NewAssetName.Len() <= Internal::AssetNamePrefix.Len())
		{
			return LOCTEXT("PrefixOnly", "Add a name after the prefix.");
		}

		FText InvalidNameReason;
		if (!FName::IsValidXName(NewAssetName, INVALID_OBJECTNAME_CHARACTERS, &InvalidNameReason))
		{
			return InvalidNameReason;
		}

		const FString PackageFolder = FPackageName::GetLongPackagePath(SourceAsset.GetOutermost()->GetName());
		if (FPackageName::DoesPackageExist(PackageFolder / NewAssetName))
		{
			return FText::Format(LOCTEXT("AssetAlreadyExists", "'{0}' already exists in {1}."),
				FText::FromString(NewAssetName),
				FText::FromString(PackageFolder));
		}

		return FText::GetEmpty();
	}

	FCollapsePlan PlanCollapse(const TSet<UFlowGraphNode*>& SelectedNodes)
	{
		return FCollapsePlan(SelectedNodes);
	}
}

#undef LOCTEXT_NAMESPACE
