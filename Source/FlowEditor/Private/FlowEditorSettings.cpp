#include "FlowEditorSettings.h"

UFlowEditorSettings::UFlowEditorSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bShowDefaultPinNames(false)
	, NodeDescriptionBackground(FLinearColor(0.12f, 0.12f, 0.12f, 1.0f))
	, NodeStatusBackground(FLinearColor(0.12f, 0.12f, 0.12f, 1.0f))
	, NodePreloadedBackground(FLinearColor(0.12f, 0.12f, 0.12f, 1.0f))
	, RecentWireDuration(3.0f)
	, RecentWireColor(FLinearColor(1.0f, 0.05f, 0.0f, 1.0f))
	, RecentWireThickness(6.0f)
	, RecordedWireColor(FLinearColor(1.0f, 0.7f, 0.4f, 1.0f))
	, RecordedWireThickness(3.5f)
	, InactiveWireColor(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f))
	, InactiveWireThickness(1.5f)
{
	NodeTitleColors.Emplace(EFlowNodeStyle::Condition, FLinearColor(1.0f, 0.895f, 0.016f, 1.0f));
	NodeTitleColors.Emplace(EFlowNodeStyle::Default, FLinearColor(0.196f, 0.394f, 0.661f, 1.0f));
	NodeTitleColors.Emplace(EFlowNodeStyle::InOut, FLinearColor(0.468f,0.014f,0.040f,1.0f));
	NodeTitleColors.Emplace(EFlowNodeStyle::Latent, FLinearColor(0.398f, 0.0403f, 0.806f, 1.0f));
	NodeTitleColors.Emplace(EFlowNodeStyle::Logic, FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	NodeTitleColors.Emplace(EFlowNodeStyle::SubGraph, FLinearColor(0.588f,0.337f,0.058f,1.0f));
}
