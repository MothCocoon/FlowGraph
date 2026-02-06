// Fill out your copyright notice in the Description page of Project Settings.


#include "Graph/Nodes/FlowGraphNode_NamedReroute.h"

#include "FlowEditorCommands.h"
#include "Graph/FlowGraphEditor.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/FlowGraphUtils.h"
#include "Nodes/Route/FlowNode_NamedReroute.h"

#define LOCTEXT_NAMESPACE "FlowGraphEditor"

UFlowGraphNode_NamedRerouteDeclaration::UFlowGraphNode_NamedRerouteDeclaration(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssignedNodeClasses = {UFlowNode_NamedRerouteDeclaration::StaticClass()};
	bCanRenameNode = true;
}

void UFlowGraphNode_NamedRerouteDeclaration::GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);
	
	const FFlowGraphCommands& FlowGraphCommands = FFlowGraphCommands::Get();
	if (Context->Node)
	{
		FToolMenuSection& Section = Menu->FindOrAddSection("FlowGraphNodeActions");

		{
			const TAttribute<FText> Label = LOCTEXT("FlowGraphNodeActions_CreateRerouteUsageFromDeclaration_Label", "Create Reroute Usage From Declaration");
			const TAttribute<FText> ToolTip = LOCTEXT("FlowGraphNodeActions_CreateRerouteUsageFromDeclaration_Tooltip", "Create a reroute usage from the declaration.");
			const FSlateIcon Icon = FSlateIcon();

			FToolUIAction UIAction;
			UIAction.ExecuteAction = FToolMenuExecuteAction::CreateWeakLambda(this, [this](const FToolMenuContext&){OnCreateRerouteUsageFromDeclaration();});
			UIAction.CanExecuteAction = FToolMenuCanExecuteAction::CreateWeakLambda(this, [this](const FToolMenuContext&){return CanCreateRerouteUsageFromDeclaration();});
			Section.AddMenuEntry(TEXT("CreateRerouteUsageFromDeclaration"), Label, ToolTip, Icon, UIAction);
		}

		{
			const TAttribute<FText> Label = LOCTEXT("FlowGraphNodeActions_SelectNamedRerouteUsages_Label", "Select Named Reroute Usages");
			const TAttribute<FText> ToolTip = LOCTEXT("FlowGraphNodeActions_SelectNamedRerouteUsages_Tooltip", "Select the named reroute usages.");
			const FSlateIcon Icon = FSlateIcon();

			FToolUIAction UIAction;
			UIAction.ExecuteAction = FToolMenuExecuteAction::CreateWeakLambda(this, [this](const FToolMenuContext&){OnSelectNamedRerouteUsages();});
			UIAction.CanExecuteAction = FToolMenuCanExecuteAction::CreateWeakLambda(this, [this](const FToolMenuContext&){return CanSelectNamedRerouteUsages();});
			Section.AddMenuEntry(TEXT("SelectNamedRerouteUsages"), Label, ToolTip, Icon, UIAction);
		}
	}
}

void UFlowGraphNode_NamedRerouteDeclaration::OnNodeDoubleClicked() const
{
	OnSelectNamedRerouteUsages();
}

void UFlowGraphNode_NamedRerouteDeclaration::OnRenameNode(const FString& NewName)
{
	Super::OnRenameNode(NewName);

	if (auto* Declaration = Cast<UFlowNode_NamedRerouteDeclaration>(GetFlowNodeBase()))
	{
		Declaration->Modify();
		Declaration->SetEditableName(NewName);
		Declaration->MarkPackageDirty();

		// Refresh usage names
		for(UFlowNode* FlowNode : GetFlowAsset()->GetAllNodes())
		{
			auto* Usage = Cast<UFlowNode_NamedRerouteUsage>(FlowNode);
			if (Usage && Usage->Declaration == Declaration)
			{
				if (UEdGraphNode* UsageGraphNode = Usage->GetGraphNode())
				{
					UsageGraphNode->ReconstructNode();
				}
			}
		}
	}
}

void UFlowGraphNode_NamedRerouteDeclaration::OnCreateRerouteUsageFromDeclaration() const
{
	TSharedPtr<SFlowGraphEditor> FlowGraphEditor = FFlowGraphUtils::GetFlowGraphEditor(GetGraph());
	if (!FlowGraphEditor.IsValid())
	{
		return;
	}
	
	FlowGraphEditor->ClearSelectionSet();
	if (auto* Declaration = Cast<UFlowNode_NamedRerouteDeclaration>(GetFlowNodeBase()))
	{
		FFlowGraphSchemaAction_NewNamedRerouteUsage UsageAction;
		UsageAction.Declaration = Declaration;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 6
		UsageAction.PerformAction(GetGraph(), nullptr, FVector2f(NodePosX + 150.f, NodePosY), true);
#else
		UsageAction.PerformAction(GetGraph(), nullptr, FVector2f(NodePosX + 150.f, NodePosY), true);
#endif
	}
	FlowGraphEditor->ZoomToFit(true);
}

bool UFlowGraphNode_NamedRerouteDeclaration::CanCreateRerouteUsageFromDeclaration() const
{
	TSharedPtr<SFlowGraphEditor> FlowGraphEditor = FFlowGraphUtils::GetFlowGraphEditor(GetGraph());
	if (!FlowGraphEditor.IsValid())
	{
		return false;
	}
	
	if (FlowGraphEditor->GetSelectedFlowNodes().Num() == 1)
	{
		for (const UFlowGraphNode* SelectedNode : FlowGraphEditor->GetSelectedFlowNodes())
		{
			return SelectedNode == this;
		}
	}
	return false;
}

