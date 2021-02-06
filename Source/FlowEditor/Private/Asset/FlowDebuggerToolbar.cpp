#include "Asset/FlowDebuggerToolbar.h"
#include "Asset/FlowAssetEditor.h"
#include "FlowEditorCommands.h"

#include "FlowAsset.h"

#include "EditorStyleSet.h"
#include "MultiBoxBuilder.h"
#include "Widgets/Navigation/SBreadcrumbTrail.h"

#define LOCTEXT_NAMESPACE "FlowDebuggerToolbar"

//////////////////////////////////////////////////////////////////////////
// Flow Instance List

FText SFlowInstanceList::NoInstanceSelectedText = LOCTEXT("NoInstanceSelected", "No instance selected");

void SFlowInstanceList::Construct(const FArguments& InArgs, TWeakPtr<FFlowAssetEditor> InFlowAssetEditor)
{
	FlowAssetEditor = InFlowAssetEditor;

	// collect instance names of this Flow Asset
	InstanceNames = {MakeShareable(new FName(*NoInstanceSelectedText.ToString()))};
	FlowAssetEditor.Pin()->GetFlowAsset()->GetInstanceDisplayNames(InstanceNames);

	// select instance
	if (const UFlowAsset* InspectedInstance = FlowAssetEditor.Pin()->GetFlowAsset()->GetInspectedInstance())
	{
		const FName& InspectedInstanceName = InspectedInstance->GetDisplayName();
		for (const TSharedPtr<FName>& Instance : InstanceNames)
		{
			if (*Instance == InspectedInstanceName)
			{
				SelectedInstance = Instance;
				break;
			}
		}
	}
	else
	{
		// default object is always available
		SelectedInstance = InstanceNames[0];
	}

	// create dropdown
	SAssignNew(Dropdown, SComboBox<TSharedPtr<FName>>)
		.OptionsSource(&InstanceNames)
		.OnGenerateWidget(this, &SFlowInstanceList::OnGenerateWidget)
		.OnSelectionChanged(this, &SFlowInstanceList::OnSelectionChanged)
		.Visibility_Static(&FFlowAssetEditor::GetDebuggerVisibility)
		[
			SNew(STextBlock)
				.Text(this, &SFlowInstanceList::GetSelectedInstanceName)
		];

	ChildSlot
	[
		Dropdown.ToSharedRef()
	];
}

TSharedRef<SWidget> SFlowInstanceList::OnGenerateWidget(const TSharedPtr<FName> Item) const
{
	return SNew(STextBlock).Text(FText::FromName(*Item.Get()));
}

void SFlowInstanceList::OnSelectionChanged(const TSharedPtr<FName> SelectedItem, const ESelectInfo::Type SelectionType)
{
	SelectedInstance = SelectedItem;

	if (FlowAssetEditor.IsValid() && FlowAssetEditor.Pin()->GetFlowAsset())
	{
		const FName NewSelectedInstanceName = (SelectedInstance.IsValid() && *SelectedInstance != *InstanceNames[0]) ? *SelectedInstance : NAME_None;
		FlowAssetEditor.Pin()->GetFlowAsset()->SetInspectedInstance(NewSelectedInstanceName);
	}
}

FText SFlowInstanceList::GetSelectedInstanceName() const
{
	return SelectedInstance.IsValid() ? FText::FromName(*SelectedInstance) : NoInstanceSelectedText;
}

//////////////////////////////////////////////////////////////////////////
// Flow Breadcrumb

void SFlowBreadcrumb::Construct(const FArguments& InArgs, TWeakPtr<FFlowAssetEditor> InFlowAssetEditor)
{
	FlowAssetEditor = InFlowAssetEditor;

	// create breadcrumb
	SAssignNew(BreadcrumbTrail, SBreadcrumbTrail<FFlowBreadcrumb>)
		.OnCrumbClicked(this, &SFlowBreadcrumb::OnCrumbClicked)
		.Visibility_Static(&FFlowAssetEditor::GetDebuggerVisibility)
		.ButtonStyle(FEditorStyle::Get(), "FlatButton")
		.DelimiterImage(FEditorStyle::GetBrush("Sequencer.BreadcrumbIcon"))
		.PersistentBreadcrumbs(true)
		.TextStyle(FEditorStyle::Get(), "Sequencer.BreadcrumbText");

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.AutoHeight()
			.Padding(25.0f, 0.0f)
			[
				BreadcrumbTrail.ToSharedRef()
			]
	];

	// fill breadcrumb
	BreadcrumbTrail->ClearCrumbs();
	if (UFlowAsset* InspectedInstance = FlowAssetEditor.Pin()->GetFlowAsset()->GetInspectedInstance())
	{
		TArray<UFlowAsset*> InstancesFromRoot = {InspectedInstance};

		UFlowAsset* CheckedInstance = InspectedInstance;
		while (UFlowAsset* MasterInstance = CheckedInstance->GetMasterInstance())
		{
			InstancesFromRoot.Insert(MasterInstance, 0);
			CheckedInstance = MasterInstance;
		}

		for (UFlowAsset* Instance : InstancesFromRoot)
		{
			TAttribute<FText> CrumbNameAttribute = MakeAttributeSP(this, &SFlowBreadcrumb::GetBreadcrumbText, Instance);
			BreadcrumbTrail->PushCrumb(CrumbNameAttribute, FFlowBreadcrumb(Instance));
		}
	}
}

void SFlowBreadcrumb::OnCrumbClicked(const FFlowBreadcrumb& Item)
{
	ensure(FlowAssetEditor.Pin()->GetFlowAsset()->GetInspectedInstance());

	if (Item.InstanceName != FlowAssetEditor.Pin()->GetFlowAsset()->GetInspectedInstance()->GetDisplayName())
	{
		FAssetEditorManager::Get().OpenEditorForAsset(Item.AssetPathName);
	}
}

FText SFlowBreadcrumb::GetBreadcrumbText(const TWeakObjectPtr<UFlowAsset> FlowInstance) const
{
	return FlowInstance.IsValid() ? FText::FromName(FlowInstance->GetDisplayName()) : FText();
}

//////////////////////////////////////////////////////////////////////////
// Flow Debugger Toolbar

FFlowDebuggerToolbar::FFlowDebuggerToolbar(TSharedPtr<FFlowAssetEditor> InNodeEditor)
	: FlowAssetEditor(InNodeEditor)
{
}

void FFlowDebuggerToolbar::AddToolbar(FToolBarBuilder& ToolbarBuilder)
{
	ToolbarBuilder.BeginSection("");
	{
		FlowInstanceList = SNew(SFlowInstanceList, FlowAssetEditor);
		ToolbarBuilder.AddWidget(FlowInstanceList.ToSharedRef());

		ToolbarBuilder.AddToolBarButton(FFlowToolbarCommands::Get().GoToMasterInstance);

		FlowBreadcrumb = SNew(SFlowBreadcrumb, FlowAssetEditor);
		ToolbarBuilder.AddWidget(FlowBreadcrumb.ToSharedRef());
	}
	ToolbarBuilder.EndSection();
}

#undef LOCTEXT_NAMESPACE
