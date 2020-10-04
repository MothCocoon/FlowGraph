#pragma once

#include "EdGraph/EdGraph.h"
#include "FlowGraph.generated.h"

class UFlowNode;
class UFlowAsset;

UCLASS()
class FLOWEDITOR_API UFlowGraph : public UEdGraph
{
	GENERATED_UCLASS_BODY()

	static UEdGraph* CreateGraph(UFlowAsset* InFlowAsset);

	// UEdGraph
	virtual void NotifyGraphChanged() override;
	// --

	/** Returns the FlowAsset that contains this graph */
	UFlowAsset* GetFlowAsset() const;
};
