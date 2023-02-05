// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowAssetEditorContext.h"
#include "Asset/FlowAssetEditor.h"

UFlowAsset* UFlowAssetEditorContext::GetFlowAsset() const
{
	return FlowAssetEditor.IsValid() ? FlowAssetEditor.Pin()->GetFlowAsset() : nullptr;
}
