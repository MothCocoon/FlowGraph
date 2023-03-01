// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

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
#include "SlateOptMacros.h"
#include "SLevelOfDetailBranchNode.h"
#include "SNodePanel.h"
#include "Styling/SlateColor.h"
#include "TutorialMetaData.h"
#include "Graph/FlowGraphEditorSettings.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

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

void SFlowGraphNode::Construct(const FArguments& InArgs, UFlowGraphNode* InNode)
{
	GraphNode = InNode;

	FlowGraphNode = InNode;
	FlowGraphNode->OnSignalModeChanged.BindRaw(this, &SFlowGraphNode::UpdateGraphNode);

	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

void SFlowGraphNode::GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const
{
	const FString Description = GEditor->PlayWorld && UFlowGraphEditorSettings::Get()->bHideNodeDescriptionOnPIE ? "" : FlowGraphNode->GetNodeDescription();	
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
			const FGraphInformationPopupInfo DescriptionPopup = FGraphInformationPopupInfo(nullptr, FlowGraphNode->GetStatusBackgroundColor(), Status);
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
			case EFlowNodeState::NeverActivated:
				return SGraphNode::GetShadowBrush(bSelected);
			case EFlowNodeState::Active:
				return FFlowEditorStyle::Get()->GetBrush(TEXT("Flow.Node.ActiveShadow"));
			case EFlowNodeState::Completed:
			case EFlowNodeState::Aborted:
				return FFlowEditorStyle::Get()->GetBrush(TEXT("Flow.Node.WasActiveShadow"));
			default: ;
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
							.ColorAndOpacity(this, &SFlowGraphNode::GetNodeTitleIconColor)
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

	// Setup a meta tag for this node
	FGraphNodeMetaData TagMeta(TEXT("FlowGraphNode"));
	PopulateMetaTag(&TagMeta);

	this->ContentScale.Bind(this, &SGraphNode::GetContentScale);

	const TSharedPtr<SVerticalBox> InnerVerticalBox = SNew(SVerticalBox)
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
							.ColorAndOpacity(this, &SFlowGraphNode::GetNodeBodyColor)
						]
					+ SOverlay::Slot()
						[
							InnerVerticalBox.ToSharedRef()
						]
			]
		];

	if (GraphNode && GraphNode->SupportsCommentBubble())
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

void SFlowGraphNode::UpdateErrorInfo()
{
	if (const UFlowNode* FlowNode = FlowGraphNode->GetFlowNode())
	{
		if (FlowNode->ValidationLog.Messages.Num() > 0)
		{
			EMessageSeverity::Type MaxSeverity = EMessageSeverity::Info;
			for (const TSharedRef<FTokenizedMessage>& Message : FlowNode->ValidationLog.Messages)
			{
				if (Message->GetSeverity() < MaxSeverity)
				{
					MaxSeverity = Message->GetSeverity();
				}
			}

			switch(MaxSeverity)
			{
				case EMessageSeverity::Error:
					ErrorMsg = FString(TEXT("ERROR!"));
					ErrorColor = FAppStyle::GetColor("ErrorReporting.BackgroundColor");
					break;
				case EMessageSeverity::PerformanceWarning:
				case EMessageSeverity::Warning:
					ErrorMsg = FString(TEXT("WARNING!"));
					ErrorColor = FAppStyle::GetColor("ErrorReporting.WarningBackgroundColor");
					break;
				case EMessageSeverity::Info:
					ErrorMsg = FString(TEXT("NOTE"));
					ErrorColor = FAppStyle::GetColor("InfoReporting.BackgroundColor");
					break;
				default: ;
			}

			return;
		}

		if (FlowNode->GetClass()->HasAnyClassFlags(CLASS_Deprecated) || FlowNode->bNodeDeprecated)
		{
			ErrorMsg = FlowNode->ReplacedBy ? FString::Printf(TEXT(" REPLACED BY: %s "), *FlowNode->ReplacedBy->GetName()) : FString(TEXT(" DEPRECATED! "));
			ErrorColor = FAppStyle::GetColor("ErrorReporting.WarningBackgroundColor");
			return;
		}
	}

	SGraphNode::UpdateErrorInfo();
}

