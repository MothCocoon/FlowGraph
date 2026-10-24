// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Graph/FlowGraphEditorSettings.h"

#include "Graph/FlowGraphSchema.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowGraphEditorSettings)

UFlowGraphEditorSettings::FOnSettingsChanged UFlowGraphEditorSettings::SettingsChanged;

void UFlowGraphEditorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UFlowGraphEditorSettings, bShowNodeClass))
	{
		GetDefault<UFlowGraphSchema>()->ForceVisualizationCacheClear();
	}

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UFlowGraphEditorSettings, bShowAddOnsInNodeDetails))
	{
		SettingsChanged.Broadcast();
	}
}

UFlowGraphEditorSettings::FOnSettingsChanged& UFlowGraphEditorSettings::OnSettingsChanged()
{
	return SettingsChanged;
}
