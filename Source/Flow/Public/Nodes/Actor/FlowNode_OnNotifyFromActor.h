// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/Actor/FlowNode_ComponentObserver.h"
#include "FlowNode_OnNotifyFromActor.generated.h"

/**
 * Triggers output when Flow Component with matching Identity Tag calls NotifyGraph function with matching Notify Tag
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "On Notify From Actor"))
class FLOW_API UFlowNode_OnNotifyFromActor : public UFlowNode_ComponentObserver
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(EditAnywhere, Category = "Notify")
	FGameplayTagContainer NotifyTags;

	// If true, node will check given Notify Tag is present in the Recently Sent Notify Tags
	// This might be helpful in multiplayer, if client-side Flow Node started work after server sent the notify
	UPROPERTY(EditAnywhere, Category = "Notify")
	bool bRetroactive;

	virtual void ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component) override;
	virtual void ForgetActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component) override;

	virtual void OnNotifyFromComponent(UFlowComponent* Component, const FGameplayTag& Tag);
	
#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
#endif
};
