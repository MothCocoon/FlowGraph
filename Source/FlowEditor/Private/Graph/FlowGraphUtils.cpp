#include "Graph/FlowGraphUtils.h"
#include "Graph/FlowAssetEditor.h"
#include "Graph/FlowAssetGraph.h"

#include "FlowAsset.h"

#include "Toolkits/ToolkitManager.h"

TSharedPtr<FFlowAssetEditor> FFlowGraphUtils::GetFlowAssetEditor(const UObject* ObjectToFocusOn)
{
	check(ObjectToFocusOn);

	TSharedPtr<FFlowAssetEditor> FlowAssetEditor;
	if (UFlowAsset* FlowAsset = Cast<const UFlowAssetGraph>(ObjectToFocusOn)->GetFlowAsset())
	{
		const TSharedPtr<IToolkit> FoundAssetEditor = FToolkitManager::Get().FindEditorForAsset(FlowAsset);
		if (FoundAssetEditor.IsValid())
		{
			FlowAssetEditor = StaticCastSharedPtr<FFlowAssetEditor>(FoundAssetEditor);
		}
	}
	return FlowAssetEditor;
}
