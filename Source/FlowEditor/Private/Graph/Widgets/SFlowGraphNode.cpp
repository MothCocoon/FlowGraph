#include "Graph/Widgets/SFlowGraphNode.h"
#include "FlowEditorStyle.h"
#include "Graph/FlowGraphSettings.h"

#include "Nodes/FlowNode.h"

#include "EdGraph/EdGraphPin.h"
#include "Editor.h"
#include "GraphEditorSettings.h"
#include "IDocumentation.h"
#include "Input/Reply.h"
#include "Layout/Margin.h"
#include "Misc/Attribute.h"
#include "SCommentBubble.h"
#include "SGraphNode.h"
#include "SGraphPin.h"
#include "SLevelOfDetailBranchNode.h"
#include "SNodePanel.h"
#include "SlateOptMacros.h"
#include "Styling/SlateColor.h"
#include "TutorialMetaData.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SToolTip.h"

#define LOCTEXT_NAMESPACE "SFlowGraphNode"

SFlowGraphPinExec::SFlowGraphPinExec()
{
	PinColorModifier = UFlowGraphSettings::Get()->ExecPinColorModifier;
}

void SFlowGraphPinExec::Construct(const FArguments& InArgs, UEdGraphPin* InPin)
{
	SGraphPinExec::Construct(SGraphPinExec::FArguments(), InPin);
	bUsePinColorForText = true;
}

