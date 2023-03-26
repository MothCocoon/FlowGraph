// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "CoreMinimal.h"

class FFlowAssetEditor;
class SFlowGraphEditor;

class FLOWEDITOR_API FFlowGraphUtils
{
public:
	FFlowGraphUtils() {}

	static TSharedPtr<FFlowAssetEditor> GetFlowAssetEditor(const UEdGraph* Graph);
	static TSharedPtr<SFlowGraphEditor> GetFlowGraphEditor(const UEdGraph* Graph);
};
