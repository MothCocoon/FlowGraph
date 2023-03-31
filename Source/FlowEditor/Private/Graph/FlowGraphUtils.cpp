// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraphUtils.h"
#include "Asset/FlowAssetEditor.h"
#include "Graph/FlowGraph.h"

#include "FlowAsset.h"

#include "Toolkits/ToolkitManager.h"

TSharedPtr<FFlowAssetEditor> FFlowGraphUtils::GetFlowAssetEditor(const UEdGraph* Graph)
{
	check(Graph);

	TSharedPtr<FFlowAssetEditor> FlowAssetEditor;
	if (const UFlowAsset* FlowAsset = Cast<const UFlowGraph>(Graph)->GetFlowAsset())
	{
		const TSharedPtr<IToolkit> FoundAssetEditor = FToolkitManager::Get().FindEditorForAsset(FlowAsset);
		if (FoundAssetEditor.IsValid())
		{
			FlowAssetEditor = StaticCastSharedPtr<FFlowAssetEditor>(FoundAssetEditor);
		}
	}
	return FlowAssetEditor;
}

TSharedPtr<SFlowGraphEditor> FFlowGraphUtils::GetFlowGraphEditor(const UEdGraph* Graph)
{
	TSharedPtr<SFlowGraphEditor> FlowGraphEditor;
	
	const TSharedPtr<FFlowAssetEditor> FlowEditor = GetFlowAssetEditor(Graph);
	if (FlowEditor.IsValid())
	{
		FlowGraphEditor = FlowEditor->GetFlowGraph();
	}

	return FlowGraphEditor;
}