void SFlowGraphPinExec::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
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
				if (GetDirection() == EGPD_Input)
				{
					PinRecords = FlowNode->GetInputRecords(GetPinObj()->PinName);
				}
				else
				{
					PinRecords = FlowNode->GetOutputRecords(GetPinObj()->PinName);
				}

				if (PinRecords.Num() > 0)
				{
					FString TooltipString = FString();
					for (int32 i = 0; i < PinRecords.Num(); i++)
					{
						TooltipString += FString::FromInt(i + 1) + TEXT(") ") + PinRecords[i].HumanReadableTime;
						if (i < PinRecords.Num() - 1)
						{
							TooltipString += LINE_TERMINATOR;
						}
					}
					GraphPinObj->PinToolTip = TooltipString;
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
		const FGraphInformationPopupInfo DescriptionPopup = FGraphInformationPopupInfo(nullptr, UFlowGraphSettings::Get()->NodeDescriptionBackground, Description);
		Popups.Add(DescriptionPopup);
	}

	if (GEditor->PlayWorld)
	{
		const FString Status = FlowGraphNode->GetStatusString();
		if (!Status.IsEmpty())
		{
			const FGraphInformationPopupInfo DescriptionPopup = FGraphInformationPopupInfo(nullptr, UFlowGraphSettings::Get()->NodeStatusBackground, Status);
			Popups.Add(DescriptionPopup);
		}
		else if (FlowGraphNode->IsContentPreloaded())
		{
			const FGraphInformationPopupInfo DescriptionPopup = FGraphInformationPopupInfo(nullptr, UFlowGraphSettings::Get()->NodeStatusBackground, TEXT("Preloaded"));
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
				return FFlowEditorStyle::Get()->GetBrush(TEXT("Flow.Node.ActiveShadow"));
			case EFlowActivationState::WasActive:
				return FFlowEditorStyle::Get()->GetBrush(TEXT("Flow.Node.WasActiveShadow"));
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

	// Pin breakpoints
	for (const TPair<FEdGraphPinReference, FFlowBreakpoint>& PinBreakpoint : FlowGraphNode->PinBreakpoints)
	{
		if (PinBreakpoint.Key.Get()->Direction == EGPD_Input)
		{
			GetPinBrush(true, WidgetSize.X, FlowGraphNode->InputPins.IndexOfByKey(PinBreakpoint.Key.Get()), PinBreakpoint.Value, Brushes);
		}
		else
		{
			GetPinBrush(false, WidgetSize.X, FlowGraphNode->OutputPins.IndexOfByKey(PinBreakpoint.Key.Get()), PinBreakpoint.Value, Brushes);
		}
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

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SFlowGraphNode::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	//	     ______________________
	//	    |      TITLE AREA      |
	//	    +-------+------+-------+
	//	    | (>) L |      | R (>) |
	//	    | (>) E |      | I (>) |
	//	    | (>) F |      | G (>) |
	//	    | (>) T |      | H (>) |
	//	    |       |      | T (>) |
	//	    |_______|______|_______|
	//
	TSharedPtr<SVerticalBox> MainVerticalBox;
	SetupErrorReporting();

	const TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);

	// Get node icon
	IconColor = FLinearColor::White;
	const FSlateBrush* IconBrush = nullptr;
	if (GraphNode && GraphNode->ShowPaletteIconOnNode())
	{
		IconBrush = GraphNode->GetIconAndTint(IconColor).GetOptionalIcon();
	}

	const TSharedRef<SOverlay> DefaultTitleAreaWidget = SNew(SOverlay)
		+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
					.HAlign(HAlign_Fill)
					[
						SNew(SBorder)
							.BorderImage(FFlowEditorStyle::GetBrush("Flow.Node.Title"))
							// The extra margin on the right is for making the color spill stretch well past the node title
							.Padding(FMargin(10, 5, 30, 3))
							.BorderBackgroundColor(this, &SGraphNode::GetNodeTitleColor)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
									.VAlign(VAlign_Top)
									.Padding(FMargin(0.f, 0.f, 4.f, 0.f))
									.AutoWidth()
									[
										SNew(SImage)
											.Image(IconBrush)
											.ColorAndOpacity(this, &SGraphNode::GetNodeTitleIconColor)
									]
								+ SHorizontalBox::Slot()
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot()
											.AutoHeight()
											[
												CreateTitleWidget(NodeTitle)
											]
										+ SVerticalBox::Slot()
											.AutoHeight()
											[
												NodeTitle.ToSharedRef()
											]
									]
							]
					]
			];

	SetDefaultTitleAreaWidget(DefaultTitleAreaWidget);

	const TSharedRef<SWidget> TitleAreaWidget = 
		SNew(SLevelOfDetailBranchNode)
			.UseLowDetailSlot(this, &SFlowGraphNode::UseLowDetailNodeTitles)
			.LowDetail()
			[
				SNew(SBorder)
					.BorderImage(FFlowEditorStyle::GetBrush("Flow.Node.Title"))
					.Padding(FMargin(75.0f, 22.0f)) // Saving enough space for a 'typical' title so the transition isn't quite so abrupt
					.BorderBackgroundColor(this, &SGraphNode::GetNodeTitleColor)
			]
			.HighDetail()
			[
				DefaultTitleAreaWidget
			];


	if (!SWidget::GetToolTip().IsValid())
	{
		const TSharedRef<SToolTip> DefaultToolTip = IDocumentation::Get()->CreateToolTip(TAttribute<FText>(this, &SGraphNode::GetNodeTooltip), nullptr, GraphNode->GetDocumentationLink(), GraphNode->GetDocumentationExcerptName());
		SetToolTip(DefaultToolTip);
	}

	// Setup a meta tag for this node
	FGraphNodeMetaData TagMeta(TEXT("FlowGraphNode"));
	PopulateMetaTag(&TagMeta);

	this->ContentScale.Bind(this, &SGraphNode::GetContentScale);

	TSharedPtr<SVerticalBox> InnerVerticalBox = SNew(SVerticalBox)
		+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			.Padding(Settings->GetNonPinNodeBodyPadding())
			[
				TitleAreaWidget
			]
		+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			[
				CreateNodeContentArea()
			];

	const TSharedPtr<SWidget> EnabledStateWidget = GetEnabledStateWidget();
	if (EnabledStateWidget.IsValid())
	{
		InnerVerticalBox->AddSlot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			.Padding(FMargin(2, 0))
			[
				EnabledStateWidget.ToSharedRef()
			];
	}

	InnerVerticalBox->AddSlot()
		.AutoHeight()
		.Padding(Settings->GetNonPinNodeBodyPadding())
		[
			ErrorReporting->AsWidget()
		];

	this->GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SAssignNew(MainVerticalBox, SVerticalBox)
			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SOverlay)
						.AddMetaData<FGraphNodeMetaData>(TagMeta)
						+ SOverlay::Slot()
							.Padding(Settings->GetNonPinNodeBodyPadding())
							[
								SNew(SImage)
									.Image(GetNodeBodyBrush())
									.ColorAndOpacity(this, &SGraphNode::GetNodeBodyColor)
							]
						+ SOverlay::Slot()
							[
								InnerVerticalBox.ToSharedRef()
							]
				]
		];

	if (GraphNode)
	{
		// Create comment bubble
		TSharedPtr<SCommentBubble> CommentBubble;
		const FSlateColor CommentColor = GetDefault<UGraphEditorSettings>()->DefaultCommentNodeTitleColor;

		SAssignNew(CommentBubble, SCommentBubble)
			.GraphNode(GraphNode)
			.Text(this, &SGraphNode::GetNodeComment)
			.OnTextCommitted(this, &SGraphNode::OnCommentTextCommitted)
			.OnToggled(this, &SGraphNode::OnCommentBubbleToggled)
			.ColorAndOpacity(CommentColor)
			.AllowPinning(true)
			.EnableTitleBarBubble(true)
			.EnableBubbleCtrls(true)
			.GraphLOD(this, &SGraphNode::GetCurrentLOD)
			.IsGraphNodeHovered(this, &SGraphNode::IsHovered);

		GetOrAddSlot(ENodeZone::TopCenter)
			.SlotOffset(TAttribute<FVector2D>(CommentBubble.Get(), &SCommentBubble::GetOffset))
			.SlotSize(TAttribute<FVector2D>(CommentBubble.Get(), &SCommentBubble::GetSize))
			.AllowScaling(TAttribute<bool>(CommentBubble.Get(), &SCommentBubble::IsScalingAllowed))
			.VAlign(VAlign_Top)
			[
				CommentBubble.ToSharedRef()
			];
	}

	CreateBelowWidgetControls(MainVerticalBox);
	CreatePinWidgets();
	CreateInputSideAddButton(LeftNodeBox);
	CreateOutputSideAddButton(RightNodeBox);
	CreateBelowPinControls(InnerVerticalBox);
	CreateAdvancedViewArrow(InnerVerticalBox);
}

