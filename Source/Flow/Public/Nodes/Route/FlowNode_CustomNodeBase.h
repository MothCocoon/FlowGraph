// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/FlowNode.h"

#include "FlowNode_CustomNodeBase.generated.h"

/**
 * Base-class for CustomInput and CustomOutput node types
 */
UCLASS(Abstract, NotBlueprintable)
class FLOW_API UFlowNode_CustomNodeBase : public UFlowNode
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY()
	FName EventName;

public:
	void SetEventName(const FName& InEventName);
	const FName& GetEventName() const { return EventName; }

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
	virtual EDataValidationResult ValidateNode() override;
#endif
};
