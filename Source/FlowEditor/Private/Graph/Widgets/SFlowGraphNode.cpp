#include "Graph/Widgets/SFlowGraphNode.h"
#include "FlowEditorSettings.h"
#include "FlowEditorStyle.h"

#include "Nodes/FlowNode.h"

#include "Editor.h"
#include "GraphEditorSettings.h"
#include "Widgets/SBoxPanel.h"

void SFlowGraphPin::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SGraphPinExec::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (IsHovered())
	{
		if (GEditor->PlayWorld)
		{
			UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(GraphPinObj->GetOwningNode());
			if (UFlowNode* FlowNode = GraphNode->GetInspectedNodeInstance())
			{
				TArray<FPinRecord> PinRecords;
				if (GetDirection() == EEdGraphPinDirection::EGPD_Input)
				{
					PinRecords = FlowNode->GetInputRecords(GetPinObj()->PinName);
				}
				else
				{
					PinRecords = FlowNode->GetOutputRecords(GetPinObj()->PinName);
				}

				if (PinRecords.Num() > 0)
				{
					FString TooltipStrig = FString();
					for (int32 i = 0; i < PinRecords.Num(); i++)
					{
						TooltipStrig += FString::FromInt(i + 1) + TEXT(") ") + PinRecords[i].HumanReadableTime;
						if (i < PinRecords.Num() - 1)
						{
							TooltipStrig += LINE_TERMINATOR;
						}
					}
					GraphPinObj->PinToolTip = TooltipStrig;
				}
				else
				{
					GraphPinObj->PinToolTip = TEXT("0 calls");
				}
			}
		}
		else
		{
			GraphPinObj->PinToolTip = FString();
		}
	}
}

void SFlowGraphNode::Construct(const FArguments& InArgs, UFlowGraphNode* InNode)
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
		const FGraphInformationPopupInfo DescriptionPopup = FGraphInformationPopupInfo(nullptr, UFlowEditorSettings::Get()->NodeDescriptionBackground, Description);
		Popups.Add(DescriptionPopup);
	}

	if (GEditor->PlayWorld)
	{
		const FString Status = FlowGraphNode->GetStatusString();
		if (!Status.IsEmpty())
		{
			const FGraphInformationPopupInfo DescriptionPopup = FGraphInformationPopupInfo(nullptr, UFlowEditorSettings::Get()->NodeStatusBackground, Status);
			Popups.Add(DescriptionPopup);
		}
		else if (FlowGraphNode->IsContentPreloaded())
		{
			const FGraphInformationPopupInfo DescriptionPopup = FGraphInformationPopupInfo(nullptr, UFlowEditorSettings::Get()->NodeStatusBackground, TEXT("Preloaded"));
			Popups.Add(DescriptionPopup);
		}
	}
}

const FSlateBrush* SFlowGraphNode::GetShadowBrush(bool bSelected) const
{
	if (GEditor->PlayWorld)
	{
		switch (FlowGraphNode->GetActivationState())
		{
			case EFlowActivationState::NeverActivated:
				return SGraphNode::GetShadowBrush(bSelected);
			case EFlowActivationState::Active:
				return FFlowEditorStyle::Get()->GetBrush(TEXT("FlowGraph.ActiveShadow"));
			case EFlowActivationState::WasActive:
				return FFlowEditorStyle::Get()->GetBrush(TEXT("FlowGraph.WasActiveShadow"));
		}
	}

	return SGraphNode::GetShadowBrush(bSelected);
}

