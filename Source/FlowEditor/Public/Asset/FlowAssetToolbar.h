// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Widgets/Input/SComboBox.h"
#include "Widgets/Navigation/SBreadcrumbTrail.h"

#include "FlowAsset.h"

class FFlowAssetEditor;

//////////////////////////////////////////////////////////////////////////
// Flow Asset Instance List

class SFlowAssetInstanceList final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFlowAssetInstanceList) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TWeakObjectPtr<UFlowAsset> InTemplateAsset);
	virtual ~SFlowAssetInstanceList() override;

private:
	void RefreshInstances();
	
	TSharedRef<SWidget> OnGenerateWidget(TSharedPtr<FName> Item) const;
	void OnSelectionChanged(TSharedPtr<FName> SelectedItem, ESelectInfo::Type SelectionType);
	FText GetSelectedInstanceName() const;

	TWeakObjectPtr<UFlowAsset> TemplateAsset;
	TSharedPtr<SComboBox<TSharedPtr<FName>>> Dropdown;

	TArray<TSharedPtr<FName>> InstanceNames;
	TSharedPtr<FName> SelectedInstance;

	static FText NoInstanceSelectedText;
};

//////////////////////////////////////////////////////////////////////////
// Flow Asset Breadcrumb

/**
 * The kind of breadcrumbs that Flow Debugger uses
 */
struct FFlowBreadcrumb
{
	FString AssetPathName;
	FName InstanceName;

	FFlowBreadcrumb()
		: AssetPathName(FString())
		, InstanceName(NAME_None)
	{}

	FFlowBreadcrumb(const UFlowAsset* FlowAsset)
		: AssetPathName(FlowAsset->GetTemplateAsset()->GetPathName())
		, InstanceName(FlowAsset->GetDisplayName())
	{}
};

class SFlowAssetBreadcrumb final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFlowAssetInstanceList) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TWeakObjectPtr<UFlowAsset> InTemplateAsset);

private:
	void OnCrumbClicked(const FFlowBreadcrumb& Item) const;
	FText GetBreadcrumbText(const TWeakObjectPtr<UFlowAsset> FlowInstance) const;

	TWeakObjectPtr<UFlowAsset> TemplateAsset;
	TSharedPtr<SBreadcrumbTrail<FFlowBreadcrumb>> BreadcrumbTrail;
};

//////////////////////////////////////////////////////////////////////////
// Flow Asset Toolbar

class FFlowAssetToolbar final : public TSharedFromThis<FFlowAssetToolbar>
{
public:
	explicit FFlowAssetToolbar(const TSharedPtr<FFlowAssetEditor> InAssetEditor, UToolMenu* ToolbarMenu);

private:
	void BuildAssetToolbar(UToolMenu* ToolbarMenu) const;
	void BuildDebuggerToolbar(UToolMenu* ToolbarMenu);

public:	
	TSharedPtr<SFlowAssetInstanceList> GetAssetInstanceList() const { return AssetInstanceList; }

private:
	TWeakPtr<FFlowAssetEditor> FlowAssetEditor;

	TSharedPtr<SFlowAssetInstanceList> AssetInstanceList;
	TSharedPtr<SFlowAssetBreadcrumb> Breadcrumb;
};
