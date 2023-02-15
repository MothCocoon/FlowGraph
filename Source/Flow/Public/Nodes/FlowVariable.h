// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "StructUtils/Public/PropertyBag.h"
#include "FlowVariable.generated.h"

class UFlowNode;

USTRUCT(BlueprintType)
struct FLOW_API FFlowPropertyBag
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = FlowProperty)
	FInstancedPropertyBag Properties;

	UPROPERTY(Transient)
	FInstancedPropertyBag RuntimeBag;

	UPROPERTY()
	FName PropertyName;

	UPROPERTY()
	FString PinTooltip;

	FORCEINLINE bool IsValid() const { return !PropertyName.IsNone(); }

	FFlowPropertyBag() = default;
	FFlowPropertyBag(const FProperty& Property);
};

USTRUCT()
struct FLOW_API FFlowInputOutputPin
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	FInstancedPropertyBag PropertyBag;

	UPROPERTY()
	FName InputPinName;

	UPROPERTY()
	FName OutputPinName;

	UPROPERTY()
	FGuid InputNodeGuid;

	UPROPERTY()
	FGuid OutputNodeGuid;

	UPROPERTY()
	FString PinTooltip;

	FFlowInputOutputPin() = default;
	FFlowInputOutputPin(const FName& InputPinName, const FName& OutputPinName, const FGuid& InputNodeGuid, const FGuid& OutputNodeGuid);

	bool IsPinValid() const;
	void SetInputValue(const UFlowNode* FromOutputNode, const FProperty* InProperty, const FProperty* OutProperty);
};
