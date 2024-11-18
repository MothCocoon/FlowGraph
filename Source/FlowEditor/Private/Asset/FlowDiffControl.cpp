// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowDiffControl.h"
#include "Asset/SFlowDiff.h"

#include "FlowAsset.h"

#include "EdGraph/EdGraph.h"
#include "GraphDiffControl.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "SBlueprintDiff.h"

#define LOCTEXT_NAMESPACE "SFlowDiffControl"

namespace
{
	FString GetDiffKeyFromDiffResult(const FDiffResultItem& Difference)
	{
		if (Difference.Result.Pin1)
		{
			return Difference.Result.Pin1->PinId.ToString() + Difference.Result.Pin2->PinId.ToString();
		}

		const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Difference.Result.Node1);
		if (!IsValid(FlowGraphNode))
		{
			FlowGraphNode = Cast<UFlowGraphNode>(Difference.Result.Node2);
		}

		if (IsValid(FlowGraphNode) && IsValid(FlowGraphNode->GetNodeTemplate()))
		{
			return FString::FromInt(FlowGraphNode->GetNodeTemplate()->GetUniqueID()); //->GetName();
		}

		if (IsValid(Difference.Result.Node1))
		{
			return FString::FromInt(Difference.Result.Node1->GetUniqueID());
		}

		if (IsValid(Difference.Result.Node2))
		{
			return FString::FromInt(Difference.Result.Node2->GetUniqueID());
		}

