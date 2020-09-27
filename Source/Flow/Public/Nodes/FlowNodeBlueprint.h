#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "FlowNodeBlueprint.generated.h"

/**
 * A specialized blueprint class required for customizing Asset Type Actions
 */

UCLASS(BlueprintType)
class FLOW_API UFlowNodeBlueprint final : public UBlueprint
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	// UBlueprint
	virtual bool SupportedByDefaultBlueprintFactory() const override { return false; }
	// --

	/** Returns the most base Flow Node blueprint for a given blueprint (if it is inherited from another Flow Node blueprint, returning null if only native / non-ability BP classes are it's parent) */
	static UFlowNodeBlueprint* FindRootFlowNodeBlueprint(UFlowNodeBlueprint* DerivedBlueprint);
#endif
};
