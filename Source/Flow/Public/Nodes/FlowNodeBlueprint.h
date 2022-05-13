// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "FlowNodeBlueprint.generated.h"

/**
 * A specialized blueprint class required for customizing Asset Type Actions
 */

UCLASS(BlueprintType)
class FLOW_API UFlowNodeBlueprint : public UBlueprint
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	// UBlueprint
	virtual bool SupportedByDefaultBlueprintFactory() const override { return false; }

	virtual bool SupportsDelegates() const override { return false; }
	virtual bool SupportsEventGraphs() const override { return false; }
	virtual bool SupportsAnimLayers() const override { return false; }
	// --
#endif
};