		return TEXT("Invalid Diff!");
	}

	FString GetNodeNameFromDiffResult(const FDiffResultItem& Difference)
	{
		if (Difference.Result.Pin1)
		{
			return TEXT("Invalid Diff!");
		}

		const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Difference.Result.Node1);
		if (!IsValid(FlowGraphNode))
		{
			FlowGraphNode = Cast<UFlowGraphNode>(Difference.Result.Node2);
		}

		if (IsValid(FlowGraphNode) && IsValid(FlowGraphNode->GetNodeTemplate()))
		{
			return FlowGraphNode->GetNodeTemplate()->GetName();
		}

		if (IsValid(Difference.Result.Node1))
		{
			return Difference.Result.Node1->GetName();
		}

		if (IsValid(Difference.Result.Node2))
		{
			return Difference.Result.Node2->GetName();
		}

		return TEXT("Invalid Diff!");
	}

	void ModifyDiffDisplayName(FDiffSingleResult& InOutResult, const FString& DiffKey)
	{
		//Only modify the DisplayStrings of node changes, because they are not very descriptive.
		//Other generated DisplayStrings seem fine, such as pin connection changes.
		if ((IsValid(InOutResult.Node1) || IsValid(InOutResult.Node2))
			&& InOutResult.Pin1 == nullptr)
		{
			FString AdditionalDescription;
			switch (InOutResult.Category)
			{
			case EDiffType::Category::SUBTRACTION:
				AdditionalDescription = TEXT("Removed ");
				break;
			case EDiffType::Category::ADDITION:
				AdditionalDescription = TEXT("Added ");
				break;
			default:
				break;
			}

			InOutResult.DisplayString = FText::FromString(AdditionalDescription + DiffKey);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
/// FFlowAssetDiffControl

FFlowAssetDiffControl::FFlowAssetDiffControl(const UFlowAsset* InOldFlowAsset, const UFlowAsset* InNewFlowAsset, FOnDiffEntryFocused InSelectionCallback)
	: FDetailsDiffControl(InOldFlowAsset, InNewFlowAsset, InSelectionCallback, false)
{
}

// TDetailsDiffControl::GenerateTreeEntries + "NoDifferences" entry + category label
void FFlowAssetDiffControl::GenerateTreeEntries(TArray<TSharedPtr<FBlueprintDifferenceTreeEntry>>& OutTreeEntries, TArray<TSharedPtr<FBlueprintDifferenceTreeEntry>>& OutRealDifferences)
{
	FDetailsDiffControl::GenerateTreeEntries(OutTreeEntries, OutRealDifferences);

	const bool bHasDifferences = Children.Num() != 0;
	if (!bHasDifferences)
	{
		// make one child informing the user that there are no differences:
		Children.Push(FBlueprintDifferenceTreeEntry::NoDifferencesEntry());
	}

	static const FText AssetPropertiesLabel = LOCTEXT("AssetPropertiesLabel", "Properties");
	static const FText AssetPropertiesTooltip = LOCTEXT("AssetPropertiesTooltip", "The list of changes made to Flow Asset properties.");
	OutTreeEntries.Push(FBlueprintDifferenceTreeEntry::CreateCategoryEntry(
		AssetPropertiesLabel,
		AssetPropertiesTooltip,
		SelectionCallback,
		Children,
		bHasDifferences
	));
}

/////////////////////////////////////////////////////////////////////////////
/// FFlowGraphToDiff

FFlowGraphToDiff::FFlowGraphToDiff(SFlowDiff* InDiffWidget, UEdGraph* InGraphOld, UEdGraph* InGraphNew, const FRevisionInfo& InRevisionOld, const FRevisionInfo& InRevisionNew)
	: FoundDiffs(MakeShared<TArray<FDiffSingleResult>>())
	, DiffWidget(InDiffWidget)
	, GraphOld(InGraphOld)
	, GraphNew(InGraphNew)
	, RevisionOld(InRevisionOld)
	, RevisionNew(InRevisionNew)
{
	check(InGraphOld || InGraphNew);

	if (InGraphNew)
	{
		OnGraphChangedDelegateHandle = InGraphNew->AddOnGraphChangedHandler(FOnGraphChanged::FDelegate::CreateRaw(this, &FFlowGraphToDiff::OnGraphChanged));
	}

	BuildDiffSourceArray();
}

FFlowGraphToDiff::~FFlowGraphToDiff()
{
	if (GraphNew)
	{
		GraphNew->RemoveOnGraphChangedHandler(OnGraphChangedDelegateHandle);
	}
}

ENodeDiffType FFlowGraphToDiff::GetNodeDiffType(const UEdGraphNode& Node) const
{
	if (IsValid(GraphOld) && Node.GetGraph() == GraphOld)
	{
		return ENodeDiffType::Old;
	}

	if (IsValid(GraphNew) && Node.GetGraph() == GraphNew)
	{
		return ENodeDiffType::New;
	}

	return ENodeDiffType::Invalid;
}

TSharedPtr<FFlowObjectDiff> FFlowGraphToDiff::GetFlowObjectDiff(const FDiffResultItem& DiffResultItem)
{
	const FString DiffKey = GetDiffKeyFromDiffResult(DiffResultItem);
	check(FlowObjectDiffsByNodeName.Contains(DiffKey));
	return *FlowObjectDiffsByNodeName.Find(DiffKey);
}

void FFlowGraphToDiff::GenerateTreeEntries(TArray<TSharedPtr<FBlueprintDifferenceTreeEntry>>& OutTreeEntries, TArray<TSharedPtr<FBlueprintDifferenceTreeEntry>>& OutRealDifferences)
{
	if (!DiffListSource.IsEmpty())
	{
		RealDifferencesStartIndex = OutRealDifferences.Num();
	}

	check(DiffWidget != nullptr);
	TArray<TSharedPtr<FBlueprintDifferenceTreeEntry>> Children;

	for (const TSharedPtr<FDiffResultItem>& Difference : DiffListSource)
	{
		FString DiffKey = GetDiffKeyFromDiffResult(*Difference.Get());
		const TSharedPtr<FFlowObjectDiff>* FlowNodeDiff = FlowObjectDiffsByNodeName.Find(DiffKey);

		//generate a FlowObjectDiff if not found
		if (FlowNodeDiff == nullptr)
		{
			TSharedPtr<FFlowObjectDiff> GenerateNodeEntry = GenerateFlowObjectDiff(Difference);
			FlowNodeDiff = &FlowObjectDiffsByNodeName.Add(DiffKey, GenerateNodeEntry);
		}

		//defer generating certain child tree entries, so they can go at the bottom of the list.
		if (Difference->Result.Diff == EDiffType::Type::NODE_MOVED
			|| Difference->Result.Diff == EDiffType::Type::NODE_COMMENT)
		{
			(*FlowNodeDiff)->LowPriorityChildDiffResult.Add(Difference);
		}

		//Use the highest priority category for the parent diff for the purpose of color.
		//Note that a lower category is a higher priority one.
		//One issue is that every change (even moves or comments) trigger a "Modification" DiffResult, so we need to
		//handle that special case separately. Only allow a "Modification" DiffResult to change colors if there is
		//actually a property change, or if a sub node was added/removed.
		if (Difference->Result.Category != EDiffType::Category::MODIFICATION
			&& (*FlowNodeDiff)->DiffResult->Result.Category > Difference->Result.Category)
		{
			(*FlowNodeDiff)->DiffResult->Result.Diff = Difference->Result.Diff;
			(*FlowNodeDiff)->DiffResult->Result.Category = Difference->Result.Category;
		}
	}

	//loop through the generated FlowObjectDiffs to:
	//a) add DiffTreeEntries to their correct place in the tree,
	//b) generate property diffs,
	//c) set DiffType and Category to change colors where appropriate.
	for (const auto& Nodes : FlowObjectDiffsByNodeName)
	{
		TSharedPtr<FFlowObjectDiff> FlowNodeDiff = Nodes.Value;
		OutRealDifferences.Push(FlowNodeDiff->DiffTreeEntry);

		UEdGraphNode* Node1 = FlowNodeDiff->DiffResult->Result.Node1;
		//not a node diff
		if (!IsValid(Node1))
		{
			Children.Push(FlowNodeDiff->DiffTreeEntry);
			continue;
		}

		FlowNodeDiff->ParentNodeDiff = FindParentNode(Cast<UFlowGraphNode>(Node1));
		if (FlowNodeDiff->ParentNodeDiff.IsValid())
		{
			const TSharedPtr<FFlowObjectDiff> ParentNode = FlowNodeDiff->ParentNodeDiff.Pin();
			ParentNode->DiffTreeEntry->Children.Push(FlowNodeDiff->DiffTreeEntry);

			//if a parent has any sub node changes,  make sure the color does not stay as "minor"
			if (ParentNode->DiffResult->Result.Category > FlowNodeDiff->DiffResult->Result.Category)
			{
				ParentNode->DiffResult->Result.Diff = EDiffType::Type::NODE_PROPERTY;
				ParentNode->DiffResult->Result.Category = EDiffType::Category::MODIFICATION;
			}
		}
		else
		{
			Children.Push(FlowNodeDiff->DiffTreeEntry);
		}

		//find and generate  property diffs.
		TArray<FSingleObjectDiffEntry> PropertyDiffsArray;
		FlowNodeDiff->DiffProperties(PropertyDiffsArray);

		//generate property diff tree entries.
		for (const auto& PropertyDiffEntry : PropertyDiffsArray)
		{
			check(FlowNodeDiff.IsValid());
			TSharedPtr<FFlowObjectDiffArgs>& FlowObjectPropertyDiff = FlowNodeDiff->PropertyDiffArgList.Add_GetRef(MakeShared<FFlowObjectDiffArgs>(FlowNodeDiff, PropertyDiffEntry));

			TSharedPtr<FBlueprintDifferenceTreeEntry> ChildEntry = MakeShared<FBlueprintDifferenceTreeEntry>(
				FOnDiffEntryFocused::CreateSP(DiffWidget, &SFlowDiff::OnDiffListSelectionChanged, FlowObjectPropertyDiff),
				FGenerateDiffEntryWidget::CreateStatic(&GenerateObjectDiffWidget, FlowObjectPropertyDiff->PropertyDiff, RightRevision));

			Nodes.Value->DiffTreeEntry->Children.Push(ChildEntry);
			OutRealDifferences.Push(ChildEntry);
		}

		//if a property changed, allow it to change the color of it's parent tree entry. 
		if (PropertyDiffsArray.Num() > 0)
		{
			if (FlowNodeDiff->DiffResult->Result.Category > EDiffType::Category::MODIFICATION)
			{
				FlowNodeDiff->DiffResult->Result.Diff = EDiffType::Type::NODE_PROPERTY;
				FlowNodeDiff->DiffResult->Result.Category = EDiffType::Category::MODIFICATION;
			}
		}
	}

	//generate low priority child tree entries. This for loop should go after generating other tree entries, so that
	//these lower priority child trees are at the bottom.
	for (const auto& Nodes : FlowObjectDiffsByNodeName)
	{
		const TSharedPtr<FFlowObjectDiff> FlowNodeDiff = Nodes.Value;

		for (const auto& Difference : Nodes.Value->LowPriorityChildDiffResult)
		{
			TSharedPtr<FBlueprintDifferenceTreeEntry> ChildEntry = MakeShared<FBlueprintDifferenceTreeEntry>(
				FOnDiffEntryFocused::CreateRaw(DiffWidget, &SFlowDiff::OnDiffListSelectionChanged, FlowNodeDiff->DiffEntryFocusArg),
				FGenerateDiffEntryWidget::CreateSP(Difference.ToSharedRef(), &FDiffResultItem::GenerateWidget));

			FlowNodeDiff->DiffTreeEntry->Children.Push(ChildEntry);
			OutRealDifferences.Push(ChildEntry);
		}
	}

	if (Children.Num() == 0)
	{
		// make one child informing the user that there are no differences:
		Children.Push(FBlueprintDifferenceTreeEntry::NoDifferencesEntry());
	}

	const TSharedPtr<FBlueprintDifferenceTreeEntry> Entry = MakeShared<FBlueprintDifferenceTreeEntry>(
		FOnDiffEntryFocused::CreateRaw(DiffWidget, &SFlowDiff::OnGraphSelectionChanged, TSharedPtr<FFlowGraphToDiff>(AsShared()), ESelectInfo::Direct),
		FGenerateDiffEntryWidget::CreateSP(AsShared(), &FFlowGraphToDiff::GenerateCategoryWidget),
		Children);

	OutTreeEntries.Push(Entry);
}

TSharedPtr<FFlowObjectDiff> FFlowGraphToDiff::GenerateFlowObjectDiff(const TSharedPtr<FDiffResultItem>& Difference)
{
	//copy the first diff found in order to enable jumping to the node in the graph.
	TSharedPtr<FDiffResultItem> DuplicatedFirstFoundDiffResult = MakeShared<FDiffResultItem>(Difference->Result);

	ModifyDiffDisplayName(DuplicatedFirstFoundDiffResult->Result, GetNodeNameFromDiffResult(*Difference.Get()));

	TSharedPtr<FFlowObjectDiff> NewFlowObjectDiff = MakeShared<FFlowObjectDiff>(DuplicatedFirstFoundDiffResult, *this);

	static const FSingleObjectDiffEntry InvalidDiff = FSingleObjectDiffEntry(); //do not specify a property change for the main object diff itself. 
	NewFlowObjectDiff->DiffEntryFocusArg = MakeShared<FFlowObjectDiffArgs>(
		NewFlowObjectDiff,
		FSingleObjectDiffEntry(InvalidDiff));

	NewFlowObjectDiff->DiffTreeEntry = MakeShared<FBlueprintDifferenceTreeEntry>(
		FOnDiffEntryFocused::CreateRaw(DiffWidget, &SFlowDiff::OnDiffListSelectionChanged, NewFlowObjectDiff->DiffEntryFocusArg),
		FGenerateDiffEntryWidget::CreateSP(DuplicatedFirstFoundDiffResult.ToSharedRef(), &FDiffResultItem::GenerateWidget));

	return NewFlowObjectDiff;
}

TSharedPtr<FFlowObjectDiff> FFlowGraphToDiff::FindParentNode(UFlowGraphNode* Node)
{
	if (!IsValid(Node))
	{
		return nullptr;
	}

	const UFlowGraphNode* ParentNode = Node->GetParentNode();
	for (auto& FlowNodeDiff : FlowObjectDiffsByNodeName)
	{
		//don't allow a pin diff be the parent of anything.
		if (FlowNodeDiff.Value->DiffResult->Result.Pin1)
		{
			continue;
		}
		//if parent node is set, use that.
		if (IsValid(ParentNode))
		{
			if (FlowNodeDiff.Value->DiffResult->Result.Node1 == ParentNode
				|| FlowNodeDiff.Value->DiffResult->Result.Node2 == ParentNode)
			{
				return FlowNodeDiff.Value;
			}
		}
		//if parent node is not set (not set in node removal changes for some reason),
		//try to find the parent in the SubNodes of known node changes.
		else
		{
			const UFlowGraphNode* NodeToCheck = Cast<UFlowGraphNode>(FlowNodeDiff.Value->DiffResult->Result.Node1);
			if (IsValid(NodeToCheck))
			{
				const int32 Index = NodeToCheck->SubNodes.Find(Node);
				if (Index != INDEX_NONE)
				{
					return FlowNodeDiff.Value;
				}
			}
		}
	}

	return nullptr;
}

FText FFlowGraphToDiff::GetToolTip() const
{
	if (GraphOld && GraphNew)
	{
		if (DiffListSource.Num() > 0)
		{
			return LOCTEXT("ContainsDifferences", "Revisions are different");
		}
		else
		{
			return LOCTEXT("GraphsIdentical", "Revisions appear to be identical");
		}
	}
	else
	{
		const UEdGraph* GoodGraph = GraphOld ? GraphOld : GraphNew;
		check(GoodGraph);
		const FRevisionInfo& Revision = GraphNew ? RevisionOld : RevisionNew;
		FText RevisionText = LOCTEXT("CurrentRevision", "Current Revision");

		if (!Revision.Revision.IsEmpty())
		{
			RevisionText = FText::Format(LOCTEXT("Revision Number", "Revision {0}"), FText::FromString(Revision.Revision));
		}

		return FText::Format(LOCTEXT("MissingGraph", "Graph '{0}' missing from {1}"), FText::FromString(GoodGraph->GetName()), RevisionText);
	}
}

TSharedRef<SWidget> FFlowGraphToDiff::GenerateCategoryWidget() const
{
	const UEdGraph* Graph = GraphOld ? GraphOld : GraphNew;
	check(Graph);

	FLinearColor Color = (GraphOld && GraphNew) ? DiffViewUtils::Identical() : FLinearColor(0.3f, 0.3f, 1.f);

	if (DiffListSource.Num() > 0)
	{
		Color = DiffViewUtils::Differs();
	}

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.ColorAndOpacity(Color)
			.Text(FText::FromString(TEXT("Graph")))
			.ToolTipText(GetToolTip())
		]
		+ DiffViewUtils::Box(GraphOld != nullptr, Color)
		+ DiffViewUtils::Box(GraphNew != nullptr, Color);
}

void FFlowGraphToDiff::BuildDiffSourceArray()
{
	FoundDiffs->Empty();
	FGraphDiffControl::DiffGraphs(GraphOld, GraphNew, *FoundDiffs);

	Algo::SortBy(*FoundDiffs, &FDiffSingleResult::Diff);

	DiffListSource.Empty();
	for (const FDiffSingleResult& Diff : *FoundDiffs)
	{
		DiffListSource.Add(MakeShared<FDiffResultItem>(Diff));
	}
}

void FFlowGraphToDiff::OnGraphChanged(const FEdGraphEditAction& Action) const
{
	DiffWidget->OnGraphChanged(this);
}

#undef LOCTEXT_NAMESPACE
