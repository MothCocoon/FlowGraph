// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "CoreMinimal.h"
#include "FlowNode_Property.h"
#include "FlowNode_PropertySetter.generated.h"

/**
 * 
 */
UCLASS(NotBlueprintable, NotPlaceable, meta = (DisplayName = "Set"))
class FLOW_API UFlowNode_PropertySetter : public UFlowNode_Property
{
	GENERATED_UCLASS_BODY()

	virtual void ExecuteInput(const FName& PinName) override;

	virtual const TMultiMap<TWeakObjectPtr<UObject>, FFlowInputOutputPin> GetInputProperties() override;
};
