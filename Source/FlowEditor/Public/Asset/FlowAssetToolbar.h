// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Widgets/Input/SComboBox.h"
#include "Widgets/Navigation/SBreadcrumbTrail.h"

#include "FlowAsset.h"

class FFlowAssetEditor;
class UToolMenu;

//////////////////////////////////////////////////////////////////////////
// Flow Asset Instance List

class FLOWEDITOR_API SFlowAssetInstanceList final : public SCompoundWidget
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
struct FLOWEDITOR_API FFlowBreadcrumb
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

class FLOWEDITOR_API SFlowAssetBreadcrumb final : public SCompoundWidget
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

class FLOWEDITOR_API FFlowAssetToolbar final : public TSharedFromThis<FFlowAssetToolbar>
{
public:
	explicit FFlowAssetToolbar(const TSharedPtr<FFlowAssetEditor> InAssetEditor, UToolMenu* ToolbarMenu);

private:
	void BuildAssetToolbar(UToolMenu* ToolbarMenu) const;
	TSharedRef<SWidget> MakeDiffMenu() const;
	
	void BuildDebuggerToolbar(UToolMenu* ToolbarMenu);

public:
	TSharedPtr<SFlowAssetInstanceList> GetAssetInstanceList() const { return AssetInstanceList; }

private:
	TWeakPtr<FFlowAssetEditor> FlowAssetEditor;

	TSharedPtr<SFlowAssetInstanceList> AssetInstanceList;
	TSharedPtr<SFlowAssetBreadcrumb> Breadcrumb;
};
