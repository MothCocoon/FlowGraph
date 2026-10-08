// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "GameplayTagContainer.h"

#include "Nodes/FlowNode.h"
#include "FlowNode_NotifyActor.generated.h"

/**
 * Finds all Flow Components with matching Identity Tag and calls ReceiveNotify event on these components.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Notify Actor", Keywords = "event"))
class FLOW_API UFlowNode_NotifyActor : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowNode_NotifyActor();

protected:
	UPROPERTY(EditAnywhere, Category = "TargetComponent")
	FGameplayTagContainer IdentityTags;

	UPROPERTY(EditAnywhere, Category = "TargetComponent")
	EGameplayContainerMatchType MatchType = EGameplayContainerMatchType::All;

	/* If true, Flow Component must have exact Identity Tags.
	 * If false, a child of any Identity Tag also matches. */
	UPROPERTY(EditAnywhere, Category = "TargetComponent")
	bool bExactMatch = true;
	
	UPROPERTY(EditAnywhere, Category = "Notify")
	FGameplayTagContainer NotifyTags;

	UPROPERTY(EditAnywhere, Category = "Notify")
	EFlowNetMode NetMode = EFlowNetMode::Authority;

public:	
	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
	virtual FString GetNodeDescription() const override;
	virtual const FFlowAgentDoc& GetAgentDoc() const override;
	
protected:	
	virtual EDataValidationResult ValidateNode() override;
#endif
};
