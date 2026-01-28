// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraphEditorSettings.h"
#include "Graph/FlowGraphSchema.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowGraphEditorSettings)

UFlowGraphEditorSettings::UFlowGraphEditorSettings()
	: NodeDoubleClickTarget(EFlowNodeDoubleClickTarget::PrimaryAssetOrNodeDefinition)
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

#if WITH_EDITOR
void UFlowGraphEditorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UFlowGraphEditorSettings, bShowNodeClass))
	{
		GetDefault<UFlowGraphSchema>()->ForceVisualizationCacheClear();
	}
}
#endif
