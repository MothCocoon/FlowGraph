// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraphEditorSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowGraphEditorSettings)

UFlowGraphEditorSettings::UFlowGraphEditorSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, NodeDoubleClickTarget(EFlowNodeDoubleClickTarget::PrimaryAssetOrNodeDefinition)
	, bShowNodeClass(false)
	, bShowNodeDescriptionWhilePlaying(true)
	, bEnforceFriendlyPinNames(false)
	, bShowSubGraphPreview(true)
	, bShowSubGraphPath(true)
	, SubGraphPreviewSize(FVector2D(640.f, 360.f))
	, bHotReloadNativeNodes(false)
	, bHighlightInputWiresOfSelectedNodes(false)
	, bHighlightOutputWiresOfSelectedNodes(false)
{
}