void UFlowGraphNode_NamedRerouteDeclaration::OnSelectNamedRerouteUsages() const
{
	TSharedPtr<SFlowGraphEditor> FlowGraphEditor = FFlowGraphUtils::GetFlowGraphEditor(GetGraph());
	if (!FlowGraphEditor.IsValid())
	{
		return;
	}
	
	FlowGraphEditor->ClearSelectionSet();
	if (auto* Declaration = Cast<UFlowNode_NamedRerouteDeclaration>(GetFlowNodeBase()))
	{
		//TArray<UEdGraphNode*> FoundGraphNodes;
		for(UFlowNode* FlowNode : GetFlowAsset()->GetAllNodes())
		{
			auto* Usage = Cast<UFlowNode_NamedRerouteUsage>(FlowNode);
			if (Usage && Usage->Declaration == Declaration)
			{
				if (UEdGraphNode* UsageGraphNode = Usage->GetGraphNode())
				{
					//FoundGraphNodes.Add(UsageGraphNode);
					FlowGraphEditor->SetNodeSelection(UsageGraphNode, true);
				}
			}
		}

		//Add usage results into SearchTab
		// if (FoundGraphNodes.Num())
		// {
		// 	// Spawn the tab in case the user doesn't have it open
		// 	TabManager->TryInvokeTab(FFlowEditorTabs::FindTabId);
		//
		// 	FindResults->PopulateSearchItems(FoundGraphNodes);
		// 	FindResults->FocusForUse();
		// }
	}
	FlowGraphEditor->ZoomToFit(true);
}

bool UFlowGraphNode_NamedRerouteDeclaration::CanSelectNamedRerouteUsages() const
{
	TSharedPtr<SFlowGraphEditor> FlowGraphEditor = FFlowGraphUtils::GetFlowGraphEditor(GetGraph());
	if (!FlowGraphEditor.IsValid())
	{
		return false;
	}
	
	if (FlowGraphEditor->GetSelectedFlowNodes().Num() == 1)
	{
		for (const UFlowGraphNode* SelectedNode : FlowGraphEditor->GetSelectedFlowNodes())
		{
			return SelectedNode == this;
		}
	}
	return false;
}

UFlowGraphNode_NamedRerouteUsage::UFlowGraphNode_NamedRerouteUsage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssignedNodeClasses = {UFlowNode_NamedRerouteUsage::StaticClass()};
}

void UFlowGraphNode_NamedRerouteUsage::GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);

	const FFlowGraphCommands& FlowGraphCommands = FFlowGraphCommands::Get();
	if (Context->Node)
	{
		FToolMenuSection& Section = Menu->FindOrAddSection("FlowGraphNodeActions");
		{
			const TAttribute<FText> Label = LOCTEXT("FlowGraphNodeActions_SelectNamedRerouteDeclaration_Label", "Select Named Reroute Declaration");
			const TAttribute<FText> ToolTip = LOCTEXT("FlowGraphNodeActions_SelectNamedRerouteDeclaration_Tooltip", "Select the named reroute declaration.");
			const FSlateIcon Icon = FSlateIcon();

			FToolUIAction UIAction;
			UIAction.ExecuteAction = FToolMenuExecuteAction::CreateWeakLambda(this, [this](const FToolMenuContext&){OnSelectNamedRerouteDeclaration();});
			UIAction.CanExecuteAction = FToolMenuCanExecuteAction::CreateWeakLambda(this, [this](const FToolMenuContext&){return CanSelectNamedRerouteDeclaration();});
			Section.AddMenuEntry(TEXT("SelectNamedRerouteDeclaration"), Label, ToolTip, Icon, UIAction);
		}
	}
}

void UFlowGraphNode_NamedRerouteUsage::OnNodeDoubleClicked() const
{
	OnSelectNamedRerouteDeclaration();
}

void UFlowGraphNode_NamedRerouteUsage::OnSelectNamedRerouteDeclaration() const
{
	TSharedPtr<SFlowGraphEditor> FlowGraphEditor = FFlowGraphUtils::GetFlowGraphEditor(GetGraph());
	if (!FlowGraphEditor.IsValid())
	{
		return;
	}
	
	FlowGraphEditor->ClearSelectionSet();
	if (auto* Usage = Cast<UFlowNode_NamedRerouteUsage>(GetFlowNodeBase()))
	{
		if (IsValid(Usage->Declaration))
		{
			if (UEdGraphNode* DeclarationGraphNode = Usage->Declaration->GetGraphNode())
			{
				FlowGraphEditor->SetNodeSelection(DeclarationGraphNode, true);
			}
		}
	}
	FlowGraphEditor->ZoomToFit(true);
}

bool UFlowGraphNode_NamedRerouteUsage::CanSelectNamedRerouteDeclaration() const
{
	TSharedPtr<SFlowGraphEditor> FlowGraphEditor = FFlowGraphUtils::GetFlowGraphEditor(GetGraph());
	if (!FlowGraphEditor.IsValid())
	{
		return false;
	}
	
	if (FlowGraphEditor->GetSelectedFlowNodes().Num() == 1)
	{
		for (const UFlowGraphNode* SelectedNode : FlowGraphEditor->GetSelectedFlowNodes())
		{
			return SelectedNode == this;
		}
	}
	return false;
}

#undef LOCTEXT_NAMESPACE