// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"
#include "FlowNode_Property.generated.h"

/**
 * 
 */
UCLASS(NotBlueprintable, NotPlaceable, meta = (DisplayName = "Property"))
class FLOW_API UFlowNode_Property : public UFlowNode
{
	GENERATED_UCLASS_BODY()
	void SetProperty(FProperty* InProperty);

protected:
	virtual UObject* GetVariableHolder() override;
	virtual uint8* GetVariableContainer() override;
	virtual void PostLoad() override;

	virtual const TArray<FFlowPropertyPin> GetOutputProperties() override;
	virtual const TArray<FFlowPropertyPin> GetInputProperties() override;

	UPROPERTY(SaveGame)
	FName PropertyName;

	FProperty* Property;

	void LoadProperty();
};
