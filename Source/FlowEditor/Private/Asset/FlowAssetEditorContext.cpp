// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Asset/FlowAssetEditorContext.h"

#include "Asset/FlowAssetEditor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowAssetEditorContext)

UFlowAsset* UFlowAssetEditorContext::GetFlowAsset() const
{
	const TSharedPtr<FFlowAssetEditor> FlowAssetEditorPtr = FlowAssetEditor.Pin();
	return FlowAssetEditorPtr ? FlowAssetEditorPtr->GetFlowAsset() : nullptr;
}
