// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "FlowNodeAddOnBlueprint.generated.h"

/**
 * Flow Node AddOn Blueprint class
 */
UCLASS(BlueprintType)
class FLOW_API UFlowNodeAddOnBlueprint : public UBlueprint
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	// UBlueprint
	virtual bool SupportedByDefaultBlueprintFactory() const override { return false; }

	virtual bool SupportsDelegates() const override { return false; }
	// --
#endif
};
