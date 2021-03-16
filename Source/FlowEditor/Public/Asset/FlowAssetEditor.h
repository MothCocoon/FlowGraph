#pragma once

#include "EditorUndoClient.h"
#include "Misc/NotifyHook.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/IToolkitHost.h"
#include "UObject/GCObject.h"

class SFlowPalette;
class UFlowAsset;
class UFlowGraphNode;

class IDetailsView;
class SDockableTab;
class SGraphEditor;
class UEdGraphNode;
struct FPropertyChangedEvent;
struct Rect;

class FFlowAssetEditor : public FAssetEditorToolkit,
						public FEditorUndoClient,
						public FGCObject,
						public FNotifyHook
{
	/** The FlowAsset asset being inspected */
	UFlowAsset* FlowAsset;

	TSharedPtr<class FFlowAssetToolbar> AssetToolbar;
	TSharedPtr<class FFlowDebugger> Debugger;
	TSharedPtr<class FFlowDebuggerToolbar> DebuggerToolbar;

	TSharedPtr<SGraphEditor> FocusedGraphEditor;
	TSharedPtr<class IDetailsView> DetailsView;
	TSharedPtr<class SFlowPalette> Palette;

public:
	/**	The tab ids for all the tabs used */
	static const FName DetailsTab;
	static const FName GraphTab;
	static const FName PaletteTab;

private:
	/** The current UI selection state of this editor */
	FName CurrentUISelection;

public:
	FFlowAssetEditor();
	virtual ~FFlowAssetEditor();

	UFlowAsset* GetFlowAsset() const { return FlowAsset; };

	// FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	// --

	// FEditorUndoClient
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	// --

	virtual void HandleUndoTransaction();

	// FNotifyHook
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;
	// --

	// IToolkit
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	// --

private:
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> SpawnTab_GraphCanvas(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> SpawnTab_Palette(const FSpawnTabArgs& Args) const;

public:
	/** Edits the specified FlowAsset object */
	void InitFlowAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, UObject* ObjectToEdit);

private:
	void AddFlowAssetToolbar();
	void BindAssetCommands();

	void AddPlayWorldToolbar() const;

	void CreateFlowDebugger();
	void BindDebuggerCommands();

protected:
	virtual void RefreshAsset();

	virtual void GoToMasterInstance();
	virtual bool CanGoToMasterInstance();

private:
	void CreateWidgets();
	TSharedRef<SGraphEditor> CreateGraphWidget();

protected:
	FGraphAppearanceInfo GetGraphAppearanceInfo() const;
	virtual FText GetCornerText() const;

private:
	void BindGraphCommands();
	void UndoGraphAction();
	void RedoGraphAction();

	FReply OnSpawnGraphNodeByShortcut(FInputChord InChord, const FVector2D& InPosition, UEdGraph* InGraph);

public:
	/** Gets the UI selection state of this editor */
	FName GetUISelectionState() const { return CurrentUISelection; }
	void SetUISelectionState(const FName SelectionOwner);

	virtual void ClearSelectionStateFor(const FName SelectionOwner);

private:
	void OnCreateComment() const;
	void OnStraightenConnections() const;

public:
	static bool CanEdit();
	static bool IsPIE();
	static EVisibility GetDebuggerVisibility();

	TSet<UFlowGraphNode*> GetSelectedFlowNodes() const;
	int32 GetNumberOfSelectedNodes() const;
	bool GetBoundsForSelectedNodes(class FSlateRect& Rect, float Padding) const;

private:
	void OnSelectedNodesChanged(const TSet<UObject*>& Nodes);

public:
	void SelectSingleNode(UEdGraphNode* Node) const;

private:
	void SelectAllNodes() const;
	bool CanSelectAllNodes() const;

	void DeleteSelectedNodes();
	void DeleteSelectedDuplicatableNodes();
	bool CanDeleteNodes() const;

	void CopySelectedNodes() const;
	bool CanCopyNodes() const;

	void CutSelectedNodes();
	bool CanCutNodes() const;

	void PasteNodes();

public:
	void PasteNodesHere(const FVector2D& Location);
	bool CanPasteNodes() const;

private:
	void DuplicateNodes();
	bool CanDuplicateNodes() const;

	void OnNodeDoubleClicked(class UEdGraphNode* Node) const;
	void OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged);

	void RefreshContextPins() const;
	bool CanRefreshContextPins() const;

	void AddInput() const;
	bool CanAddInput() const;

	void AddOutput() const;
	bool CanAddOutput() const;

	void RemovePin() const;
	bool CanRemovePin() const;

	void OnAddBreakpoint() const;
	void OnAddPinBreakpoint() const;

	bool CanAddBreakpoint() const;
	bool CanAddPinBreakpoint() const;

	void OnRemoveBreakpoint() const;
	void OnRemovePinBreakpoint() const;

	bool CanRemoveBreakpoint() const;
	bool CanRemovePinBreakpoint() const;

	void OnEnableBreakpoint() const;
	void OnEnablePinBreakpoint() const;

	bool CanEnableBreakpoint() const;
	bool CanEnablePinBreakpoint() const;

	void OnDisableBreakpoint() const;
	void OnDisablePinBreakpoint() const;

	bool CanDisableBreakpoint() const;
	bool CanDisablePinBreakpoint() const;

	void OnToggleBreakpoint() const;
	void OnTogglePinBreakpoint() const;

	bool CanToggleBreakpoint() const;
	bool CanTogglePinBreakpoint() const;

	void FocusViewport() const;
	bool CanFocusViewport() const;

	void JumpToNodeDefinition() const;
	bool CanJumpToNodeDefinition() const;
};
