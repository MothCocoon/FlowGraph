// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "IDetailsView.h"
#include "DiffResults.h"
#include "GraphEditor.h"
#include "SDetailsDiff.h"
#include "Textures/SlateIcon.h"

struct FFlowGraphToDiff;
struct FFlowObjectDiffArgs;
class IDetailsView;
class SSplitter2x2;
class UFlowAsset;

enum class EAssetEditorCloseReason : uint8;

namespace FlowDiffUtils
{
	FLOWEDITOR_API void SelectNextRow(SListView<TSharedPtr<struct FDiffSingleResult>>& ListView, const TArray<TSharedPtr<struct FDiffSingleResult>>& ListViewSource);
	FLOWEDITOR_API void SelectPrevRow(SListView<TSharedPtr<struct FDiffSingleResult>>& ListView, const TArray<TSharedPtr<struct FDiffSingleResult>>& ListViewSource);
	FLOWEDITOR_API bool HasNextDifference(const SListView<TSharedPtr<struct FDiffSingleResult>>& ListView, const TArray<TSharedPtr<struct FDiffSingleResult>>& ListViewSource);
	FLOWEDITOR_API bool HasPrevDifference(const SListView<TSharedPtr<struct FDiffSingleResult>>& ListView, const TArray<TSharedPtr<struct FDiffSingleResult>>& ListViewSource);
}

/** Panel used to display the asset */
struct FLOWEDITOR_API FFlowDiffPanel
{
	FFlowDiffPanel();

	/** Generate a panel for NewGraph diffed against OldGraph */
	void GeneratePanel(UEdGraph* NewGraph, UEdGraph* OldGraph);

	/** Generate a panel that displays the Graph and reflects the items in the DiffResults */
	void GeneratePanel(UEdGraph* Graph, TSharedPtr<TArray<FDiffSingleResult>> DiffResults, TAttribute<int32> FocusedDiffResult);

	/** Called when user hits keyboard shortcut to copy nodes */
	void CopySelectedNodes() const;

	/** Gets whatever nodes are selected in the Graph Editor */
	FGraphPanelSelectionSet GetSelectedNodes() const;

	/** Can user copy any of the selected nodes? */
	bool CanCopyNodes() const;

	/** Functions used to focus/find a particular change in a diff result */
	void FocusDiff(const UEdGraphPin& Pin) const;
	void FocusDiff(const UEdGraphNode& Node) const;

	/** The Flow Asset that owns the graph we are showing */
	const UFlowAsset* FlowAsset;

	/** The box around the graph editor, used to change the content when new graphs are set */
	TSharedPtr<SBox> GraphEditorBox;

	/** using SNullWidget::NullNullWidget can only work for a single widget, since widget instances can only be
	 * used one at a time. EmptyDetailsView is used for displaying an empty details panel instead. */
	TSharedPtr<IDetailsView> EmptyDetailsView;

	/** The graph editor which does the work of displaying the graph */
	TWeakPtr<class SGraphEditor> GraphEditor;

	/** Revision information for this asset */
	FRevisionInfo RevisionInfo;

	/** True if we should show a name identifying which asset this panel is displaying */
	bool bShowAssetName;

	/** The widget that contains the revision info in graph mode */
	TSharedPtr<SWidget> OverlayGraphRevisionInfo;
private:
	/** Command list for this diff panel */
	TSharedPtr<FUICommandList> GraphEditorCommands;
};

/* Visual Diff between two Flow Assets */
class FLOWEDITOR_API SFlowDiff : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_TwoParams(FOpenInDefaults, const class UFlowAsset*, const class UFlowAsset*);

	SLATE_BEGIN_ARGS(SFlowDiff)
		{
		}

		SLATE_ARGUMENT(const class UFlowAsset*, OldFlow)
		SLATE_ARGUMENT(const class UFlowAsset*, NewFlow)
		SLATE_ARGUMENT(struct FRevisionInfo, OldRevision)
		SLATE_ARGUMENT(struct FRevisionInfo, NewRevision)
		SLATE_ARGUMENT(bool, ShowAssetNames)
		SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SFlowDiff() override;

	/** Called when a new Graph is clicked on by user */
	void OnGraphChanged(const FFlowGraphToDiff* Diff);

	/** Called when user clicks on a new graph list item */
	void OnGraphSelectionChanged(const TSharedPtr<FFlowGraphToDiff> Item, ESelectInfo::Type SelectionType);

	/** Called when user clicks on an entry in the listview of differences */
	void OnDiffListSelectionChanged(TSharedPtr<FFlowObjectDiffArgs> FlowObjectDiffArgs);

	/** Helper function for generating an empty widget */
	static TSharedRef<SWidget> DefaultEmptyPanel();

	/** Helper function to create a window that holds a diff widget */
	static TSharedPtr<SWindow> CreateDiffWindow(const FText WindowTitle, const UFlowAsset* OldFlow, const UFlowAsset* NewFlow, const struct FRevisionInfo& OldRevision, const struct FRevisionInfo& NewRevision);

