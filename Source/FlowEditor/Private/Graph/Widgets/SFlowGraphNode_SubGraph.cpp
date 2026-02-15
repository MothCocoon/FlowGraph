// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/Widgets/SFlowGraphNode_SubGraph.h"
#include "Graph/FlowGraphEditorSettings.h"

#include "FlowAsset.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"

#include "SGraphPreviewer.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SToolTip.h"

#define LOCTEXT_NAMESPACE "SFlowGraphNode_SubGraph"

TSharedPtr<SToolTip> SFlowGraphNode_SubGraph::GetComplexTooltip()
{
	const UFlowGraphEditorSettings* GraphEditorSettings = GetDefault<UFlowGraphEditorSettings>();
	if (GraphEditorSettings->bShowSubGraphPreview && FlowGraphNode)
	{
		if (UFlowNode* FlowNode = Cast<UFlowNode>(FlowGraphNode->GetFlowNodeBase()))
		{
			const UFlowAsset* AssetToEdit = Cast<UFlowAsset>(FlowNode->GetAssetToEdit());
			if (AssetToEdit && AssetToEdit->GetGraph())
			{
				TSharedPtr<SWidget> TitleBarWidget = SNullWidget::NullWidget;
				if (GraphEditorSettings->bShowSubGraphPath)
				{
					FString CleanAssetName = AssetToEdit->GetPathName(nullptr);
					const int32 SubStringIdx = CleanAssetName.Find(".", ESearchCase::IgnoreCase, ESearchDir::FromEnd);
					CleanAssetName.LeftInline(SubStringIdx);
					
					TitleBarWidget = SNew(SBox)
					.Padding(10.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(CleanAssetName))
					];
				}
				
				return SNew(SToolTip)
				[
					SNew(SBox)
					.WidthOverride(GraphEditorSettings->SubGraphPreviewSize.X)
					.HeightOverride(GraphEditorSettings->SubGraphPreviewSize.Y)
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							SNew(SGraphPreviewer, AssetToEdit->GetGraph())
							.CornerOverlayText(LOCTEXT("FlowNodePreviewGraphOverlayText", "GRAPH PREVIEW"))
							.ShowGraphStateOverlay(false)
							.TitleBar(TitleBarWidget)
						]
					]
				];
			}
		}
	}

	return SFlowGraphNode::GetComplexTooltip();
}

#undef LOCTEXT_NAMESPACE