TSharedRef<SWidget> SFlowGraphNode::CreateTitleWidget(TSharedPtr<SNodeTitle> NodeTitle)
{
	SAssignNew(InlineEditableText, SInlineEditableTextBlock)
		.Style(FAppStyle::Get(), "Graph.Node.NodeTitleInlineEditableText")
		.Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
		.OnVerifyTextChanged(this, &SFlowGraphNode::OnVerifyNameTextChanged)
		.OnTextCommitted(this, &SFlowGraphNode::OnNameTextCommited)
		.IsReadOnly(this, &SFlowGraphNode::IsNameReadOnly)
		.IsSelected(this, &SFlowGraphNode::IsSelectedExclusively);
		InlineEditableText->SetColorAndOpacity(TAttribute<FLinearColor>::Create(TAttribute<FLinearColor>::FGetter::CreateSP(this, &SFlowGraphNode::GetNodeTitleTextColor)));

	return InlineEditableText.ToSharedRef();
}

TSharedRef<SWidget> SFlowGraphNode::CreateNodeContentArea()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("NoBorder"))
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

FSlateColor SFlowGraphNode::GetNodeTitleColor() const
{
	FLinearColor ReturnTitleColor = GraphNode->IsDeprecated() ? FLinearColor::Red : GetNodeObj()->GetNodeTitleColor();

	if (FlowGraphNode->GetSignalMode() == EFlowSignalMode::Enabled)
	{
		ReturnTitleColor.A = FadeCurve.GetLerp();
	}
	else
	{
		ReturnTitleColor *= FLinearColor(0.5f, 0.5f, 0.5f, 0.4f);
	}

	return ReturnTitleColor;
}

FSlateColor SFlowGraphNode::GetNodeBodyColor() const
{
	FLinearColor ReturnBodyColor = GraphNode->GetNodeBodyTintColor();
	if (FlowGraphNode->GetSignalMode() != EFlowSignalMode::Enabled)
	{
		ReturnBodyColor *= FLinearColor(1.0f, 1.0f, 1.0f, 0.5f); 
	}
	return ReturnBodyColor;
}

FSlateColor SFlowGraphNode::GetNodeTitleIconColor() const
{
	FLinearColor ReturnIconColor = IconColor;
	if (FlowGraphNode->GetSignalMode() != EFlowSignalMode::Enabled)
	{
		ReturnIconColor *= FLinearColor(1.0f, 1.0f, 1.0f, 0.3f); 
	}
	return ReturnIconColor;
}

FLinearColor SFlowGraphNode::GetNodeTitleTextColor() const
{
	FLinearColor ReturnTextColor = FLinearColor::White;
	if (FlowGraphNode->GetSignalMode() != EFlowSignalMode::Enabled)
	{
		ReturnTextColor *= FLinearColor(1.0f, 1.0f, 1.0f, 0.3f); 
	}
	return ReturnTextColor;
}

TSharedPtr<SWidget> SFlowGraphNode::GetEnabledStateWidget() const
{
	if (FlowGraphNode->GetSignalMode() != EFlowSignalMode::Enabled && !GraphNode->IsAutomaticallyPlacedGhostNode())
	{
		const bool bPassThrough = FlowGraphNode->GetSignalMode() == EFlowSignalMode::PassThrough;
		const FText StatusMessage = bPassThrough ? LOCTEXT("PassThrough", "Pass Through") : LOCTEXT("DisabledNode", "Disabled");
		const FText StatusMessageTooltip = bPassThrough ?
			LOCTEXT("PassThroughTooltip", "This node won't execute internal logic, but it will trigger all connected outputs") :
			LOCTEXT("DisabledNodeTooltip", "This node is disabled and will not be executed");

		return SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush(bPassThrough ? "Graph.Node.DevelopmentBanner" : "Graph.Node.DisabledBanner"))
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(STextBlock)
				.Text(StatusMessage)
				.ToolTipText(StatusMessageTooltip)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FLinearColor::White)
				.ShadowOffset(FVector2D::UnitVector)
				.Visibility(EVisibility::Visible)
			];
	}

	return TSharedPtr<SWidget>();
}

