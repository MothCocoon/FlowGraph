// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_LogicalAND.generated.h"

/**
 * Logical AND
 * Output will be triggered only once
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "AND", Keywords = "&"))
class FLOW_API UFlowNode_LogicalAND final : public UFlowNode
{
	GENERATED_UCLASS_BODY()

private:
	UPROPERTY(SaveGame)
	TSet<FName> ExecutedInputNames;
	
#if WITH_EDITOR
public:
	virtual bool CanUserAddInput() const override { return true; }
#endif

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;
};
