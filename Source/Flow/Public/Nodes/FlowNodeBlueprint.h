// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Engine/Blueprint.h"
#include "FlowNodeBlueprint.generated.h"

/**
 * Flow Node Blueprint class
 */
UCLASS(BlueprintType)
class FLOW_API UFlowNodeBlueprint : public UBlueprint
{
	GENERATED_BODY()

public:
	UFlowNodeBlueprint() = default;

#if WITH_EDITOR
	virtual bool SupportedByDefaultBlueprintFactory() const override { return false; }
	virtual bool SupportsDelegates() const override { return false; }
#endif
};
