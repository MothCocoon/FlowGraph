// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraphSettings.h"

#include "FlowAsset.h"

#define LOCTEXT_NAMESPACE "FlowGraphSettings"

UFlowGraphSettings::UFlowGraphSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bExposeFlowAssetCreation(true)
	, bExposeFlowNodeCreation(true)
	, bShowAssetToolbarAboveLevelEditor(true)
	, FlowAssetCategoryName(LOCTEXT("FlowAssetCategory", "Flow"))
	, WorldAssetClass(UFlowAsset::StaticClass())
	, bShowDefaultPinNames(false)
	, ExecPinColorModifier(0.75f, 0.75f, 0.75f, 1.0f)
	, NodeDescriptionBackground(FLinearColor(0.0625f, 0.0625f, 0.0625f, 1.0f))
	, NodeStatusBackground(FLinearColor(0.12f, 0.12f, 0.12f, 1.0f))
	, NodePreloadedBackground(FLinearColor(0.12f, 0.12f, 0.12f, 1.0f))
	, ConnectionDrawType(EFlowConnectionDrawType::Default)
	, CircuitConnectionAngle(45.f)
	, CircuitConnectionSpacing(FVector2D(30.f))
	, InactiveWireColor(FLinearColor(0.364f, 0.364f, 0.364f, 1.0f))
	, InactiveWireThickness(1.5f)
	, RecentWireDuration(3.0f)
	, RecentWireColor(FLinearColor(1.0f, 0.05f, 0.0f, 1.0f))
	, RecentWireThickness(6.0f)
	, RecordedWireColor(FLinearColor(0.432f, 0.258f, 0.096f, 1.0f))
	, RecordedWireThickness(3.5f)
	, SelectedWireColor(FLinearColor(0.984f, 0.482f, 0.010f, 1.0f))
	, SelectedWireThickness(1.5f)
{
	NodeTitleColors.Emplace(EFlowNodeStyle::Condition, FLinearColor(1.0f, 0.62f, 0.016f, 1.0f));
	NodeTitleColors.Emplace(EFlowNodeStyle::Default, FLinearColor(-0.728f, 0.581f, 1.0f, 1.0f));
	NodeTitleColors.Emplace(EFlowNodeStyle::InOut, FLinearColor(1.0f, 0.0f, 0.008f, 1.0f));
	NodeTitleColors.Emplace(EFlowNodeStyle::Latent, FLinearColor(0.0f, 0.770f, 0.375f, 1.0f));
	NodeTitleColors.Emplace(EFlowNodeStyle::Logic, FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	NodeTitleColors.Emplace(EFlowNodeStyle::SubGraph, FLinearColor(1.0f, 0.128f, 0.0f, 1.0f));
}

#undef LOCTEXT_NAMESPACE
