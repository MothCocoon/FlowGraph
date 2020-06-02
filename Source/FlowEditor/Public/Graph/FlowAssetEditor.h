#pragma once

#include "CoreMinimal.h"
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
private:
	/** The FlowAsset asset being inspected */
	UFlowAsset* FlowAsset;

	TSharedPtr<SGraphEditor> FocusedGraphEditor;
	TSharedPtr<class IDetailsView> DetailsView;
	TSharedPtr<class SFlowPalette> Palette;
	TSharedPtr<FUICommandList> Commands;

	/**	The tab ids for all the tabs used */
	static const FName DetailsTabId;
	static const FName GraphCanvasTabId;
	static const FName PaletteTabId;

public:
	FFlowAssetEditor();
	virtual ~FFlowAssetEditor();

	UFlowAsset* GetFlowAsset() const { return FlowAsset; };

	// FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	// --

	// FEditorUndoClient
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }
	// --

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
	void InitFlowAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit);

private:
	void ExtendToolbar();

	void BindGraphCommands();
	void UndoGraphAction();
	void RedoGraphAction() const;

	void CreateInternalWidgets();
	TSharedRef<SGraphEditor> CreateGraphEditorWidget();

	FReply OnSpawnGraphNodeByShortcut(FInputChord InChord, const FVector2D& InPosition, UEdGraph* InGraph);
	void OnCreateComment() const;
	void OnStraightenConnections() const;

public:
	bool CanEdit() const;

	void SetSelection(TArray<UObject*> SelectedObjects) const;

	TSet<UObject*> GetSelectedNodes() const;
	int32 GetNumberOfSelectedNodes() const;
	bool GetBoundsForSelectedNodes(class FSlateRect& Rect, float Padding) const;

private:
	void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection) const;
	void SelectAllNodes() const;
	bool CanSelectAllNodes() const;

	void DeleteSelectedNodes() const;
	void DeleteSelectedDuplicatableNodes() const;
	bool CanDeleteNodes() const;

	void CopySelectedNodes() const;
	bool CanCopyNodes() const;

	void CutSelectedNodes() const;
	bool CanCutNodes() const;

	void PasteNodes() const;

public:
	void PasteNodesHere(const FVector2D& Location) const;
	bool CanPasteNodes() const;

private:
	void DuplicateNodes() const;
	bool CanDuplicateNodes() const;

	void OnNodeDoubleClicked(class UEdGraphNode* Node) const;
	void OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged);

	void RefreshContextInputs() const;
	bool CanRefreshContextInputs() const;

	void RefreshContextOutputs() const;
	bool CanRefreshContextOutputs() const;
	
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
};