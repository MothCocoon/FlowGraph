#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "UObject/ObjectMacros.h"
#include "FlowGraphNode.generated.h"

class UEdGraphPin;
class UEdGraphSchema;

class UFlowNode;

UCLASS()
class UFlowGraphNode : public UEdGraphNode
{
	GENERATED_UCLASS_BODY()

private:
	TArray<UEdGraphPin*> InputPins;
	TArray<UEdGraphPin*> OutputPins;
	
private:
	UPROPERTY(Instanced)
	UFlowNode* FlowNode;

public:
	void SetFlowNode(UFlowNode* InFlowNode);
	UFlowNode* GetFlowNode() const;

	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;

	virtual FString GetDesc() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;

	virtual void CreateInputPins();
	void CreateInputPin(const FName PinName = TEXT(""));
	bool CanAddInputPin() const { return false; };
	void AddInputPin();
	void RemoveInputPin(UEdGraphPin* InGraphPin);

	UEdGraphPin* GetInputPin(const uint8 Index) const;
	void GetInputPins(TArray<UEdGraphPin*>& OutPins) const;
	uint8 GetInputCount() const;
	uint8 GetInputPinIndex(const UEdGraphPin* Pin) const;

	virtual void CreateOutputPins();
	void CreateOutputPin(const FName PinName = TEXT(""));
	bool CanAddOutputPin() const { return false; };
	void AddOutputPin();
	void RemoveOutputPin(UEdGraphPin* InGraphPin);

	UEdGraphPin* GetOutputPin(const uint8 Index) const;
	void GetOutputPins(TArray<UEdGraphPin*>& OutPins) const;
	uint8 GetOutputCount() const;

	bool CanFocusViewport() const;
	int32 EstimateNodeWidth() const;

	/**
	 * Handles inserting the node between the FromPin and what the FromPin was original connected to
	 *
	 * @param FromPin			The pin this node is being spawned from
	 * @param NewLinkPin		The new pin the FromPin will connect to
	 * @param OutNodeList		Any nodes that are modified will get added to this list for notification purposes
	 */
	void InsertNewNode(UEdGraphPin* FromPin, UEdGraphPin* NewLinkPin, TSet<UEdGraphNode*>& OutNodeList);

	// UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual void ReconstructNode() override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const override;
	virtual UObject* GetAssetToOpen() const;
	// End of UEdGraphNode interface

	virtual void PostLoad() override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	virtual void PostEditImport() override;
	virtual void PrepareForCopying() override;
	void PostCopyNode();

private:
	/** Make sure the FlowNode is owned by the FlowAsset */
	void ResetFlowNodeOwner();
};
