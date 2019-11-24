#include "FlowEditorSettings.h"

UFlowEditorSettings::UFlowEditorSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, NodeDescriptionBackground(FLinearColor(0.12f, 0.12f, 0.12f, 1.0f))
	, RecentWireDuration(3.0f)
	, RecentWireColor(FLinearColor(1.0f, 0.05f, 0.0f, 1.0f))
	, RecentWireThickness(6.0f)
	, RecordedWireColor(FLinearColor(1.0f, 0.7f, 0.4f, 1.0f))
	, RecordedWireThickness(3.5f)
	, InactiveWireColor(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f))
	, InactiveWireThickness(1.5f)
{
}
