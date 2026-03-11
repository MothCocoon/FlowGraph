// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/Graph/FlowNode_DefineProperties.h"
#include "FlowNode_FormatText.generated.h"

/**
 * Formats a text string using the standard UE FText formatting system.
 * using input pins as parameters and the output is delivered to OUTPIN_TextOutput.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Format Text", Keywords = "print"))
class FLOW_API UFlowNode_FormatText : public UFlowNode_DefineProperties
{
	GENERATED_BODY()

public:
	UFlowNode_FormatText();

private:
	/* Format text string.
	 * Uses standard Unreal "FText" formatting: eg, {PinName} will refer to input called PinName.
	 * Note: complex types are exported "ToString" and InstancedStruct is not supported. */
	UPROPERTY(EditAnywhere, Category = "Flow", meta = (DefaultForInputFlowPin, FlowPinType = Text))
	FText FormatText;

#if WITH_EDITOR
public:
	// UObject
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	// --

	virtual void UpdateNodeConfigText_Implementation() override;
#endif

protected:
	EFlowDataPinResolveResult TryResolveFormatText(const FName& PinName, FText& OutFormattedText) const;

public:
	// IFlowDataPinValueSupplierInterface
	virtual FFlowDataPinResult TrySupplyDataPin(FName PinName) const override;
	// --

	static const FName OUTPIN_TextOutput;
};
