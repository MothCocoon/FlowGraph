#include "SFlowGraphNode.h"
#include "../Nodes/FlowGraphNode.h"
#include "../../FlowEditorSettings.h"

#include "GraphEditorSettings.h"
#include "ScopedTransaction.h"
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
	const FString Description = GraphNode->GetDesc();
	if (!Description.IsEmpty())
	{
		FGraphInformationPopupInfo DescriptionPopup = FGraphInformationPopupInfo(nullptr, UFlowEditorSettings::Get()->NodeDescriptionBackground, Description);
		Popups.Add(DescriptionPopup);
	}
}

void SFlowGraphNode::CreateOutputSideAddButton(TSharedPtr<SVerticalBox> OutputBox)
{
	TSharedRef<SWidget> AddPinButton = AddPinButtonContent(
		NSLOCTEXT("FlowNode", "FlowNodeAddPinButton", "Add output"),
		NSLOCTEXT("FlowNode", "FlowNodeAddPinButton_Tooltip", "Adds an output to the flow node")
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
	
	if (ButtonVisibility == EVisibility::Visible && !FlowGraphNode->CanAddOutputPin())
	{
		ButtonVisibility = EVisibility::Collapsed;
	}

	return ButtonVisibility;
}

FReply SFlowGraphNode::OnAddPin()
{
	FlowGraphNode->AddOutputPin();

	return FReply::Handled();
}
