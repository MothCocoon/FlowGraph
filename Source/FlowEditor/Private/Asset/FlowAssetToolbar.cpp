#include "Asset/FlowAssetToolbar.h"
#include "Asset/FlowAssetEditor.h"
#include "FlowEditorCommands.h"

#include "FlowAsset.h"

#include "EditorStyleSet.h"
#include "Kismet2/DebuggerCommands.h"
#include "Misc/Attribute.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FlowDebuggerToolbar"

//////////////////////////////////////////////////////////////////////////
// Flow Asset Instance List

FText SFlowAssetInstanceList::NoInstanceSelectedText = LOCTEXT("NoInstanceSelected", "No instance selected");

void SFlowAssetInstanceList::Construct(const FArguments& InArgs, TWeakPtr<FFlowAssetEditor> InFlowAssetEditor)
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
		.OnGenerateWidget(this, &SFlowAssetInstanceList::OnGenerateWidget)
		.OnSelectionChanged(this, &SFlowAssetInstanceList::OnSelectionChanged)
		.Visibility_Static(&FFlowAssetEditor::GetDebuggerVisibility)
		[
			SNew(STextBlock)
			.Text(this, &SFlowAssetInstanceList::GetSelectedInstanceName)
		];

	ChildSlot
	[
		Dropdown.ToSharedRef()
	];
}

TSharedRef<SWidget> SFlowAssetInstanceList::OnGenerateWidget(const TSharedPtr<FName> Item) const
{
	return SNew(STextBlock).Text(FText::FromName(*Item.Get()));
}

void SFlowAssetInstanceList::OnSelectionChanged(const TSharedPtr<FName> SelectedItem, const ESelectInfo::Type SelectionType)
{
	SelectedInstance = SelectedItem;

	if (FlowAssetEditor.IsValid() && FlowAssetEditor.Pin()->GetFlowAsset())
	{
		const FName NewSelectedInstanceName = (SelectedInstance.IsValid() && *SelectedInstance != *InstanceNames[0]) ? *SelectedInstance : NAME_None;
		FlowAssetEditor.Pin()->GetFlowAsset()->SetInspectedInstance(NewSelectedInstanceName);
	}
}

FText SFlowAssetInstanceList::GetSelectedInstanceName() const
{
	return SelectedInstance.IsValid() ? FText::FromName(*SelectedInstance) : NoInstanceSelectedText;
}

//////////////////////////////////////////////////////////////////////////
// Flow Asset Breadcrumb

void SFlowAssetBreadcrumb::Construct(const FArguments& InArgs, TWeakPtr<FFlowAssetEditor> InFlowAssetEditor)
{
	FlowAssetEditor = InFlowAssetEditor;

	// create breadcrumb
	SAssignNew(BreadcrumbTrail, SBreadcrumbTrail<FFlowBreadcrumb>)
		.OnCrumbClicked(this, &SFlowAssetBreadcrumb::OnCrumbClicked)
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
		.Padding(25.0f, 10.0f)
		[
			BreadcrumbTrail.ToSharedRef()
		]
	];

	// fill breadcrumb
	BreadcrumbTrail->ClearCrumbs();
	if (UFlowAsset* InspectedInstance = FlowAssetEditor.Pin()->GetFlowAsset()->GetInspectedInstance())
	{
		TArray<UFlowAsset*> InstancesFromRoot = {InspectedInstance};

		const UFlowAsset* CheckedInstance = InspectedInstance;
		while (UFlowAsset* MasterInstance = CheckedInstance->GetMasterInstance())
		{
			InstancesFromRoot.Insert(MasterInstance, 0);
			CheckedInstance = MasterInstance;
		}

		for (UFlowAsset* Instance : InstancesFromRoot)
		{
			TAttribute<FText> CrumbNameAttribute = MakeAttributeSP(this, &SFlowAssetBreadcrumb::GetBreadcrumbText, Instance);
			BreadcrumbTrail->PushCrumb(CrumbNameAttribute, FFlowBreadcrumb(Instance));
		}
	}
}

void SFlowAssetBreadcrumb::OnCrumbClicked(const FFlowBreadcrumb& Item) const
{
	ensure(FlowAssetEditor.Pin()->GetFlowAsset()->GetInspectedInstance());

	if (Item.InstanceName != FlowAssetEditor.Pin()->GetFlowAsset()->GetInspectedInstance()->GetDisplayName())
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Item.AssetPathName);
	}
}

FText SFlowAssetBreadcrumb::GetBreadcrumbText(const TWeakObjectPtr<UFlowAsset> FlowInstance) const
{
	return FlowInstance.IsValid() ? FText::FromName(FlowInstance->GetDisplayName()) : FText();
}

//////////////////////////////////////////////////////////////////////////
// Flow Asset Toolbar

FFlowAssetToolbar::FFlowAssetToolbar(const TSharedPtr<FFlowAssetEditor> InAssetEditor, UToolMenu* ToolbarMenu)
	: FlowAssetEditor(InAssetEditor)
{
	BuildAssetToolbar(ToolbarMenu);
	BuildDebuggerToolbar(ToolbarMenu);
}

void FFlowAssetToolbar::BuildAssetToolbar(UToolMenu* ToolbarMenu) const
{
	FToolMenuSection& Section = ToolbarMenu->AddSection("Editing");
	Section.InsertPosition = FToolMenuInsert("Asset", EToolMenuInsertType::After);
	
	Section.AddEntry(FToolMenuEntry::InitToolBarButton(FFlowToolbarCommands::Get().RefreshAsset));
}

void FFlowAssetToolbar::BuildDebuggerToolbar(UToolMenu* ToolbarMenu)
{
	FToolMenuSection& Section = ToolbarMenu->AddSection("Debugging");
	Section.InsertPosition = FToolMenuInsert("Asset", EToolMenuInsertType::After);
	
	FPlayWorldCommands::BuildToolbar(Section);
	
	AssetInstanceList = SNew(SFlowAssetInstanceList, FlowAssetEditor);
	Section.AddEntry(FToolMenuEntry::InitWidget("AssetInstances", AssetInstanceList.ToSharedRef(), FText(), true));

	Section.AddEntry(FToolMenuEntry::InitToolBarButton(FFlowToolbarCommands::Get().GoToMasterInstance));

	Breadcrumb = SNew(SFlowAssetBreadcrumb, FlowAssetEditor);
	Section.AddEntry(FToolMenuEntry::InitWidget("AssetBreadcrumb", Breadcrumb.ToSharedRef(), FText(), true));
}

#undef LOCTEXT_NAMESPACE
