// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Engine/Blueprint.h"
#include "FlowNodeAddOnBlueprint.generated.h"

/**
 * Flow Node AddOn Blueprint class
 */
UCLASS(BlueprintType)
class FLOW_API UFlowNodeAddOnBlueprint : public UBlueprint
{
	GENERATED_BODY()

public:
	UFlowNodeAddOnBlueprint() = default;

#if WITH_EDITOR
	virtual bool SupportedByDefaultBlueprintFactory() const override { return false; }
	virtual bool SupportsDelegates() const override { return false; }
#endif
};
