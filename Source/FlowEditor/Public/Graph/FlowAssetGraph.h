#pragma once

#include "EdGraph/EdGraph.h"
#include "FlowAssetGraph.generated.h"

class UFlowNode;
class UFlowAsset;

UCLASS(MinimalAPI)
class UFlowAssetGraph : public UEdGraph
{
	GENERATED_UCLASS_BODY()

	static UEdGraph* CreateGraph(UFlowAsset* InFlowAsset);
	
	// UEdGraph
	virtual void NotifyGraphChanged() override;
	// --
	
	/** Returns the FlowAsset that contains this graph */
	UFlowAsset* GetFlowAsset() const;
};
