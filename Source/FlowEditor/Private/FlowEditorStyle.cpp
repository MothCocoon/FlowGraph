#include "FlowEditorStyle.h"
#include "FlowEditorSettings.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )

TSharedPtr<FSlateStyleSet> FFlowEditorStyle::StyleSet = nullptr;

FName FFlowEditorStyle::GetStyleSetName()
{
	static FName FlowEditorStyleName(TEXT("FlowEditorStyle"));
	return FlowEditorStyleName;
}

void FFlowEditorStyle::Initialize()
{
	StyleSet = MakeShareable(new FSlateStyleSet(TEXT("FlowEditorStyle")));

	const FVector2D Icon16(16.0f, 16.0f);
	const FVector2D Icon20(20.0f, 20.0f);
	const FVector2D Icon30(30.0f, 30.0f);
	const FVector2D Icon40(40.0f, 40.0f);
	const FVector2D Icon64(64.0f, 64.0f);

	// engine assets
	StyleSet->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate/"));

	StyleSet->Set("FlowGraph.PausePlaySession", new IMAGE_BRUSH("Icons/icon_pause_40x", Icon40));
	StyleSet->Set("FlowGraph.PausePlaySession.Small", new IMAGE_BRUSH("Icons/icon_pause_40x", Icon20));
	StyleSet->Set("FlowGraph.ResumePlaySession", new IMAGE_BRUSH("Icons/icon_simulate_40x", Icon40));
	StyleSet->Set("FlowGraph.ResumePlaySession.Small", new IMAGE_BRUSH("Icons/icon_simulate_40x", Icon20));
	StyleSet->Set("FlowGraph.StopPlaySession", new IMAGE_BRUSH("Icons/icon_stop_40x", Icon40));
	StyleSet->Set("FlowGraph.StopPlaySession.Small", new IMAGE_BRUSH("Icons/icon_stop_40x", Icon20));

	StyleSet->Set("FlowGraph.StepOut", new IMAGE_BRUSH("Icons/icon_DebugStepOut_40x", Icon40));

	StyleSet->Set("FlowGraph.BreakpointEnabled", new IMAGE_BRUSH("Old/Kismet2/Breakpoint_Valid", FVector2D(24.0f, 24.0f)));
	StyleSet->Set("FlowGraph.BreakpointDisabled", new IMAGE_BRUSH("Old/Kismet2/Breakpoint_Disabled", FVector2D(24.0f, 24.0f)));
	StyleSet->Set("FlowGraph.BreakpointHit", new IMAGE_BRUSH("Old/Kismet2/IP_Breakpoint", Icon40));
	StyleSet->Set("FlowGraph.PinBreakpointHit", new IMAGE_BRUSH("Old/Kismet2/IP_Breakpoint", Icon30));

	// Flow assets
	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("Flow"))->GetContentDir());

	StyleSet->Set("ClassIcon.FlowAsset", new IMAGE_BRUSH(TEXT("Icons/FlowAsset_16x"), Icon16));
	StyleSet->Set("ClassThumbnail.FlowAsset", new IMAGE_BRUSH(TEXT("Icons/FlowAsset_64x"), Icon64));

	StyleSet->Set("FlowGraph.ActiveShadow", new BOX_BRUSH("Icons/FlowNode_Shadow", FMargin(18.0f / 64.0f)));
	StyleSet->Set("FlowGraph.WasActiveShadow", new BOX_BRUSH("Icons/FlowNode_Shadow", FMargin(18.0f / 64.0f)));

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
};

void FFlowEditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
	ensure(StyleSet.IsUnique());
	StyleSet.Reset();
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH