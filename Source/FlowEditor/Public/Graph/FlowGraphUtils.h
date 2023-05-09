// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"

class FFlowAssetEditor;
class SFlowGraphEditor;
class UEdGraph;

class FLOWEDITOR_API FFlowGraphUtils
{
public:
	FFlowGraphUtils() {}

	static TSharedPtr<FFlowAssetEditor> GetFlowAssetEditor(const UEdGraph* Graph);
	static TSharedPtr<SFlowGraphEditor> GetFlowGraphEditor(const UEdGraph* Graph);
};
