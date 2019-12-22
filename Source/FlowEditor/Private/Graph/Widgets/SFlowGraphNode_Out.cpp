#include "SFlowGraphNode_Out.h"
#include "../Nodes/FlowGraphNode_Out.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"

void SFlowGraphNode_Out::Construct(const FArguments& InArgs, UFlowGraphNode_Out* InNode)
{
	GraphNode = InNode;

	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

TSharedRef<SWidget> SFlowGraphNode_Out::CreateNodeContentArea()
{
	return SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("NoBorder"))
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Padding(FMargin(0, 3))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.FillWidth(1.0f)
			[
				// LEFT
				SAssignNew(LeftNodeBox, SVerticalBox)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			[
				// RIGHT
				SAssignNew(RightNodeBox, SVerticalBox)
			]
		];
}