#include "FlowEditorStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )

TSharedPtr<FSlateStyleSet> FFlowEditorStyle::StyleSet = nullptr;
TSharedPtr<ISlateStyle> FFlowEditorStyle::Get() { return StyleSet; }

FName FFlowEditorStyle::GetStyleSetName()
{
	static FName FlowEditorStyleName(TEXT("FlowEditorStyle"));
	return FlowEditorStyleName;
}

void FFlowEditorStyle::Initialize()
{
	const FVector2D Icon16 = FVector2D(16.0f, 16.0f);
	const FVector2D Icon64 = FVector2D(64.0f, 64.0f);

	StyleSet = MakeShareable(new FSlateStyleSet(TEXT("FlowEditorStyle")));
	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("Flow"))->GetContentDir());

	StyleSet->Set("ClassIcon.FlowAsset", new IMAGE_BRUSH(TEXT("Icons/FlowAsset_16x"), Icon16));
	StyleSet->Set("ClassThumbnail.FlowAsset", new IMAGE_BRUSH(TEXT("Icons/FlowAsset_64x"), Icon64));

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