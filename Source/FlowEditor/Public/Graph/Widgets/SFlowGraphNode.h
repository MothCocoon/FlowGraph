// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "SGraphNode.h"
#include "KismetPins/SGraphPinExec.h"

#include "Graph/Nodes/FlowGraphNode.h"

class FLOWEDITOR_API SFlowGraphPinExec : public SGraphPinExec
{
public:
	SFlowGraphPinExec();

	SLATE_BEGIN_ARGS(SFlowGraphPinExec) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin);
};

class FLOWEDITOR_API SFlowGraphNode : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SFlowGraphNode) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UFlowGraphNode* InNode);

	virtual ~SFlowGraphNode();
	
protected:
	// SNodePanel::SNode
	virtual void GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const override;
	virtual const FSlateBrush* GetShadowBrush(bool bSelected) const override;
	virtual void GetOverlayBrushes(bool bSelected, const FVector2D WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const override;
	// --

	// SGraphNode
	virtual void GetPinBrush(const bool bLeftSide, const float WidgetWidth, const int32 PinIndex, const FFlowPinTrait& Breakpoint, TArray<FOverlayBrushInfo>& Brushes) const;

	virtual FText GetTitle() const;
	virtual FText GetDescription() const;
	virtual EVisibility GetDescriptionVisibility() const;

	virtual FText GetPreviewCornerText() const;
	virtual const FSlateBrush* GetNameIcon() const;

	virtual FSlateColor GetBorderBackgroundColor() const;

	virtual FSlateColor GetConfigBoxBackgroundColor() const;

	/** adds subnode widget inside current node */
	virtual void AddSubNode(TSharedPtr<SGraphNode> SubNodeWidget);
	// --

	// SGraphNode Interface
	virtual TSharedPtr<SToolTip> GetComplexTooltip() override;
	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnMouseMove(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent) override;
	virtual TSharedRef<SGraphNode> GetNodeUnderMouse(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void SetOwner(const TSharedRef<SGraphPanel>& OwnerPanel) override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;

	virtual void UpdateGraphNode() override;
	virtual void UpdateErrorInfo() override;

	virtual TSharedRef<SWidget> CreateTitleWidget(TSharedPtr<SNodeTitle> NodeTitle) override;
	virtual TSharedRef<SWidget> CreateNodeContentArea() override;
	virtual void CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox) override;
	virtual const FSlateBrush* GetNodeBodyBrush() const override;

	virtual void CreateInputSideAddButton(TSharedPtr<SVerticalBox> OutputBox) override;
	virtual void CreateOutputSideAddButton(TSharedPtr<SVerticalBox> OutputBox) override;
	// --

	// SWidget
	virtual FReply OnMouseButtonDown(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent) override;
	// --

	// purposely overriden non-virtual methods, added PR #9791 to made these methods virtual: https://github.com/EpicGames/UnrealEngine/pull/9791
	FSlateColor GetNodeTitleColor() const;
	FSlateColor GetNodeBodyColor() const;
	FSlateColor GetNodeTitleIconColor() const;
	FLinearColor GetNodeTitleTextColor() const;
	TSharedPtr<SWidget> GetEnabledStateWidget() const;
	// --
	
	// Variant of SGraphNode::AddPinButtonContent
	virtual void AddPinButton(TSharedPtr<SVerticalBox> OutputBox, TSharedRef<SWidget> ButtonContent, const EEdGraphPinDirection Direction, FString DocumentationExcerpt = FString(), TSharedPtr<SToolTip> CustomTooltip = nullptr);

	// Variant of SGraphNode::OnAddPin
	virtual FReply OnAddFlowPin(const EEdGraphPinDirection Direction);

protected:
	/** adds a sub node widget inside current node */
	void AddSubNodeWidget(TSharedPtr<SGraphNode> NewSubNodeWidget);
	
	/** removes dragged subnodes from the current node, 
	  * bInOutReorderOperation reports if this is a simple "reorder" internally within the node or 
	  * if one or more of the removed SubNodes will be removed from the node completely */
	void RemoveDraggedSubNodes(const TArray< TSharedRef<SGraphNode> >& DraggedNodes, bool& bInOutReorderOperation);

	bool ShouldDropDraggedNodesAsSubNodes(const TArray<TSharedRef<SGraphNode>>& DraggedNodes, UFlowGraphNode* DropTargetNode) const;

	/** gets decorator or service node if one is found under mouse cursor */
	TSharedPtr<SGraphNode> GetSubNodeUnderCursor(const FGeometry& WidgetGeometry, const FPointerEvent& MouseEvent);

	/** gets drag over marker visibility */
	EVisibility GetDragOverMarkerVisibility() const;

	/** sets drag marker visible or collapsed on this node */
	void SetDragMarker(bool bEnabled);

	FMargin ComputeSubNodeChildIndentPaddingMargin() const;

	void CreateConfigText(TSharedPtr<SVerticalBox> MainBox);
	FText GetNodeConfigText() const;
	EVisibility GetNodeConfigTextVisibility() const;

	void CreateOrRebuildSubNodeBox(TSharedPtr<SVerticalBox> MainBox);

	bool IsFlowGraphNodeSelected(UFlowGraphNode* Node) const;

protected:
	// The FlowGraphNode this slate widget is representing
	UFlowGraphNode* FlowGraphNode = nullptr;

	// SubNodes that are embedded in this widget
	TArray<TSharedPtr<SGraphNode>> SubNodes;

	// Is the drag marker currently visible?
	bool bDragMarkerVisible = false;

	// Box to hold the SubNode widgets
	TSharedPtr<SVerticalBox> SubNodeBox;

	// Config Text Block widget
	TSharedPtr<STextBlock> ConfigTextBlock;

public:
	static const FLinearColor UnselectedNodeTint;
	static const FLinearColor ConfigBoxColor;
};
