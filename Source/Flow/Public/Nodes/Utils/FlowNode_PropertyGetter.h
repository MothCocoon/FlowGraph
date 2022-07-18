// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "CoreMinimal.h"
#include "FlowNode_Property.h"
#include "FlowNode_PropertyGetter.generated.h"

/**
 * 
 */
UCLASS(NotBlueprintable, NotPlaceable, meta = (DisplayName = "Get"))
class FLOW_API UFlowNode_PropertyGetter final : public UFlowNode_Property
{
	GENERATED_UCLASS_BODY()

protected:
	virtual const TMultiMap<TWeakObjectPtr<UObject>, FFlowInputOutputPin> GetOutputProperties() override;
};
