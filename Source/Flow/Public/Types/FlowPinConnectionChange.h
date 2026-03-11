// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "UObject/Object.h"
#include "FlowPinConnectionChange.generated.h"

class UFlowNode;

/**
 * Editor-only representation of a change to a node pin's connection.
 * PinName is the *final* pin name visible on the node (after any disambiguation / mangling).
 */
USTRUCT(BlueprintType)
struct FLOW_API FFlowPinConnectionChange
{
	GENERATED_BODY()

public:

	FFlowPinConnectionChange() = default;
	explicit FFlowPinConnectionChange(
		const FName& ChangedPinName,
		UFlowNode* InOldConnectedNode,
		const FName& InOldConnectedPinName,
		UFlowNode* InNewConnectedNode,
		const FName& InNewConnectedPinName)
		: PinName(ChangedPinName)
		, OldConnectedNode(InOldConnectedNode)
		, OldConnectedPinName(InOldConnectedPinName)
		, NewConnectedNode(InNewConnectedNode)
		, NewConnectedPinName(InNewConnectedPinName)
		{}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Flow")
	FName PinName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Flow")
	TObjectPtr<UFlowNode> OldConnectedNode = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flow")
	FName OldConnectedPinName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flow")
	TObjectPtr<UFlowNode> NewConnectedNode = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flow")
	FName NewConnectedPinName = NAME_None;
};
