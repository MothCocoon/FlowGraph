#pragma once

#include "Styling/SlateStyle.h"

class FFlowEditorStyle
{
public:
	static void Initialize();
	static void Shutdown();

	static TSharedPtr<ISlateStyle> Get();
	static FName GetStyleSetName();

private:
	static TSharedPtr<FSlateStyleSet> StyleSet;
};