protected:
	/** Called when user clicks button to go to next difference */
	void NextDiff() const;

	/** Called when user clicks button to go to prev difference */
	void PrevDiff() const;

	/** Called to determine whether we have a list of differences to cycle through */
	bool HasNextDiff() const;
	bool HasPrevDiff() const;

	/** Find the FGraphToDiff that displays the graph with GraphPath relative path */
	FFlowGraphToDiff* FindGraphToDiffEntry(const FString& GraphPath) const;

	/** Bring these revisions of graph into focus on main display*/
	void FocusOnGraphRevisions(const FFlowGraphToDiff* Diff);

	/** User toggles the option to lock the views between the two assets */
	void OnToggleLockView();

	/** User toggles the option to change the split view mode between vertical and horizontal */
	void OnToggleSplitViewMode();

	/** Reset the graph editor, called when user switches graphs to display*/
	void ResetGraphEditors() const;

	/** Get the image to show for the toggle lock option*/
	FSlateIcon GetLockViewImage() const;

	/** Get the image to show for the toggle split view mode option*/
	FSlateIcon GetSplitViewModeImage() const;

	/** List of graphs to diff, are added to panel last */
	TSharedPtr<FFlowGraphToDiff> GraphToDiff;

	/** Get Graph editor associated with this Graph */
	FFlowDiffPanel& GetDiffPanelForNode(const UEdGraphNode& Node);

	/** Event handler that updates the graph view when user selects a new graph */
	void HandleGraphChanged(const FString& GraphPath);

	/** Function used to generate the list of differences and the widgets needed to calculate that list */
	void GenerateDifferencesList();

	/** Called when editor may need to be closed */
	void OnCloseAssetEditor(UObject* Asset, const EAssetEditorCloseReason CloseReason);

	struct FDiffControl
	{
		FDiffControl()
			: Widget()
			, DiffControl(nullptr)
		{
		}

		TSharedPtr<SWidget> Widget;
		TSharedPtr<class IDiffControl> DiffControl;
	};

	FDiffControl GenerateDetailsPanel();
	FDiffControl GenerateGraphPanel();

	TSharedRef<SOverlay> GenerateGraphWidgetForPanel(FFlowDiffPanel& OutDiffPanel) const;
	TSharedRef<SBox> GenerateRevisionInfoWidgetForPanel(TSharedPtr<SWidget>& OutGeneratedWidget, const FText& InRevisionText) const;

	/** Accessor and event handler for toggling between diff view modes (defaults, components, graph view, interface, macro): */
	void SetCurrentMode(FName NewMode);
	FName GetCurrentMode() const { return CurrentMode; }
	void OnModeChanged(const FName& InNewViewMode) const;

	void UpdateTopSectionVisibility(const FName& InNewViewMode) const;

	FName CurrentMode;

	/*The two panels used to show the old & new revision*/
	FFlowDiffPanel PanelOld, PanelNew;

	/** If the two views should be locked */
	bool bLockViews;

	/** If the view on Graph Mode should be divided vertically */
	bool bVerticalSplitGraphMode = true;

	/** Contents widget that we swap when mode changes (defaults, components, etc) */
	TSharedPtr<SBox> ModeContents;

	TSharedPtr<SSplitter> TopRevisionInfoWidget;
	TSharedPtr<SSplitter> DiffGraphSplitter;
	TSharedPtr<SSplitter> GraphToolBarWidget;

	friend struct FListItemGraphToDiff;

	/** We can't use the global tab manager because we need to instance the diff control, so we have our own tab manager: */
	TSharedPtr<FTabManager> TabManager;

	/** Tree of differences collected across all panels: */
	TArray<TSharedPtr<class FBlueprintDifferenceTreeEntry>> PrimaryDifferencesList;

	/** List of all differences, cached so that we can iterate only the differences and not labels, etc: */
	TArray<TSharedPtr<class FBlueprintDifferenceTreeEntry>> RealDifferences;

	/** Tree view that displays the differences, cached for the buttons that iterate the differences: */
	TSharedPtr<STreeView<TSharedPtr<FBlueprintDifferenceTreeEntry>>> DifferencesTreeView;

	/** Stored references to widgets used to display various parts of a asset, from the mode name */
	TMap<FName, FDiffControl> ModePanels;

	/** A pointer to the window holding this */
	TWeakPtr<SWindow> WeakParentWindow;

	TSharedPtr<SSplitter2x2> GraphDiffSplitter = nullptr;

	FDelegateHandle AssetEditorCloseDelegate;
};
