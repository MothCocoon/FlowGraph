// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"

class UFlowAsset;
class FFlowAssetEditor;
class SFlowGraphEditor;
class UEdGraph;

class FLOWEDITOR_API FFlowGraphUtils
{
public:
	FFlowGraphUtils() {}

	static TSharedPtr<FFlowAssetEditor> GetFlowAssetEditor(const UEdGraph* Graph);
	static TSharedPtr<FFlowAssetEditor> GetFlowAssetEditor(const UFlowAsset* FlowAsset);
	static TSharedPtr<SFlowGraphEditor> GetFlowGraphEditor(const UEdGraph* Graph);

	static FString RemovePrefixFromNodeText(const FText& Source);
};
