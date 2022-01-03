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

	virtual ~SFlowAssetInstanceList() override;
	
	void Construct(const FArguments& InArgs, TWeakPtr<FFlowAssetEditor> InFlowAssetEditor);

	void SetSelectedInstanceName(FName NewInstanceName);
	
private:
	TSharedRef<SWidget> OnGenerateWidget(TSharedPtr<FName> Item) const;
	void OnComboBoxOpening();
	void OnSelectionChanged(TSharedPtr<FName> SelectedItem, ESelectInfo::Type SelectionType);
	FText GetSelectedInstanceName() const;

	void RefreshInstanceNames();
	void ResetSelectedInstance(const bool bSimulateInEditor);
	
	TWeakPtr<FFlowAssetEditor> FlowAssetEditor;
	TSharedPtr<SComboBox<TSharedPtr<FName>>> Dropdown;

	TArray<TSharedPtr<FName>> NamePtrCache;
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
	SLATE_BEGIN_ARGS(SFlowAssetInstanceList)	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<FFlowAssetEditor> InFlowAssetEditor);

private:
	void OnCrumbClicked(const FFlowBreadcrumb& Item) const;
	FText GetBreadcrumbText(const TWeakObjectPtr<UFlowAsset> FlowInstance) const;

	TWeakPtr<FFlowAssetEditor> FlowAssetEditor;
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
