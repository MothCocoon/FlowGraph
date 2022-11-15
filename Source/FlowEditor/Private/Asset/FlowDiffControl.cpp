// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowDiffControl.h"
#include "Asset/SFlowDiff.h"

#include "FlowAsset.h"

#include "GraphDiffControl.h"
#include "SBlueprintDiff.h"

#define LOCTEXT_NAMESPACE "SFlowDiffControl"

/////////////////////////////////////////////////////////////////////////////
/// FFlowAssetDiffControl

FFlowAssetDiffControl::FFlowAssetDiffControl(const UFlowAsset* InOldFlowAsset, const UFlowAsset* InNewFlowAsset, FOnDiffEntryFocused InSelectionCallback)
	: TDetailsDiffControl(InOldFlowAsset, InNewFlowAsset, InSelectionCallback)
{
}

// TDetailsDiffControl::GenerateTreeEntries + "NoDifferences" entry + category label
void FFlowAssetDiffControl::GenerateTreeEntries(TArray<TSharedPtr<FBlueprintDifferenceTreeEntry>>& OutTreeEntries, TArray<TSharedPtr<FBlueprintDifferenceTreeEntry>>& OutRealDifferences)
{
	TDetailsDiffControl::GenerateTreeEntries(OutTreeEntries, OutRealDifferences);

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

void FFlowGraphToDiff::GenerateTreeEntries(TArray<TSharedPtr<FBlueprintDifferenceTreeEntry>>& OutTreeEntries, TArray<TSharedPtr<FBlueprintDifferenceTreeEntry>>& OutRealDifferences)
{
	if (!DiffListSource.IsEmpty())
	{
		RealDifferencesStartIndex = OutRealDifferences.Num();
	}

	TArray<TSharedPtr<FBlueprintDifferenceTreeEntry>> Children;
	for (const TSharedPtr<FDiffResultItem>& Difference : DiffListSource)
	{
		TSharedPtr<FBlueprintDifferenceTreeEntry> ChildEntry = MakeShared<FBlueprintDifferenceTreeEntry>(
			FOnDiffEntryFocused::CreateRaw(DiffWidget, &SFlowDiff::OnDiffListSelectionChanged, Difference),
			FGenerateDiffEntryWidget::CreateSP(Difference.ToSharedRef(), &FDiffResultItem::GenerateWidget));
		Children.Push(ChildEntry);
		OutRealDifferences.Push(ChildEntry);
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

	const bool bHasDiffs = DiffListSource.Num() > 0;

	if (bHasDiffs)
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

	struct SortDiff
	{
		bool operator ()(const FDiffSingleResult& A, const FDiffSingleResult& B) const
		{
			return A.Diff < B.Diff;
		}
	};

	Sort(FoundDiffs->GetData(), FoundDiffs->Num(), SortDiff());

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
