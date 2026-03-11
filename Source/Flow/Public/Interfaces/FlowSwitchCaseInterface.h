// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "UObject/Interface.h"
#include "Templates/SubclassOf.h"

#include "FlowSwitchCaseInterface.generated.h"

class UFlowNodeAddOn;

/**
 * 'Case' AddOn for the Switch node.
 */
UINTERFACE(MinimalAPI, BlueprintType, Blueprintable, DisplayName = "Flow Switch Case Interface")
class UFlowSwitchCaseInterface : public UInterface
{
	GENERATED_BODY()
};

class FLOW_API IFlowSwitchCaseInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent)
	bool TryTriggerForCase() const;
	virtual bool TryTriggerForCase_Implementation() const { return true; }

	static bool ImplementsInterfaceSafe(const UFlowNodeAddOn* AddOnTemplate);
};
