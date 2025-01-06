// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/Widgets/SFlowGraphNode.h"
#include "DragFlowGraphNode.h"
#include "FlowEditorStyle.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSettings.h"
#include "Nodes/FlowNode.h"

#include "EdGraph/EdGraphPin.h"
#include "Editor.h"
#include "GraphEditorSettings.h"
#include "IDocumentation.h"
#include "Input/Reply.h"
#include "Internationalization/BreakIterator.h"
#include "Layout/Margin.h"
#include "Misc/Attribute.h"
#include "NodeFactory.h"
#include "SCommentBubble.h"
#include "ScopedTransaction.h"
#include "SGraphNode.h"
#include "SGraphPanel.h"
#include "SGraphPin.h"
#include "SlateOptMacros.h"
#include "SLevelOfDetailBranchNode.h"
#include "SNodePanel.h"
#include "Styling/SlateColor.h"
#include "TutorialMetaData.h"
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

const FLinearColor SFlowGraphNode::UnselectedNodeTint = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f);
const FLinearColor SFlowGraphNode::ConfigBoxColor = FLinearColor(0.04f, 0.04f, 0.04f, 1.0f);

void SFlowGraphNode::Construct(const FArguments& InArgs, UFlowGraphNode* InNode)
{
	GraphNode = InNode;

	FlowGraphNode = InNode;

	check(FlowGraphNode);
	FlowGraphNode->OnSignalModeChanged.BindRaw(this, &SFlowGraphNode::UpdateGraphNode);
	FlowGraphNode->OnReconstructNodeCompleted.BindRaw(this, &SFlowGraphNode::UpdateGraphNode);

	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();

	bDragMarkerVisible = false;
}

SFlowGraphNode::~SFlowGraphNode()
{
	check(FlowGraphNode);
	FlowGraphNode->OnSignalModeChanged.Unbind();
	FlowGraphNode->OnReconstructNodeCompleted.Unbind();

	FlowGraphNode = nullptr;
}

