#include "FlowGraphUtils.h"
#include "FlowAssetEditor.h"
#include "FlowAssetGraph.h"

#include "Graph/FlowAsset.h"

#include "Toolkits/ToolkitManager.h"

TSharedPtr<FFlowAssetEditor> FFlowGraphUtils::GetFlowAssetEditor(const UObject* ObjectToFocusOn)
{
	check(ObjectToFocusOn);

	TSharedPtr<FFlowAssetEditor> FlowAssetEditor;
	if (UFlowAsset* FlowAsset = Cast<const UFlowAssetGraph>(ObjectToFocusOn)->GetFlowAsset())
	{
		TSharedPtr<IToolkit> FoundAssetEditor = FToolkitManager::Get().FindEditorForAsset(FlowAsset);
		if (FoundAssetEditor.IsValid())
		{
			FlowAssetEditor = StaticCastSharedPtr<FFlowAssetEditor>(FoundAssetEditor);
		}
	}
	return FlowAssetEditor;
}
