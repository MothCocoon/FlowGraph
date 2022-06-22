// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "EditorUndoClient.h"
#include "GraphEditor.h"
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
struct FSlateBrush;
struct FPropertyChangedEvent;
struct Rect;

class FFlowAssetEditor : public FAssetEditorToolkit, public FEditorUndoClient, public FGCObject, public FNotifyHook
{
	/** The FlowAsset asset being inspected */
	UFlowAsset* FlowAsset;

	TSharedPtr<class FFlowAssetToolbar> AssetToolbar;
	TSharedPtr<class FFlowDebugger> FlowDebugger;

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
	virtual ~FFlowAssetEditor() override;

	UFlowAsset* GetFlowAsset() const { return FlowAsset; };

	// FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override
	{
		return TEXT("FFlowAssetEditor");
	}
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

protected:
	virtual void CreateToolbar();

	virtual void BindToolbarCommands();
	virtual void RefreshAsset();
	virtual void GoToMasterInstance();
	virtual bool CanGoToMasterInstance();

	virtual void CreateWidgets();

	virtual TSharedRef<SGraphEditor> CreateGraphWidget();
	virtual FGraphAppearanceInfo GetGraphAppearanceInfo() const;
	virtual FText GetCornerText() const;

	virtual void BindGraphCommands();

private:
	static void UndoGraphAction();
	static void RedoGraphAction();

	static FReply OnSpawnGraphNodeByShortcut(FInputChord InChord, const FVector2D& InPosition, UEdGraph* InGraph);

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

protected:
	virtual void OnSelectedNodesChanged(const TSet<UObject*>& Nodes);

public:
	virtual void SelectSingleNode(UEdGraphNode* Node) const;

protected:
	virtual void SelectAllNodes() const;
	virtual bool CanSelectAllNodes() const;

	virtual void DeleteSelectedNodes();
	virtual void DeleteSelectedDuplicableNodes();
	virtual bool CanDeleteNodes() const;

	virtual void CopySelectedNodes() const;
	virtual bool CanCopyNodes() const;

	virtual void CutSelectedNodes();
	virtual bool CanCutNodes() const;

	virtual void PasteNodes();

public:
	virtual void PasteNodesHere(const FVector2D& Location);
	virtual bool CanPasteNodes() const;

protected:
	virtual void DuplicateNodes();
	virtual bool CanDuplicateNodes() const;

	virtual void OnNodeDoubleClicked(class UEdGraphNode* Node) const;
	virtual void OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged);

	virtual void RefreshContextPins() const;
	virtual bool CanRefreshContextPins() const;

private:
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

	void OnForcePinActivation() const;
	
	void FocusViewport() const;
	bool CanFocusViewport() const;

	void JumpToNodeDefinition() const;
	bool CanJumpToNodeDefinition() const;
};
