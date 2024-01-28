// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowOwnerFunctionRef.h"
#include "Nodes/FlowNode.h"

#include "FlowNode_CallOwnerFunction.generated.h"

class UFlowOwnerFunctionParams;
class IFlowOwnerInterface;

// Example signature for valid Flow Owner Functions
typedef TFunction<FName(UFlowOwnerFunctionParams* Params)> FFlowOwnerFunctionSignature;

/**
 * FlowNode to call an owner function
 * - Owner must implement IFlowOwnerInterface
 * - Callable functions must take a single input parameter deriving from UFlowOwnerFunctionParams
 *   and return FName for the Output event to trigger (or "None" to trigger none of the outputs)
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Call Owner Function"))
class FLOW_API UFlowNode_CallOwnerFunction : public UFlowNode
{
	GENERATED_UCLASS_BODY()

protected:
	// Function reference on the expected owner to call
	UPROPERTY(EditAnywhere, Category = "Call Owner", meta = (DisplayName = "Function"))
	FFlowOwnerFunctionRef FunctionRef;

	// Parameter object to pass to the function when called
	UPROPERTY(EditAnywhere, Category = "Call Owner", Instanced)
	UFlowOwnerFunctionParams* Params;

protected:
	// UFlowNode
	virtual void ExecuteInput(const FName& PinName) override;
	// ---

	bool TryExecuteOutputPin(const FName& OutputName);
	bool ShouldFinishForOutputName(const FName& OutputName) const;

#if WITH_EDITOR

public:
	// UFlowNode
	virtual FText GetNodeTitle() const override;
	virtual FString GetStatusString() const override;
	virtual EDataValidationResult ValidateNode() override;
	// ---

	// UObject
	virtual void PostLoad() override;
	virtual bool CanEditChange(const FProperty* InProperty) const override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// ---

protected:
	bool TryAllocateParamsInstance();
	void OnChangedParamsObject();

	// returns true if the InOutPins array was rebuilt
	bool RebuildPinArray(const TArray<FName>& NewPinNames, TArray<FFlowPin>& InOutPins, const FFlowPin& DefaultPin);

	UClass* GetRequiredParamsClass() const;
	UClass* GetExistingParamsClass() const;

	static UClass* GetParamsClassForFunctionName(const UClass& ExpectedOwnerClass, const FName& FunctionName);
	static UClass* GetParamsClassForFunction(const UFunction& Function);

public:
	bool IsAcceptableParamsPropertyClass(const UClass* ParamsClass) const;

	UClass* TryGetExpectedOwnerClass() const;
	static bool DoesFunctionHaveValidFlowOwnerFunctionSignature(const UFunction& Function);

protected:
	// Helper function for DoesFunctionHaveValidFlowOwnerFunctionSignature()
	static bool DoesFunctionHaveNameReturnType(const UFunction& Function);
#endif // WITH_EDITOR
};
