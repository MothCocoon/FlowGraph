#include "Graph/FlowGraphEditorSettings.h"

#include "FlowAsset.h"

UFlowGraphEditorSettings::UFlowGraphEditorSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bShowNodeClass(false)
	, bShowSubGraphPreview(true)
	, bShowSubGraphPath(true)
	, SubGraphPreviewSize(FVector2D(640.f, 360.f))
	, bHighlightInputWiresOfSelectedNodes(true)
	, bHighlightOutputWiresOfSelectedNodes(false)
{
}
