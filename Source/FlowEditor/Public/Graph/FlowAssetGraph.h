#pragma once

#include "EdGraph/EdGraph.h"
#include "FlowAssetGraph.generated.h"

class UFlowAsset;

UCLASS(MinimalAPI)
class UFlowAssetGraph : public UEdGraph
{
	GENERATED_UCLASS_BODY()

	// UEdGraph
	virtual void NotifyGraphChanged() override;
	// --
	
	/** Returns the FlowAsset that contains this graph */
	UFlowAsset* GetFlowAsset() const;
};
