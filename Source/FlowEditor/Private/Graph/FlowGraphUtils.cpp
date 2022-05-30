// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraphUtils.h"
#include "Asset/FlowAssetEditor.h"
#include "Graph/FlowGraph.h"

#include "FlowAsset.h"

#include "Toolkits/ToolkitManager.h"

TSharedPtr<FFlowAssetEditor> FFlowGraphUtils::GetFlowAssetEditor(const UObject* ObjectToFocusOn)
{
	check(ObjectToFocusOn);

	TSharedPtr<FFlowAssetEditor> FlowAssetEditor;
	if (UFlowAsset* FlowAsset = Cast<const UFlowGraph>(ObjectToFocusOn)->GetFlowAsset())
	{
		const TSharedPtr<IToolkit> FoundAssetEditor = FToolkitManager::Get().FindEditorForAsset(FlowAsset);
		if (FoundAssetEditor.IsValid())
		{
			FlowAssetEditor = StaticCastSharedPtr<FFlowAssetEditor>(FoundAssetEditor);
		}
	}
	return FlowAssetEditor;
}
