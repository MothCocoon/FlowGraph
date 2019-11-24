#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "Misc/NotifyHook.h"
#include "Toolkits/IToolkitHost.h"
#include "UObject/GCObject.h"

#include "IFlowAssetEditor.h"

class SFlowPalette;
class UFlowAsset;

class IDetailsView;
class SDockableTab;
class SGraphEditor;
class UEdGraphNode;
struct FPropertyChangedEvent;
struct Rect;

class FFlowAssetEditor : public IFlowAssetEditor, public FGCObject, public FNotifyHook, public FEditorUndoClient
{
public:
	FFlowAssetEditor();

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;

	/** Destructor */
	virtual ~FFlowAssetEditor();

	/** Edits the specified FlowAsset object */
	void InitFlowAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit);

	/** IFlowAssetEditor interface */
	virtual UFlowAsset* GetFlowAsset() const override;
	virtual void SetSelection(TArray<UObject*> SelectedObjects) override;
	virtual bool GetBoundsForSelectedNodes(class FSlateRect& Rect, float Padding) override;
	virtual int32 GetNumberOfSelectedNodes() const override;
	virtual TSet<UObject*> GetSelectedNodes() const override;
	
	/** IToolkit interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	/** FGCObject interface */
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	//~ Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }
	// End of FEditorUndoClient

private:
	TSharedRef<SDockTab> SpawnTab_GraphCanvas(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Properties(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Palette(const FSpawnTabArgs& Args);

protected:
	FReply OnSpawnGraphNodeByShortcut(FInputChord InChord, const FVector2D& InPosition, UEdGraph* InGraph);

	void OnNodeDoubleClicked(class UEdGraphNode* Node);
	void OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged);

	void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection);
	void SelectAllNodes();
	bool CanSelectAllNodes() const;

	void DeleteSelectedNodes();
	void DeleteSelectedDuplicatableNodes();
	bool CanDeleteNodes() const;

	void CutSelectedNodes();
	bool CanCutNodes() const;

	void CopySelectedNodes();
	bool CanCopyNodes() const;

	void PasteNodes();
	virtual void PasteNodesHere(const FVector2D& Location) override;
	virtual bool CanPasteNodes() const override;

	void DuplicateNodes();
	bool CanDuplicateNodes() const;

	void UndoGraphAction();
	void RedoGraphAction();

private:
	/** FNotifyHook interface */
	virtual void NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged) override;

	void CreateInternalWidgets();
	void ExtendToolbar();
	void BindGraphCommands();

	void AddInput();
	bool CanAddInput() const;
	void DeleteInput();
	bool CanDeleteInput() const;

	void AddOutput();
	bool CanAddOutput() const;
	void DeleteOutput();
	bool CanDeleteOutput() const;

	void FocusViewport();
	bool CanFocusViewport() const;

	void OnCreateComment();

	/** Create new graph editor widget */
	TSharedRef<SGraphEditor> CreateGraphEditorWidget();

private:
	/** The FlowAsset asset being inspected */
	UFlowAsset* FlowAsset;

	/** List of open tool panels; used to ensure only one exists at any one time */
	TMap<FName, TWeakPtr<SDockableTab> > SpawnedToolPanels;

	TSharedPtr<SGraphEditor> FlowGraphEditor;
	TSharedPtr<class IDetailsView> FlowAssetProperties;
	TSharedPtr<class SFlowPalette> Palette;
	TSharedPtr<FUICommandList> GraphEditorCommands;

	/**	The tab ids for all the tabs used */
	static const FName GraphCanvasTabId;
	static const FName PropertiesTabId;
	static const FName PaletteTabId;
};
