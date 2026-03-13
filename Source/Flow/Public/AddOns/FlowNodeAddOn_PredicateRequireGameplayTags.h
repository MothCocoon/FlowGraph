// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "GameplayEffectTypes.h"

#include "AddOns/FlowNodeAddOn.h"
#include "Interfaces/FlowPredicateInterface.h"

#include "FlowNodeAddOn_PredicateRequireGameplayTags.generated.h"

UCLASS(MinimalApi, NotBlueprintable, meta = (DisplayName = "Require Gameplay Tags"))
class UFlowNodeAddOn_PredicateRequireGameplayTags
	: public UFlowNodeAddOn
	, public IFlowPredicateInterface
{
	GENERATED_BODY()

public:

	UFlowNodeAddOn_PredicateRequireGameplayTags();

	// IFlowPredicateInterface
	virtual bool EvaluatePredicate_Implementation() const override;
	// --

	// UFlowNodeBase
	virtual void UpdateNodeConfigText_Implementation() override;
	// --

#if WITH_EDITOR
	// UObject Interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// --

	// UFlowNodeBase
	void OnEditorPinConnectionsChanged(const TArray<FFlowPinConnectionChange>& Changes) override;
	// --
#endif

	bool TryGetTagsToCheckFromDataPin(FGameplayTagContainer& TagsToCheckValue) const;

public:

	/* DataPin input for the Gameplay Tag or Tag Container to test with the Requirements. */
	UPROPERTY(EditAnywhere, Category = Configuration, meta = (DefaultForInputFlowPin, FlowPinType = "GameplayTagContainer"))
	FGameplayTagContainer Tags;

	/* Requirements to evaluate the Test Tags with. */
	UPROPERTY(EditAnywhere, Category = Configuration)
	FGameplayTagRequirements Requirements;
};
