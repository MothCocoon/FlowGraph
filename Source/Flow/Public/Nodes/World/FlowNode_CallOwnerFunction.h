// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "GameplayTagContainer.h"

#include "Types/FlowOwnerFunctionRef.h"
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

public:

#if WITH_EDITOR
	// UObject
	virtual void PostLoad() override;
	virtual bool CanEditChange(const FProperty* InProperty) const override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// --

	// UFlowNode
	virtual FText GetNodeTitle() const override;
	virtual EDataValidationResult ValidateNode() override;

	virtual FString GetStatusString() const override;
	// --
#endif // WITH_EDITOR

	UClass* GetRequiredParamsClass() const;
	UClass* GetExistingParamsClass() const;

	bool IsAcceptableParamsPropertyClass(const UClass* ParamsClass) const;

	UClass* TryGetExpectedOwnerClass() const;

	static bool DoesFunctionHaveValidFlowOwnerFunctionSignature(const UFunction& Function);

	static UClass* GetParamsClassForFunctionName(const UClass& ExpectedOwnerClass, const FName& FunctionName);
	static UClass* GetParamsClassForFunction(const UFunction& Function);

protected:
	// UFlowNode
	virtual void ExecuteInput(const FName& PinName) override;
	// --

	bool ShouldFinishForOutputName(const FName& OutputName) const;
	bool TryExecuteOutputPin(const FName& OutputName);

	bool TryAllocateParamsInstance();

	// Helper function for DoesFunctionHaveValidFlowOwnerFunctionSignature()
	static bool DoesFunctionHaveNameReturnType(const UFunction& Function);

protected:
	// Function reference on the expected owner to call
	// DEPRECATED - Sunsetting this feature from FlowGraph with the next release.  Custom FlowNodes are a better mechanism to use
	UPROPERTY(EditAnywhere, Category = "Call Owner", meta = (DisplayName = "DEPRECATED - Function"))
	FFlowOwnerFunctionRef FunctionRef;

	// Parameter object to pass to the function when called
	UPROPERTY(EditAnywhere, Category = "Call Owner", Instanced)
	TObjectPtr<UFlowOwnerFunctionParams> Params;
};
