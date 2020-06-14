#pragma once

#include "CoreMinimal.h"

class FFlowAssetEditor;

class FLOWEDITOR_API FFlowGraphUtils
{
public:
	FFlowGraphUtils() {}

	static TSharedPtr<FFlowAssetEditor> GetFlowAssetEditor(const UObject* ObjectToFocusOn);
};
