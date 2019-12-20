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
	
//////////////////////////////////////////////////////////////////////////
// Flow node
	
private:
	UPROPERTY(Instanced)
	UFlowNode* FlowNode;

public:
	void SetFlowNode(UFlowNode* InFlowNode);
	UFlowNode* GetFlowNode() const;

	virtual void PostLoad() override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	virtual void PostEditImport() override;
	virtual void PrepareForCopying() override;
	void PostCopyNode();

private:
	/** Make sure the FlowNode is owned by the FlowAsset */
	void ResetFlowNodeOwner();

//////////////////////////////////////////////////////////////////////////
// Graph node

public:
	// UEdGraphNode
	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
	// --

	/**
	 * Handles inserting the node between the FromPin and what the FromPin was original connected to
	 *
	 * @param FromPin			The pin this node is being spawned from
	 * @param NewLinkPin		The new pin the FromPin will connect to
	 * @param OutNodeList		Any nodes that are modified will get added to this list for notification purposes
	 */
	void InsertNewNode(UEdGraphPin* FromPin, UEdGraphPin* NewLinkPin, TSet<UEdGraphNode*>& OutNodeList);

	// UEdGraphNode
	virtual void ReconstructNode() override;
	virtual void AllocateDefaultPins() override;

	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual FText GetTooltipText() const override;
	// --

//////////////////////////////////////////////////////////////////////////
// Utils

	// short summary of node's content
	FString GetShortDescription() const;
	
	// information displayed while node is working
	FString GetStatus() const;

	UObject* GetAssetToOpen() const;
	bool CanFocusViewport() const;

//////////////////////////////////////////////////////////////////////////
// Pins

	TArray<UEdGraphPin*> InputPins;
	TArray<UEdGraphPin*> OutputPins;

	void CreateInputPin(const FName& PinName);
	void CreateOutputPin(const FName PinName);

	bool CanUserAddInput() const;
	bool CanUserAddOutput() const;

	void AddUserInput();
	void AddUserOutput();

	void RemoveUserInput(UEdGraphPin* InPin);
	void RemoveUserOutput(UEdGraphPin* InPin);

	UEdGraphPin* GetInputPin(const uint8 Index) const;
	UEdGraphPin* GetOutputPin(const uint8 Index) const;
};
