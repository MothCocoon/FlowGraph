#pragma once

#include "SGraphNode.h"
#include "KismetPins/SGraphPinExec.h"

#include "Graph/Nodes/FlowGraphNode.h"

class SFlowGraphPinExec final : public SGraphPinExec
{
public:
	SFlowGraphPinExec();

	SLATE_BEGIN_ARGS(SFlowGraphPinExec) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin);
	
	// SWidget
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	// --
};

class SFlowGraphNode : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SFlowGraphNode) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UFlowGraphNode* InNode);

protected:
	// SNodePanel::SNode
	virtual void GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const override;
	virtual const FSlateBrush* GetShadowBrush(bool bSelected) const override;
	virtual void GetOverlayBrushes(bool bSelected, const FVector2D WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const override;
	// --

	virtual void GetPinBrush(const bool bLeftSide, const float WidgetWidth, const int32 PinIndex, const FFlowBreakpoint& Breakpoint, TArray<FOverlayBrushInfo>& Brushes) const;

	// SGraphNode
	virtual void UpdateGraphNode() override;
	virtual TSharedRef<SWidget> CreateNodeContentArea() override;

	// backported from 4.23
	virtual const FSlateBrush* GetNodeBodyBrush() const;

	virtual void CreateStandardPinWidget(UEdGraphPin* Pin) override;
	virtual void CreateInputSideAddButton(TSharedPtr<SVerticalBox> OutputBox) override;
	virtual void CreateOutputSideAddButton(TSharedPtr<SVerticalBox> OutputBox) override;
	virtual FReply OnAddPin() override;
	// --

private:
	UFlowGraphNode* FlowGraphNode = nullptr;
};
