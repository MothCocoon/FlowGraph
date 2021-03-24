#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Navigation/SBreadcrumbTrail.h"

#include "FlowAsset.h"

class FExtender;
class FToolBarBuilder;
class SWidget;

class FFlowAssetEditor;

//////////////////////////////////////////////////////////////////////////
// Flow Instance List

class SFlowInstanceList final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFlowInstanceList) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<FFlowAssetEditor> InFlowAssetEditor);

private:
	TSharedRef<SWidget> OnGenerateWidget(TSharedPtr<FName> Item) const;
	void OnSelectionChanged(TSharedPtr<FName> SelectedItem, ESelectInfo::Type SelectionType);
	FText GetSelectedInstanceName() const;

	TWeakPtr<FFlowAssetEditor> FlowAssetEditor;
	TSharedPtr<SComboBox<TSharedPtr<FName>>> Dropdown;

	TArray<TSharedPtr<FName>> InstanceNames;
	TSharedPtr<FName> SelectedInstance;

	static FText NoInstanceSelectedText;
};

//////////////////////////////////////////////////////////////////////////
// Flow Breadcrumb

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
		: AssetPathName(FlowAsset->TemplateAsset->GetPathName())
		, InstanceName(FlowAsset->GetDisplayName())
	{}
};

class SFlowBreadcrumb final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFlowInstanceList)	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<FFlowAssetEditor> InFlowAssetEditor);

private:
	void OnCrumbClicked(const FFlowBreadcrumb& Item);
	FText GetBreadcrumbText(const TWeakObjectPtr<UFlowAsset> FlowInstance) const;

	TWeakPtr<FFlowAssetEditor> FlowAssetEditor;
	TSharedPtr<SBreadcrumbTrail<FFlowBreadcrumb>> BreadcrumbTrail;
};

//////////////////////////////////////////////////////////////////////////
// Flow Debugger Toolbar

class FFlowDebuggerToolbar final : public TSharedFromThis<FFlowDebuggerToolbar>
{
public:
	FFlowDebuggerToolbar(TSharedPtr<FFlowAssetEditor> InNodeEditor);

	void AddToolbar(FToolBarBuilder& ToolbarBuilder);
	TSharedPtr<SFlowInstanceList> GetFlowInstanceList() const { return FlowInstanceList; }

private:
	TWeakPtr<FFlowAssetEditor> FlowAssetEditor;

	TSharedPtr<SFlowInstanceList> FlowInstanceList;
	TSharedPtr<SFlowBreadcrumb> FlowBreadcrumb;
};