void SFlowGraphNode::GetOverlayBrushes(bool bSelected, const FVector2D WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const
{
	// Node breakpoint
	if (FlowGraphNode->NodeBreakpoint.bHasBreakpoint)
	{
		FOverlayBrushInfo NodeBrush;

		if (FlowGraphNode->NodeBreakpoint.bBreakpointHit)
		{
			NodeBrush.Brush = FFlowEditorStyle::Get()->GetBrush(TEXT("FlowGraph.BreakpointHit"));
			NodeBrush.OverlayOffset.X = WidgetSize.X - 12.0f;
		}
		else
		{
			NodeBrush.Brush = FFlowEditorStyle::Get()->GetBrush(FlowGraphNode->NodeBreakpoint.bBreakpointEnabled ? TEXT("FlowGraph.BreakpointEnabled") : TEXT("FlowGraph.BreakpointDisabled"));
			NodeBrush.OverlayOffset.X = WidgetSize.X;
		}

		NodeBrush.OverlayOffset.Y = -NodeBrush.Brush->ImageSize.Y;
		NodeBrush.AnimationEnvelope = FVector2D(0.f, 10.f);
		Brushes.Add(NodeBrush);
	}

	// Input Pin breakpoints
	for (const TPair<int32, FFlowBreakpoint>& PinBreakpoint : FlowGraphNode->InputBreakpoints)
	{
		GetPinBrush(true, WidgetSize.X, PinBreakpoint.Key, PinBreakpoint.Value, Brushes);
	}

	// Output Pin breakpoints
	for (const TPair<int32, FFlowBreakpoint>& PinBreakpoint : FlowGraphNode->OutputBreakpoints)
	{
		GetPinBrush(false, WidgetSize.X, PinBreakpoint.Key, PinBreakpoint.Value, Brushes);
	}
}

void SFlowGraphNode::GetPinBrush(const bool bLeftSide, const float WidgetWidth, const int32 PinIndex, const FFlowBreakpoint& Breakpoint, TArray<FOverlayBrushInfo>& Brushes) const
{
	if (Breakpoint.bHasBreakpoint)
	{
		FOverlayBrushInfo PinBrush;

		if (Breakpoint.bBreakpointHit)
		{
			PinBrush.Brush = FFlowEditorStyle::Get()->GetBrush(TEXT("FlowGraph.PinBreakpointHit"));
			PinBrush.OverlayOffset.X = bLeftSide ? 0.0f : (WidgetWidth - 36.0f);
			PinBrush.OverlayOffset.Y = 12.0f + PinIndex * 28.0f;
		}
		else
		{
			PinBrush.Brush = FFlowEditorStyle::Get()->GetBrush(Breakpoint.bBreakpointEnabled ? TEXT("FlowGraph.BreakpointEnabled") : TEXT("FlowGraph.BreakpointDisabled"));
			PinBrush.OverlayOffset.X = bLeftSide ? -24.0f : WidgetWidth;
			PinBrush.OverlayOffset.Y = 16.0f + PinIndex * 28.0f;
		}

		PinBrush.AnimationEnvelope = FVector2D(0.f, 10.f);
		Brushes.Add(PinBrush);
	}
}

void SFlowGraphNode::CreateStandardPinWidget(UEdGraphPin* Pin)
{
	TSharedPtr<SGraphPin> NewPin = SNew(SFlowGraphPin, Pin);

	if (!UFlowEditorSettings::Get()->bShowDefaultPinNames)
	{
		if (Pin->Direction == EGPD_Input)
		{
			if (FlowGraphNode->GetFlowNode()->InputNames.Num() == 1 && Pin->PinName == UFlowNode::DefaultInputName)
			{
				NewPin->SetShowLabel(false);
			}
		}
		else
		{
			if (FlowGraphNode->GetFlowNode()->OutputNames.Num() == 1 && Pin->PinName == UFlowNode::DefaultOutputName)
			{
				NewPin->SetShowLabel(false);
			}
		}
	}

	this->AddPin(NewPin.ToSharedRef());
}

void SFlowGraphNode::CreateInputSideAddButton(TSharedPtr<SVerticalBox> OutputBox)
{
	if (FlowGraphNode->CanUserAddInput())
	{
		const TSharedRef<SWidget> AddPinButton = AddPinButtonContent(
		NSLOCTEXT("FlowNode", "FlowNodeAddPinButton", "Add pin"),
		NSLOCTEXT("FlowNode", "FlowNodeAddPinButton_Tooltip", "Adds an input pin")
		);

		FMargin AddPinPadding = Settings->GetInputPinPadding();
		AddPinPadding.Top += 6.0f;

		OutputBox->AddSlot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			.Padding(AddPinPadding)
			[
				AddPinButton
			];
	}
}

void SFlowGraphNode::CreateOutputSideAddButton(TSharedPtr<SVerticalBox> OutputBox)
{
	if (FlowGraphNode->CanUserAddOutput())
	{
		const TSharedRef<SWidget> AddPinButton = AddPinButtonContent(
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
}

FReply SFlowGraphNode::OnAddPin()
{
	if (FlowGraphNode->CanUserAddInput())
	{
		FlowGraphNode->AddUserInput();
	}
	else if (FlowGraphNode->CanUserAddOutput())
	{
		FlowGraphNode->AddUserOutput();
	}

	return FReply::Handled();
}
