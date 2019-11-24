#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Layout/Visibility.h"
#include "SGraphNode.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class SVerticalBox;
class UFlowGraphNode;

class SFlowGraphNode : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SFlowGraphNode){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, class UFlowGraphNode* InNode);

protected:
	// SNodePanel::SNode interface
	virtual void GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const override;
	// End of SNodePanel::SNode interface

	// SGraphNode Interface
	virtual void CreateOutputSideAddButton(TSharedPtr<SVerticalBox> OutputBox) override;
	virtual EVisibility IsAddPinButtonVisible() const override;
	virtual FReply OnAddPin() override;
	// End SGraphNode Interface

private:
	UFlowGraphNode* FlowGraphNode;
};
