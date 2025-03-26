// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Widgets/Input/SComboBox.h"
#include "Widgets/Navigation/SBreadcrumbTrail.h"

#include "FlowAsset.h"

class UFlowNode_SubGraph;
class FFlowAssetEditor;
class UFlowAssetEditorContext;
class UToolMenu;

struct FFlowDebugWorld
{
	/** Actual World object */
	TWeakObjectPtr<const UWorld> WorldPtr;
	
	/** Friendly label for debug world */
    FString WorldLabel;

	FFlowDebugWorld(const TWeakObjectPtr<const UWorld>& InWorldPtr, const FString& InWorldLabel)
		: WorldPtr(InWorldPtr)
		, WorldLabel(InWorldLabel)
	{
	}

	/** Returns true if this is the special entry for no specific world */
	bool IsEmptyObject() const
	{
		return WorldPtr.IsExplicitlyNull();
	}
};

struct FFlowDebugInstance
{
	/** Actual FlowAsset instance */
	TWeakObjectPtr<const UFlowAsset> InstancePtr;

	/** Friendly label for debug instance */
	FString InstanceLabel;

	FFlowDebugInstance(const TWeakObjectPtr<const UFlowAsset>& InInstancePtr, const FString& InInstanceLabel)
		: InstancePtr(InInstancePtr)
		, InstanceLabel(InInstanceLabel)
	{
	}

	/** Returns true if this is the special entry for no specific instance */
	bool IsEmptyObject() const
	{
		return InstancePtr.IsExplicitlyNull();
	}
};

//////////////////////////////////////////////////////////////////////////
// Flow Asset Instance List

class FLOWEDITOR_API SFlowAssetInstanceList : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFlowAssetInstanceList) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TWeakObjectPtr<UFlowAsset> InTemplateAsset);
	virtual ~SFlowAssetInstanceList() override;

private:
	static EVisibility GetWorldComboVisibility();

	void GenerateDebugWorldNames();
	TSharedRef<SWidget> GenerateWorldItemWidget(TSharedPtr<FFlowDebugWorld> Item) const;
	void DebugWorldSelectionChanged(TSharedPtr<FFlowDebugWorld> SelectedItem, ESelectInfo::Type SelectionType);
	FText GetSelectedWorldName() const;
	TSharedPtr<FFlowDebugWorld> GetDebugWorld() const;

	void GenerateDebugInstances();
	TSharedRef<SWidget> GenerateInstanceItemWidget(TSharedPtr<FFlowDebugInstance> Item) const;
	void DebugInstanceSelectionChanged(TSharedPtr<FFlowDebugInstance> SelectedItem, ESelectInfo::Type SelectionType);
	FText GetSelectedInstanceName() const;
	TSharedPtr<FFlowDebugInstance> GetDebugInstance() const;

	
	TWeakObjectPtr<UFlowAsset> TemplateAsset;
	
	TSharedPtr<SComboBox<TSharedPtr<FFlowDebugWorld>>> DebugWorldsComboBox;
	TSharedPtr<SComboBox<TSharedPtr<FFlowDebugInstance>>> DebugInstancesComboBox;

	TArray<TSharedPtr<FFlowDebugWorld>> DebugWorlds;
	TArray<TSharedPtr<FFlowDebugInstance>> DebugInstances;

	static FText NoInstanceSelectedText;
	static FText AllWorldsText;
};

//////////////////////////////////////////////////////////////////////////
// Flow Asset Breadcrumb

/**
 * The kind of breadcrumbs that Flow Debugger uses
 */
struct FLOWEDITOR_API FFlowBreadcrumb
{
	const TWeakObjectPtr<const UFlowAsset> CurrentInstance;
	const TWeakObjectPtr<const UFlowAsset> ChildInstance;

	FFlowBreadcrumb()
		: CurrentInstance(nullptr)
		, ChildInstance(nullptr)
	{}

	explicit FFlowBreadcrumb(const TWeakObjectPtr<const UFlowAsset> InCurrentInstance, const TWeakObjectPtr<const UFlowAsset> InChildInstance)
		: CurrentInstance(InCurrentInstance)
		, ChildInstance(InChildInstance)
	{}
};

class FLOWEDITOR_API SFlowAssetBreadcrumb : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFlowAssetInstanceList) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TWeakObjectPtr<UFlowAsset> InTemplateAsset);

private:
	EVisibility GetBreadcrumbVisibility() const;
	void FillBreadcrumb();
	void OnCrumbClicked(const FFlowBreadcrumb& Item) const;

	TWeakObjectPtr<UFlowAsset> TemplateAsset;
	TSharedPtr<SBreadcrumbTrail<FFlowBreadcrumb>> BreadcrumbTrail;
};

//////////////////////////////////////////////////////////////////////////
// Flow Asset Toolbar

class FLOWEDITOR_API FFlowAssetToolbar : public TSharedFromThis<FFlowAssetToolbar>
{
public:
	explicit FFlowAssetToolbar(const TSharedPtr<FFlowAssetEditor> InAssetEditor, UToolMenu* ToolbarMenu);

private:
	void BuildAssetToolbar(UToolMenu* ToolbarMenu) const;
	static TSharedRef<SWidget> MakeDiffMenu(const UFlowAssetEditorContext* InContext);
	
	void BuildDebuggerToolbar(UToolMenu* ToolbarMenu) const;

private:
	TWeakPtr<FFlowAssetEditor> FlowAssetEditor;
};
