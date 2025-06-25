// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/Graph/FlowNode_DefineProperties.h"

#include "FlowNode_FormatText.generated.h"

/**
 * Formats a text string using the standard UE FText formatting system
 * using input pins as parameters and the output is delivered to OUTPIN_TextOutput
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Format Text", Keywords = "print"))
class FLOW_API UFlowNode_FormatText : public UFlowNode_DefineProperties
{
	GENERATED_UCLASS_BODY()
	
private:
	// Format text string
	// (uses standard Unreal "FText" formatting: eg, {PinName} will refer to input called PinName)
	// Note - complex types are exported "ToString" and InstancedStruct is not supported
	UPROPERTY(EditAnywhere, Category = "Flow", meta = (DefaultForInputFlowPin, FlowPinType = Text))
	FText FormatText;

protected:

#if WITH_EDITOR
public:
	virtual void UpdateNodeConfigText_Implementation() override;
#endif

	EFlowDataPinResolveResult TryResolveFormatText(const FName& PinName, FText& OutFormattedText) const;

public:
	// IFlowDataPinValueSupplierInterface
	virtual FFlowDataPinResult_Name TrySupplyDataPinAsName_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_String TrySupplyDataPinAsString_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Text TrySupplyDataPinAsText_Implementation(const FName& PinName) const override;
	// --

	static const FName OUTPIN_TextOutput;
};
