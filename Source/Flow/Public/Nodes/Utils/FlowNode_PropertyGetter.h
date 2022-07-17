// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"
#include "FlowNode_PropertyGetter.generated.h"

/**
 * 
 */
UCLASS(NotBlueprintable, NotPlaceable, meta = (DisplayName = "Property"))
class FLOW_API UFlowNode_PropertyGetter final : public UFlowNode
{
	GENERATED_UCLASS_BODY()
	void SetProperty(FProperty* InProperty);

protected:
#if WITH_EDITOR
	virtual FText GetNodeTitle() const override;

	virtual void PostLoad() override;
#endif

	virtual const TMultiMap<TWeakObjectPtr<UObject>, FFlowInputOutputPin> GetOutputProperties() override;
	virtual const TMultiMap<TWeakObjectPtr<UObject>, FFlowInputOutputPin> GetInputProperties() override;
	virtual UObject* GetVariableHolder() override;
	
private:
	UPROPERTY(SaveGame)
	FName PropertyName;

	FProperty* Property;

	void LoadProperty();
};
