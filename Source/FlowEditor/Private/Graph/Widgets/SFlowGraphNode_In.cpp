#include "Graph/Widgets/SFlowGraphNode_In.h"
#include "Widgets/SBoxPanel.h"

TSharedRef<SWidget> SFlowGraphNode_In::CreateNodeContentArea()
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
					SAssignNew(LeftNodeBox, SVerticalBox)
				]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				[
					SAssignNew(RightNodeBox, SVerticalBox)
				]
		];
}
