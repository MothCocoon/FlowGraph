#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "UObject/ObjectMacros.h"
#include "FlowAssetGraph.generated.h"

class UFlowAsset;

UCLASS(MinimalAPI)
class UFlowAssetGraph : public UEdGraph
{
	GENERATED_UCLASS_BODY()

public:
	/** Returns the FlowAsset that contains this graph */
	UFlowAsset* GetFlowAsset() const;
};