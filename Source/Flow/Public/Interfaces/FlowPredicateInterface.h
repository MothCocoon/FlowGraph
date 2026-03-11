// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "UObject/Interface.h"
#include "Templates/SubclassOf.h"

#include "FlowPredicateInterface.generated.h"

class UFlowNodeAddOn;

/**
 * Predicate interface for AddOns.
 */
UINTERFACE(MinimalAPI, BlueprintType, Blueprintable, DisplayName = "Flow Predicate Interface")
class UFlowPredicateInterface : public UInterface
{
	GENERATED_BODY()
};

class FLOW_API IFlowPredicateInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
	bool EvaluatePredicate() const;
	virtual bool EvaluatePredicate_Implementation() const { return true; }

	static bool ImplementsInterfaceSafe(const UFlowNodeAddOn* AddOnTemplate);
};
