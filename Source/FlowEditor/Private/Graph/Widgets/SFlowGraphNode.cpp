#include "SFlowGraphNode.h"
#include "FlowEditorSettings.h"
#include "../Nodes/FlowGraphNode.h"

#include "GraphEditorSettings.h"
#include "ScopedTransaction.h"
#include "UnrealEd/Public/Editor.h"
#include "Widgets/SBoxPanel.h"

void SFlowGraphNode::Construct(const FArguments& InArgs, class UFlowGraphNode* InNode)
{
	GraphNode = InNode;
	FlowGraphNode = InNode;

	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

void SFlowGraphNode::GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const
{
	const FString Description = FlowGraphNode->GetNodeDescription();
	if (!Description.IsEmpty())
	{
		FGraphInformationPopupInfo DescriptionPopup = FGraphInformationPopupInfo(nullptr, UFlowEditorSettings::Get()->NodeDescriptionBackground, Description);
		Popups.Add(DescriptionPopup);
	}

	if (GEditor->PlayWorld)
	{
		const FString Status = FlowGraphNode->GetNodeStatus();
		if (!Status.IsEmpty())
		{
			FGraphInformationPopupInfo DescriptionPopup = FGraphInformationPopupInfo(nullptr, UFlowEditorSettings::Get()->NodeStatusBackground, Status);
			Popups.Add(DescriptionPopup);
		}
		else if (FlowGraphNode->IsContentPreloaded())
		{
			FGraphInformationPopupInfo DescriptionPopup = FGraphInformationPopupInfo(nullptr, UFlowEditorSettings::Get()->NodeStatusBackground, TEXT("Preloaded"));
			Popups.Add(DescriptionPopup);
		}
	}
}

void SFlowGraphNode::CreateOutputSideAddButton(TSharedPtr<SVerticalBox> OutputBox)
{
	TSharedRef<SWidget> AddPinButton = AddPinButtonContent(
		NSLOCTEXT("FlowNode", "FlowNodeAddPinButton", "Add pin"),
		NSLOCTEXT("FlowNode", "FlowNodeAddPinButton_Tooltip", "Adds an output pin")
	);

	FMargin AddPinPadding = Settings->GetOutputPinPadding();
	AddPinPadding.Top += 6.0f;

	OutputBox->AddSlot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.Padding(AddPinPadding)
		[
			AddPinButton
		];
}

EVisibility SFlowGraphNode::IsAddPinButtonVisible() const
{
	EVisibility ButtonVisibility = SGraphNode::IsAddPinButtonVisible();
	
	if (ButtonVisibility == EVisibility::Visible && !FlowGraphNode->CanUserAddOutput())
	{
		ButtonVisibility = EVisibility::Collapsed;
	}

	return ButtonVisibility;
}

FReply SFlowGraphNode::OnAddPin()
{
	FlowGraphNode->AddUserOutput();
	return FReply::Handled();
}