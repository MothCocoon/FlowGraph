// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Widgets/Input/SComboBox.h"
#include "Widgets/Navigation/SBreadcrumbTrail.h"

#include "FlowAsset.h"

class FFlowAssetEditor;
class UFlowAssetEditorContext;
class UToolMenu;

//////////////////////////////////////////////////////////////////////////
// Flow Asset Instance Context

struct FFlowAssetInstanceContext
{
	FText DisplayText;
	TArray<TSharedPtr<FObjectKey>> AssetInstances;

	FFlowAssetInstanceContext()
	{
	}

	explicit FFlowAssetInstanceContext(const FText& InDisplayText)
		: DisplayText(InDisplayText)
	{
	}
};

//////////////////////////////////////////////////////////////////////////
// Flow Asset Instance List

class FLOWEDITOR_API SFlowAssetInstanceList : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFlowAssetInstanceList)
	{
	}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TWeakObjectPtr<UFlowAsset> InTemplateAsset);
	virtual ~SFlowAssetInstanceList() override;

	static EVisibility GetDebuggerVisibility();

protected:
	void RefreshInstances();

	EVisibility GetContextVisibility() const;
	TSharedRef<SWidget> OnGenerateContextWidget(TSharedPtr<FObjectKey> Item);
	void OnContextSelectionChanged(TSharedPtr<FObjectKey> SelectedItem, ESelectInfo::Type SelectionType);
	FText GetSelectedContextName() const;

	TSharedRef<SWidget> OnGenerateInstanceWidget(TSharedPtr<FObjectKey> Item) const;
	void OnInstanceSelectionChanged(TSharedPtr<FObjectKey> SelectedItem, ESelectInfo::Type SelectionType);
	FText GetSelectedInstanceName() const;
	FText JoinInstanceAndContextTexts(const FObjectKey& AssetInstance) const;

	TWeakObjectPtr<UFlowAsset> TemplateAsset;

	TSharedPtr<SComboBox<TSharedPtr<FObjectKey>>> ContextComboBox;
	TSharedPtr<SComboBox<TSharedPtr<FObjectKey>>> InstanceComboBox;

	TArray<TSharedPtr<FObjectKey>> Contexts;
	TArray<TSharedPtr<FObjectKey>> Instances;
	TMap<FObjectKey, FFlowAssetInstanceContext> InstancesPerContext;

	TSharedPtr<FObjectKey> NoContext;
	TSharedPtr<FObjectKey> SelectedContext;
	TSharedPtr<FObjectKey> SelectedInstance;
	
	static FText AllContextsText;
	static FText NoInstanceSelectedText;
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
	{
	}

	explicit FFlowBreadcrumb(const TWeakObjectPtr<const UFlowAsset> InCurrentInstance, const TWeakObjectPtr<const UFlowAsset> InChildInstance)
		: CurrentInstance(InCurrentInstance)
		, ChildInstance(InChildInstance)
	{
	}
};

class FLOWEDITOR_API SFlowAssetBreadcrumb : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFlowAssetInstanceList)
	{
	}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TWeakObjectPtr<UFlowAsset> InTemplateAsset);

private:
	EVisibility GetBreadcrumbVisibility() const;
	void FillBreadcrumb() const;
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
