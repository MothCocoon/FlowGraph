// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowGraphNode_Property.h"
#include "FlowGraphNode_PropertySetter.generated.h"

UCLASS()
class FLOWEDITOR_API UFlowGraphNode_PropertySetter : public UFlowGraphNode_Property
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	// --
};