void SFlowGraphNode::CreateStandardPinWidget(UEdGraphPin* Pin)
{
	const TSharedPtr<SGraphPin> NewPin = SNew(SFlowGraphPinExec, Pin);

	if (!UFlowGraphSettings::Get()->bShowDefaultPinNames && FlowGraphNode->GetFlowNode())
	{
		if (Pin->Direction == EGPD_Input)
		{
			if (FlowGraphNode->GetFlowNode()->GetInputPins().Num() == 1 && Pin->PinName == UFlowNode::DefaultInputPin.PinName)
			{
				NewPin->SetShowLabel(false);
			}
		}
		else
		{
			if (FlowGraphNode->GetFlowNode()->GetOutputPins().Num() == 1 && Pin->PinName == UFlowNode::DefaultOutputPin.PinName)
			{
				NewPin->SetShowLabel(false);
			}
		}
	}

	this->AddPin(NewPin.ToSharedRef());
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedPtr<SToolTip> SFlowGraphNode::GetComplexTooltip()
{
	return IDocumentation::Get()->CreateToolTip(TAttribute<FText>(this, &SGraphNode::GetNodeTooltip), nullptr, GraphNode->GetDocumentationLink(), GraphNode->GetDocumentationExcerptName());
}

void SFlowGraphNode::CreateInputSideAddButton(TSharedPtr<SVerticalBox> OutputBox)
{
	if (FlowGraphNode->CanUserAddInput())
	{
		TSharedPtr<SWidget> AddPinWidget;
		SAssignNew(AddPinWidget, SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		. VAlign(VAlign_Center)
		. Padding( 0,0,7,0 )
		[
			SNew(SImage)
			.Image(FAppStyle::GetBrush(TEXT("Icons.PlusCircle")))
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FlowNodeAddPinButton", "Add pin"))
			.ColorAndOpacity(FLinearColor::White)
		];

		AddPinButton(OutputBox, AddPinWidget.ToSharedRef(), EGPD_Input);
	}
}

void SFlowGraphNode::CreateOutputSideAddButton(TSharedPtr<SVerticalBox> OutputBox)
{
	if (FlowGraphNode->CanUserAddOutput())
	{
		TSharedPtr<SWidget> AddPinWidget;
		SAssignNew(AddPinWidget, SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FlowNodeAddPinButton", "Add pin"))
			.ColorAndOpacity(FLinearColor::White)
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(7,0,0,0)
		[
			SNew(SImage)
			.Image(FAppStyle::GetBrush(TEXT("Icons.PlusCircle")))
		];

		AddPinButton(OutputBox, AddPinWidget.ToSharedRef(), EGPD_Output);
	}
}

void SFlowGraphNode::AddPinButton(TSharedPtr<SVerticalBox> OutputBox, const TSharedRef<SWidget> ButtonContent, const EEdGraphPinDirection Direction, const FString DocumentationExcerpt, const TSharedPtr<SToolTip> CustomTooltip)
{
	const FText PinTooltipText = (Direction == EEdGraphPinDirection::EGPD_Input) ? LOCTEXT("FlowNodeAddPinButton_InputTooltip", "Adds an input pin") : LOCTEXT("FlowNodeAddPinButton_OutputTooltip", "Adds an output pin");
	TSharedPtr<SToolTip> Tooltip;

	if (CustomTooltip.IsValid())
	{
		Tooltip = CustomTooltip;
	}
	else if (!DocumentationExcerpt.IsEmpty())
	{
		Tooltip = IDocumentation::Get()->CreateToolTip(PinTooltipText, nullptr, GraphNode->GetDocumentationLink(), DocumentationExcerpt);
	}

	const TSharedRef<SButton> AddPinButton = SNew(SButton)
	.ContentPadding(0.0f)
	.ButtonStyle(FAppStyle::Get(), "NoBorder")
	.OnClicked(this, &SFlowGraphNode::OnAddFlowPin, Direction)
	.IsEnabled(this, &SFlowGraphNode::IsNodeEditable)
	.ToolTipText(PinTooltipText)
	.ToolTip(Tooltip)
	.Visibility(this, &SFlowGraphNode::IsAddPinButtonVisible)
	[
		ButtonContent
	];

	AddPinButton->SetCursor(EMouseCursor::Hand);

	FMargin AddPinPadding = (Direction == EEdGraphPinDirection::EGPD_Input) ? Settings->GetInputPinPadding() : Settings->GetOutputPinPadding();
	AddPinPadding.Top += 6.0f;

	OutputBox->AddSlot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.Padding(AddPinPadding)
		[
			AddPinButton
		];
}

FReply SFlowGraphNode::OnAddFlowPin(const EEdGraphPinDirection Direction)
{
	switch (Direction)
	{
		case EGPD_Input:
			FlowGraphNode->AddUserInput();
			break;
		case EGPD_Output:
			FlowGraphNode->AddUserOutput();
			break;
		default: ;
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