TSharedRef<SWidget> SFlowGraphNode::CreateNodeContentArea()
{
	return SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("NoBorder"))
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.FillWidth(1.0f)
			[
				SAssignNew(LeftNodeBox, SVerticalBox)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			[
				SAssignNew(RightNodeBox, SVerticalBox)
			]
		];
}

const FSlateBrush* SFlowGraphNode::GetNodeBodyBrush() const
{
	return FFlowEditorStyle::GetBrush("Flow.Node.Body");
}

void SFlowGraphNode::CreateStandardPinWidget(UEdGraphPin* Pin)
{
	TSharedPtr<SGraphPin> NewPin = SNew(SFlowGraphPinExec, Pin);

	if (!UFlowGraphSettings::Get()->bShowDefaultPinNames && FlowGraphNode->GetFlowNode())
	{
		if (Pin->Direction == EGPD_Input)
		{
			if (FlowGraphNode->GetFlowNode()->GetInputNames().Num() == 1 && Pin->PinName == UFlowNode::DefaultInputName)
			{
				NewPin->SetShowLabel(false);
			}
		}
		else
		{
			if (FlowGraphNode->GetFlowNode()->GetOutputNames().Num() == 1 && Pin->PinName == UFlowNode::DefaultOutputName)
			{
				NewPin->SetShowLabel(false);
			}
		}
	}

	this->AddPin(NewPin.ToSharedRef());
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SFlowGraphNode::CreateInputSideAddButton(TSharedPtr<SVerticalBox> OutputBox)
{
	if (FlowGraphNode->CanUserAddInput())
	{
		const TSharedRef<SWidget> AddPinButton = AddPinButtonContent(
			LOCTEXT("FlowNodeAddPinButton", "Add pin"),
			LOCTEXT("FlowNodeAddPinButton_Tooltip", "Adds an input pin")
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
			LOCTEXT("FlowNodeAddPinButton", "Add pin"),
			LOCTEXT("FlowNodeAddPinButton_Tooltip", "Adds an output pin")
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

#undef LOCTEXT_NAMESPACE