void SFlowGraphNode::GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const
{
	const FString& Description = FlowGraphNode->GetNodeDescription(); 
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
	if (FlowGraphNode->NodeBreakpoint.IsAllowed())
	{
		FOverlayBrushInfo NodeBrush;

		if (FlowGraphNode->NodeBreakpoint.IsHit())
		{
			NodeBrush.Brush = FFlowEditorStyle::Get()->GetBrush(TEXT("FlowGraph.BreakpointHit"));
			NodeBrush.OverlayOffset.X = WidgetSize.X - 12.0f;
		}
		else
		{
			NodeBrush.Brush = FFlowEditorStyle::Get()->GetBrush(FlowGraphNode->NodeBreakpoint.IsEnabled() ? TEXT("FlowGraph.BreakpointEnabled") : TEXT("FlowGraph.BreakpointDisabled"));
			NodeBrush.OverlayOffset.X = WidgetSize.X;
		}

		NodeBrush.OverlayOffset.Y = -NodeBrush.Brush->ImageSize.Y;
		NodeBrush.AnimationEnvelope = FVector2D(0.f, 10.f);
		Brushes.Add(NodeBrush);
	}

	// Pin breakpoints
	for (const TPair<FEdGraphPinReference, FFlowPinTrait>& PinBreakpoint : FlowGraphNode->PinBreakpoints)
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

void SFlowGraphNode::GetPinBrush(const bool bLeftSide, const float WidgetWidth, const int32 PinIndex, const FFlowPinTrait& Breakpoint, TArray<FOverlayBrushInfo>& Brushes) const
{
	if (Breakpoint.IsAllowed())
	{
		FOverlayBrushInfo PinBrush;

		if (Breakpoint.IsHit())
		{
			PinBrush.Brush = FFlowEditorStyle::Get()->GetBrush(TEXT("FlowGraph.PinBreakpointHit"));
			PinBrush.OverlayOffset.X = bLeftSide ? 0.0f : (WidgetWidth - 36.0f);
			PinBrush.OverlayOffset.Y = 12.0f + PinIndex * 28.0f;
		}
		else
		{
			PinBrush.Brush = FFlowEditorStyle::Get()->GetBrush(Breakpoint.IsEnabled() ? TEXT("FlowGraph.BreakpointEnabled") : TEXT("FlowGraph.BreakpointDisabled"));
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

	// Compute the SubNode padding indent based on the parentage depth for this node
	const FMargin NodePadding = ComputeSubNodeChildIndentPaddingMargin();

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
				.BorderBackgroundColor(this, &SFlowGraphNode::GetBorderBackgroundColor)
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
			.Padding(FMargin(NodePadding.Left, 0.0f, NodePadding.Right, 0.0f))
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

FSlateColor SFlowGraphNode::GetBorderBackgroundColor() const
{
	return SGraphNode::GetNodeTitleColor();
}

FSlateColor SFlowGraphNode::GetConfigBoxBackgroundColor() const
{
	FLinearColor NodeColor = ConfigBoxColor;

	if (FlowGraphNode && !IsFlowGraphNodeSelected(FlowGraphNode))
	{
		NodeColor *= UnselectedNodeTint;
	}

	return NodeColor;
}

void SFlowGraphNode::CreateBelowPinControls(TSharedPtr<SVerticalBox> InnerVerticalBox)
{
	static const FMargin ConfigBoxPadding = FMargin(2.0f, 0.0f, 1.0f, 0.0);

	// Add a box to wrap around the Config Text area to make it a more visually distinct part of the node
	TSharedPtr<SVerticalBox> BelowPinsBox;
	InnerVerticalBox->AddSlot()
		.AutoHeight()
		.Padding(ConfigBoxPadding)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
			.BorderBackgroundColor(this, &SFlowGraphNode::GetConfigBoxBackgroundColor)
			.Visibility(this, &SFlowGraphNode::GetNodeConfigTextVisibility)
			[
				SAssignNew(BelowPinsBox, SVerticalBox)
			]
		];

	CreateConfigText(BelowPinsBox);

	CreateOrRebuildSubNodeBox(InnerVerticalBox);
}

void SFlowGraphNode::AddSubNodeWidget(TSharedPtr<SGraphNode> NewSubNodeWidget)
{
	if (OwnerGraphPanelPtr.IsValid())
	{
		NewSubNodeWidget->SetOwner(OwnerGraphPanelPtr.Pin().ToSharedRef());
		OwnerGraphPanelPtr.Pin()->AttachGraphEvents(NewSubNodeWidget);
	}
	NewSubNodeWidget->UpdateGraphNode();

	AddSubNode(NewSubNodeWidget);
}

FMargin SFlowGraphNode::ComputeSubNodeChildIndentPaddingMargin() const
{
	if (!IsValid(FlowGraphNode) || !FlowGraphNode->IsSubNode())
	{
		return FMargin();
	}

	const UFlowGraphNode* CurrentAncestor = FlowGraphNode->GetParentNode();

	// Compute the parent depth, so it can be used to determine the indent level for this subnode
	int32 ParentDepth = 0;
	while (IsValid(CurrentAncestor))
	{
		++ParentDepth;

		CurrentAncestor = CurrentAncestor->GetParentNode();
	}

	constexpr float VerticalDefaultPadding = 2.0f;
	constexpr float HorizontalDefaultPadding = 2.0f;
	constexpr float IndentedHorizontalPadding = 6.0f;
	constexpr float RightPadding = HorizontalDefaultPadding;
	float LeftPadding = HorizontalDefaultPadding;

	if (ParentDepth > 0)
	{
		// Increase the padding by the parent depth for this node
		LeftPadding = IndentedHorizontalPadding * ParentDepth;
	}
	else
	{
		LeftPadding = 0.0f;
	}

	return
		FMargin(
			LeftPadding,
			VerticalDefaultPadding,
			RightPadding,
			VerticalDefaultPadding);
}

void SFlowGraphNode::CreateConfigText(TSharedPtr<SVerticalBox> InnerVerticalBox)
{
	static const FMargin ConfigTextPadding = FMargin(2.0f, 0.0f, 0.0f, 3.0f);

	InnerVerticalBox->AddSlot()
		.AutoHeight()
		.Padding(ConfigTextPadding)
		[
			SAssignNew(ConfigTextBlock, STextBlock)
			.AutoWrapText(true)
			.LineBreakPolicy(FBreakIterator::CreateWordBreakIterator())
			.Text(this, &SFlowGraphNode::GetNodeConfigText)
		];
}

FText SFlowGraphNode::GetNodeConfigText() const
{
	if (const UFlowGraphNode* MyNode = CastChecked<UFlowGraphNode>(GraphNode))
	{
		if (UFlowNodeBase* NodeInstance = MyNode->GetFlowNodeBase())
		{
			return NodeInstance->GetNodeConfigText();
		}
	}

	return FText::GetEmpty();
}

EVisibility SFlowGraphNode::GetNodeConfigTextVisibility() const
{
	// Hide in lower LODs
	const TSharedPtr<SGraphPanel> OwnerPanel = GetOwnerPanel();
	if (!OwnerPanel.IsValid() || OwnerPanel->GetCurrentLOD() > EGraphRenderingLOD::MediumDetail)
	{
		if (ConfigTextBlock && !ConfigTextBlock->GetText().IsEmptyOrWhitespace())
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

void SFlowGraphNode::CreateOrRebuildSubNodeBox(TSharedPtr<SVerticalBox> InnerVerticalBox)
{
	if (SubNodeBox.IsValid())
	{
		SubNodeBox->ClearChildren();
	}
	else
	{
		SAssignNew(SubNodeBox, SVerticalBox);
	}

	SubNodes.Reset();

	if (FlowGraphNode)
	{
		for (UFlowGraphNode* SubNode : FlowGraphNode->SubNodes)
		{
			TSharedPtr<SGraphNode> NewNode = FNodeFactory::CreateNodeWidget(SubNode);
			AddSubNodeWidget(NewNode);
		}
	}

	InnerVerticalBox->AddSlot()
		.AutoHeight()
		[
			SubNodeBox.ToSharedRef()
		];
}

bool SFlowGraphNode::IsFlowGraphNodeSelected(UFlowGraphNode* Node) const
{
	return GetOwnerPanel().IsValid() && GetOwnerPanel()->SelectionManager.SelectedNodes.Contains(Node);
}

void SFlowGraphNode::UpdateErrorInfo()
{
	if (const UFlowNodeBase* FlowNodeBase = FlowGraphNode->GetFlowNodeBase())
	{
		if (FlowNodeBase->ValidationLog.Messages.Num() > 0)
		{
			EMessageSeverity::Type MaxSeverity = EMessageSeverity::Info;
			for (const TSharedRef<FTokenizedMessage>& Message : FlowNodeBase->ValidationLog.Messages)
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
				default: 
					break;
			}

			return;
		}

		if (FlowNodeBase->GetClass()->HasAnyClassFlags(CLASS_Deprecated) || FlowNodeBase->bNodeDeprecated)
		{
			ErrorMsg = FlowNodeBase->ReplacedBy ? FString::Printf(TEXT(" REPLACED BY: %s "), *FlowNodeBase->ReplacedBy->GetName()) : FString(TEXT(" DEPRECATED! "));
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

	if (!IsFlowGraphNodeSelected(FlowGraphNode) && FlowGraphNode->IsSubNode())
	{
		ReturnTitleColor *= UnselectedNodeTint;
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
	else if (!IsFlowGraphNodeSelected(FlowGraphNode) && FlowGraphNode->IsSubNode())
	{
		ReturnBodyColor *= UnselectedNodeTint;
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
	else if (!IsFlowGraphNodeSelected(FlowGraphNode) && FlowGraphNode->IsSubNode())
	{
		ReturnIconColor *= UnselectedNodeTint;
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
	else if (!IsFlowGraphNodeSelected(FlowGraphNode) && FlowGraphNode->IsSubNode())
	{
		ReturnTextColor *= UnselectedNodeTint;
	}

	return ReturnTextColor;
}

TSharedPtr<SWidget> SFlowGraphNode::GetEnabledStateWidget() const
{
	if (FlowGraphNode->IsSubNode())
	{
		// SubNodes don't get enabled/disabled on their own,
		//  they follow the enabled/disabled setting of their owning flow node

		return TSharedPtr<SWidget>();
	}

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
				.VAlign(VAlign_Center)
				.Padding(0, 0, 7, 0)
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
			.Padding(7, 0, 0, 0)
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
		default:
			break;
	}

	return FReply::Handled();
}

void SFlowGraphNode::AddSubNode(TSharedPtr<SGraphNode> SubNodeWidget)
{
	SubNodes.Add(SubNodeWidget);

	SubNodeBox->AddSlot().AutoHeight()
		[
			SubNodeWidget.ToSharedRef()
		];
}

FText SFlowGraphNode::GetTitle() const
{
	return GraphNode ? GraphNode->GetNodeTitle(ENodeTitleType::FullTitle) : FText::GetEmpty();
}

FText SFlowGraphNode::GetDescription() const
{
	UFlowGraphNode* MyNode = CastChecked<UFlowGraphNode>(GraphNode);
	return MyNode ? MyNode->GetDescription() : FText::GetEmpty();
}

EVisibility SFlowGraphNode::GetDescriptionVisibility() const
{
	// LOD this out once things get too small
	TSharedPtr<SGraphPanel> MyOwnerPanel = GetOwnerPanel();
	return (!MyOwnerPanel.IsValid() || MyOwnerPanel->GetCurrentLOD() > EGraphRenderingLOD::LowDetail) ? EVisibility::Visible : EVisibility::Collapsed;
}

void SFlowGraphNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));

	const UEdGraphPin* PinObj = PinToAdd->GetPinObj();
	const bool bAdvancedParameter = PinObj && PinObj->bAdvancedView;
	if (bAdvancedParameter)
	{
		PinToAdd->SetVisibility(TAttribute<EVisibility>(PinToAdd, &SGraphPin::IsPinVisibleAsAdvanced));
	}

	if (PinToAdd->GetDirection() == EEdGraphPinDirection::EGPD_Input)
	{
		LeftNodeBox->AddSlot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(Settings->GetInputPinPadding())
			[
				PinToAdd
			];
		InputPins.Add(PinToAdd);
	}
	else // Direction == EEdGraphPinDirection::EGPD_Output
	{
		RightNodeBox->AddSlot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(Settings->GetOutputPinPadding())
			[
				PinToAdd
			];
		OutputPins.Add(PinToAdd);
	}
}

FReply SFlowGraphNode::OnMouseMove(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && !(GEditor->bIsSimulatingInEditor || GEditor->PlayWorld))
	{
		// if we are holding mouse over a subnode
		UFlowGraphNode* TestNode = Cast<UFlowGraphNode>(GraphNode);
		if (TestNode && TestNode->IsSubNode())
		{
			const TSharedRef<SGraphPanel>& Panel = GetOwnerPanel().ToSharedRef();
			const TSharedRef<SGraphNode>& Node = SharedThis(this);
			return FReply::Handled().BeginDragDrop(FDragFlowGraphNode::New(Panel, Node));
		}
	}

	if (!MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && bDragMarkerVisible)
	{
		SetDragMarker(false);
	}

	return FReply::Unhandled();
}

TSharedRef<SGraphNode> SFlowGraphNode::GetNodeUnderMouse(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const TSharedPtr<SGraphNode> SubNode = GetSubNodeUnderCursor(MyGeometry, MouseEvent);
	if (SubNode.IsValid())
	{
		return SubNode.ToSharedRef();
	}
	else
	{
		return StaticCastSharedRef<SGraphNode>(AsShared());
	}
}

FReply SFlowGraphNode::OnMouseButtonDown(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent)
{
	UFlowGraphNode* TestNode = Cast<UFlowGraphNode>(GraphNode);
	if (TestNode && TestNode->IsSubNode())
	{
		GetOwnerPanel()->SelectionManager.ClickedOnNode(GraphNode, MouseEvent);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

TSharedPtr<SGraphNode> SFlowGraphNode::GetSubNodeUnderCursor(const FGeometry& WidgetGeometry, const FPointerEvent& MouseEvent)
{
	// We just need to find the one WidgetToFind among our descendants.
	TSet< TSharedRef<SWidget> > SubWidgetsSet;
	for (int32 i = 0; i < SubNodes.Num(); i++)
	{
		SubWidgetsSet.Add(SubNodes[i].ToSharedRef());
	}

	TMap<TSharedRef<SWidget>, FArrangedWidget> Result;
	FindChildGeometries(WidgetGeometry, SubWidgetsSet, Result);

	TSharedPtr<SGraphNode> ResultNode;

	if (Result.Num() <= 0)
	{
		return ResultNode;
	}

	FArrangedChildren ArrangedChildren(EVisibility::Visible);
	Result.GenerateValueArray(ArrangedChildren.GetInternalArray());

	const int32 HoveredIndex = SWidget::FindChildUnderMouse(ArrangedChildren, MouseEvent);
	if (HoveredIndex == INDEX_NONE)
	{
		return ResultNode;
	}

	ResultNode = StaticCastSharedRef<SGraphNode>(ArrangedChildren[HoveredIndex].Widget);

	// Recurse if the subnode has subnodes
	SFlowGraphNode* ResultFlowGraphNode = static_cast<SFlowGraphNode*>(ResultNode.Get());
	const FGeometry& ChildWidgetGeometry = ArrangedChildren[HoveredIndex].Geometry;
	TSharedPtr<SGraphNode> ResultFlowGraphNodeSubNode = ResultFlowGraphNode->GetSubNodeUnderCursor(ChildWidgetGeometry, MouseEvent);

	if (ResultFlowGraphNodeSubNode)
	{
		return ResultFlowGraphNodeSubNode;
	}

	return ResultNode;
}

void SFlowGraphNode::SetDragMarker(bool bEnabled)
{
	bDragMarkerVisible = bEnabled;
}

EVisibility SFlowGraphNode::GetDragOverMarkerVisibility() const
{
	return bDragMarkerVisible ? EVisibility::Visible : EVisibility::Collapsed;
}

void SFlowGraphNode::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	// Is someone dragging a node?
	TSharedPtr<FDragNode> DragConnectionOp = DragDropEvent.GetOperationAs<FDragNode>();
	if (DragConnectionOp.IsValid())
	{
		// Inform the Drag and Drop operation that we are hovering over this node.
		TSharedPtr<SGraphNode> SubNode = GetSubNodeUnderCursor(MyGeometry, DragDropEvent);
		DragConnectionOp->SetHoveredNode(SubNode.IsValid() ? SubNode : SharedThis(this));

		UFlowGraphNode* TestNode = Cast<UFlowGraphNode>(GraphNode);
		if (DragConnectionOp->IsValidOperation() && TestNode && TestNode->IsSubNode())
		{
			SetDragMarker(true);
		}
	}

	SGraphNode::OnDragEnter(MyGeometry, DragDropEvent);
}

FReply SFlowGraphNode::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	// Is someone dragging a node?
	TSharedPtr<FDragNode> DragConnectionOp = DragDropEvent.GetOperationAs<FDragNode>();
	if (DragConnectionOp.IsValid())
	{
		// Inform the Drag and Drop operation that we are hovering over this node.
		TSharedPtr<SGraphNode> SubNode = GetSubNodeUnderCursor(MyGeometry, DragDropEvent);
		DragConnectionOp->SetHoveredNode(SubNode.IsValid() ? SubNode : SharedThis(this));
	}
	return SGraphNode::OnDragOver(MyGeometry, DragDropEvent);
}

void SFlowGraphNode::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDragNode> DragConnectionOp = DragDropEvent.GetOperationAs<FDragNode>();
	if (DragConnectionOp.IsValid())
	{
		// Inform the Drag and Drop operation that we are not hovering any pins
		DragConnectionOp->SetHoveredNode(TSharedPtr<SGraphNode>(NULL));
	}

	SetDragMarker(false);
	SGraphNode::OnDragLeave(DragDropEvent);
}

FReply SFlowGraphNode::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	SetDragMarker(false);

	TSharedPtr<FDragFlowGraphNode> DragNodeOp = DragDropEvent.GetOperationAs<FDragFlowGraphNode>();
	if (DragNodeOp.IsValid())
	{
		if (!DragNodeOp->IsValidOperation())
		{
			return FReply::Handled();
		}

		const float DragTime = float(FPlatformTime::Seconds() - DragNodeOp->StartTime);
		if (DragTime < 0.5f)
		{
			return FReply::Handled();
		}

		UFlowGraphNode* MyNode = Cast<UFlowGraphNode>(GraphNode);
		if (MyNode == nullptr)
		{
			return FReply::Unhandled();
		}

		const FScopedTransaction Transaction(LOCTEXT("GraphEd_DragDropNode", "Drag&Drop Node"));

		UFlowGraphNode* DropTargetNode = DragNodeOp->GetDropTargetNode();
		check(DropTargetNode);

		bool bReorderOperation = true;
		const TArray< TSharedRef<SGraphNode> >& DraggedNodes = DragNodeOp->GetNodes();
		RemoveDraggedSubNodes(DraggedNodes, bReorderOperation);

		bool bShouldDropAsSubNodesOfDropTargetNode = bReorderOperation || ShouldDropDraggedNodesAsSubNodes(DraggedNodes, DropTargetNode);

		// Setup the DropTarget pointers based on the type of drop we've decided to do:
		UFlowGraphNode* DropTargetParentNode = MyNode;

		if (bShouldDropAsSubNodesOfDropTargetNode)
		{
			DropTargetParentNode = DropTargetNode;
			DropTargetNode = nullptr;
		}
		else
		{
			DropTargetParentNode = DropTargetNode->GetParentNode();
		}

		check(DropTargetParentNode);

		const int32 InsertIndex = DropTargetParentNode->FindSubNodeDropIndex(DropTargetNode);

		for (int32 Idx = 0; Idx < DraggedNodes.Num(); Idx++)
		{
			UFlowGraphNode* DraggedTestNode = Cast<UFlowGraphNode>(DraggedNodes[Idx]->GetNodeObj());
			DraggedTestNode->Modify();
			DraggedTestNode->SetParentNodeForSubNode(DropTargetParentNode);

			DropTargetParentNode->Modify();
			DropTargetParentNode->InsertSubNodeAt(DraggedTestNode, InsertIndex);

			DropTargetParentNode->OnSubNodeAdded(DraggedTestNode);
		}

		if (bReorderOperation)
		{
			UpdateGraphNode();
		}
		else
		{
			UFlowGraph* MyGraph = DropTargetParentNode->GetFlowGraph();
			if (DropTargetParentNode)
			{
				MyGraph->OnSubNodeDropped();
			}
		}
	}

	return SGraphNode::OnDrop(MyGeometry, DragDropEvent);
}

bool SFlowGraphNode::ShouldDropDraggedNodesAsSubNodes(const TArray<TSharedRef<SGraphNode>>& DraggedNodes, UFlowGraphNode* DropTargetNode) const
{
	TSet<const UEdGraphNode*> DraggedFlowGraphNodes;
	for (int32 Idx = 0; Idx < DraggedNodes.Num(); Idx++)
	{
		UFlowGraphNode* DraggedNode = Cast<UFlowGraphNode>(DraggedNodes[Idx]->GetNodeObj());
		if (IsValid(DraggedNode))
		{
			DraggedFlowGraphNodes.Add(DraggedNode);
		}
	}

	for (TSet<const UEdGraphNode*>::TConstIterator It(DraggedFlowGraphNodes); It; ++It)
	{
		const UFlowGraphNode* DraggedNode = Cast<UFlowGraphNode>(*It);

		// Check if all of the dragged nodes can be stopped as a subnode 
		//  (if not ALL, then we cannot drop ANY of them)
		const bool bCanDropDraggedNodeAsSubNode = DropTargetNode->CanAcceptSubNodeAsChild(*DraggedNode, DraggedFlowGraphNodes);

		if (!bCanDropDraggedNodeAsSubNode)
		{
			return false;
		}
	}

	return true;
}

void SFlowGraphNode::RemoveDraggedSubNodes(const TArray< TSharedRef<SGraphNode> >& DraggedNodes, bool& bInOutReorderOperation)
{
	for (int32 Idx = 0; Idx < DraggedNodes.Num(); Idx++)
	{
		UFlowGraphNode* DraggedNode = Cast<UFlowGraphNode>(DraggedNodes[Idx]->GetNodeObj());
		if (DraggedNode && DraggedNode->GetParentNode())
		{
			if (DraggedNode->GetParentNode() != GraphNode)
			{
				bInOutReorderOperation = false;
			}

			DraggedNode->GetParentNode()->RemoveSubNode(DraggedNode);
		}
	}
}

FText SFlowGraphNode::GetPreviewCornerText() const
{
	return FText::GetEmpty();
}

const FSlateBrush* SFlowGraphNode::GetNameIcon() const
{
	return FAppStyle::GetBrush(TEXT("Graph.StateNode.Icon"));
}

void SFlowGraphNode::SetOwner(const TSharedRef<SGraphPanel>& OwnerPanel)
{
	SGraphNode::SetOwner(OwnerPanel);

	for (auto& ChildWidget : SubNodes)
	{
		if (ChildWidget.IsValid())
		{
			ChildWidget->SetOwner(OwnerPanel);
			OwnerPanel->AttachGraphEvents(ChildWidget);
		}
	}
}

#undef LOCTEXT_NAMESPACE
