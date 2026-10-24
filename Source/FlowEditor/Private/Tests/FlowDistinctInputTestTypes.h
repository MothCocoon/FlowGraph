// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "AddOns/FlowNodeAddOn.h"
#include "Nodes/FlowNode.h"
#include "Types/FlowDataPinValuesStandard.h"

#include "FlowDistinctInputTestTypes.generated.h"

UCLASS(Hidden, NotBlueprintable, NotPlaceable, EditInlineNew, meta = (ExcludeFromFlowCatalog))
class UFlowDistinctInputTestAddOn : public UFlowNodeAddOn
{
	GENERATED_BODY()

public:
	UFlowDistinctInputTestAddOn()
	{
#if WITH_EDITORONLY_DATA
		Value.bIsInputPin = true;
#endif
	}
	UPROPERTY()
	FFlowDataPinValue_Int Value;

	virtual bool AllowMergeInputPin(const FFlowPin&) const override { return false; }
};

UCLASS(Hidden, NotBlueprintable, NotPlaceable, meta = (ExcludeFromFlowCatalog))
class UFlowDistinctInputTestNode : public UFlowNode
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FFlowDataPinValue_Int Value;

	UPROPERTY()
	int32 Role = 0;
};
