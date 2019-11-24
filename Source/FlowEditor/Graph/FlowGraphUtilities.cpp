#include "FlowGraphUtilities.h"
#include "../FlowEditorModule.h"
#include "FlowAssetGraph.h"
#include "IFlowAssetEditor.h"
#include "Nodes/FlowGraphNode.h"

#include "Flow/Graph/FlowAsset.h"

#include "GraphEditor.h"
#include "Toolkits/ToolkitManager.h"

bool FFlowGraphUtilities::CanPasteNodes(const UEdGraph* Graph)
{
	bool bCanPaste = false;
	TSharedPtr<IFlowAssetEditor> FlowAssetEditor = GetIFlowAssetEditorForObject(Graph);
	if (FlowAssetEditor.IsValid())
	{
		bCanPaste = FlowAssetEditor->CanPasteNodes();
	}
	return bCanPaste;
}

void FFlowGraphUtilities::PasteNodesHere(UEdGraph* Graph, const FVector2D& Location)
{
	TSharedPtr<IFlowAssetEditor> FlowAssetEditor = GetIFlowAssetEditorForObject(Graph);
	if (FlowAssetEditor.IsValid())
	{
		FlowAssetEditor->PasteNodesHere(Location);
	}
}

bool FFlowGraphUtilities::GetBoundsForSelectedNodes(const UEdGraph* Graph, FSlateRect& Rect, float Padding)
{
	TSharedPtr<IFlowAssetEditor> FlowAssetEditor = GetIFlowAssetEditorForObject(Graph);
	if (FlowAssetEditor.IsValid())
	{
		return FlowAssetEditor->GetBoundsForSelectedNodes(Rect, Padding);
	}
	return false;
}

int32 FFlowGraphUtilities::GetNumberOfSelectedNodes(const UEdGraph* Graph)
{
	TSharedPtr<IFlowAssetEditor> FlowAssetEditor = GetIFlowAssetEditorForObject(Graph);
	if (FlowAssetEditor.IsValid())
	{
		return FlowAssetEditor->GetNumberOfSelectedNodes();
	}
	return 0;
}

TSet<UObject*> FFlowGraphUtilities::GetSelectedNodes(const UEdGraph* Graph)
{
	TSharedPtr<IFlowAssetEditor> FlowAssetEditor = GetIFlowAssetEditorForObject(Graph);
	if (FlowAssetEditor.IsValid())
	{
		return FlowAssetEditor->GetSelectedNodes();
	}
	return TSet<UObject*>();
}

TSharedPtr<IFlowAssetEditor> FFlowGraphUtilities::GetIFlowAssetEditorForObject(const UObject* ObjectToFocusOn)
{
	check(ObjectToFocusOn);

	TSharedPtr<IFlowAssetEditor> FlowAssetEditor;
	if (UFlowAsset* FlowAsset = Cast<const UFlowAssetGraph>(ObjectToFocusOn)->GetFlowAsset())
	{
		TSharedPtr<IToolkit> FoundAssetEditor = FToolkitManager::Get().FindEditorForAsset(FlowAsset);
		if (FoundAssetEditor.IsValid())
		{
			FlowAssetEditor = StaticCastSharedPtr<IFlowAssetEditor>(FoundAssetEditor);
		}
	}
	return FlowAssetEditor;
}
