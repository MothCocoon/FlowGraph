// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowEditorStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define IMAGE_BRUSH_SVG( RelativePath, ... ) FSlateVectorImageBrush(StyleSet->RootToContentDir(RelativePath, TEXT(".svg")), __VA_ARGS__)

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

	StyleSet->Set("FlowToolbar.RefreshAsset", new IMAGE_BRUSH_SVG( "Starship/Common/Apply", Icon20));
	StyleSet->Set("FlowToolbar.ValidateAsset", new IMAGE_BRUSH_SVG( "Starship/Common/Debug", Icon20));
	
	StyleSet->Set("FlowToolbar.SearchInAsset", new IMAGE_BRUSH_SVG( "Starship/Common/Search", Icon20));
	StyleSet->Set("FlowToolbar.GoToParentInstance", new IMAGE_BRUSH("Icons/icon_DebugStepOut_40x", Icon40));

	StyleSet->Set("FlowGraph.BreakpointEnabled", new IMAGE_BRUSH("Old/Kismet2/Breakpoint_Valid", FVector2D(24.0f, 24.0f)));
	StyleSet->Set("FlowGraph.BreakpointDisabled", new IMAGE_BRUSH("Old/Kismet2/Breakpoint_Disabled", FVector2D(24.0f, 24.0f)));
	StyleSet->Set("FlowGraph.BreakpointHit", new IMAGE_BRUSH("Old/Kismet2/IP_Breakpoint", Icon40));
	StyleSet->Set("FlowGraph.PinBreakpointHit", new IMAGE_BRUSH("Old/Kismet2/IP_Breakpoint", Icon30));

	StyleSet->Set("GraphEditor.Sequence_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_Sequence_16x", Icon16));

	// Flow assets
	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("Flow"))->GetBaseDir() / TEXT("Resources"));

	StyleSet->Set("ClassIcon.FlowAsset", new IMAGE_BRUSH(TEXT("Icons/FlowAsset_16x"), Icon16));
	StyleSet->Set("ClassThumbnail.FlowAsset", new IMAGE_BRUSH(TEXT("Icons/FlowAsset_64x"), Icon64));

	StyleSet->Set("Flow.Node.Title", new BOX_BRUSH("Icons/FlowNode_Title", FMargin(8.0f/64.0f, 0, 0, 0)));
	StyleSet->Set("Flow.Node.Body", new BOX_BRUSH("Icons/FlowNode_Body", FMargin(16.f/64.f)));
	StyleSet->Set("Flow.Node.ActiveShadow", new BOX_BRUSH("Icons/FlowNode_Shadow_Active", FMargin(18.0f/64.0f)));
	StyleSet->Set("Flow.Node.WasActiveShadow", new BOX_BRUSH("Icons/FlowNode_Shadow_WasActive", FMargin(18.0f/64.0f)));

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
