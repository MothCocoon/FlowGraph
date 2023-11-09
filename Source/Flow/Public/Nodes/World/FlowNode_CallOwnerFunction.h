// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "CoreMinimal.h"

#include "GameplayTagContainer.h"

#include "FlowOwnerFunctionRef.h"
#include "Nodes/FlowNode.h"

#include "FlowNode_CallOwnerFunction.generated.h"


// Forward Declarations
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
	GENERATED_BODY()

public:

#if WITH_EDITOR
	//Begin UObject
	virtual void PostLoad() override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	//End UObject

	//Begin UFlowNode public
	virtual FText GetNodeTitle() const override;
	virtual EDataValidationResult ValidateNode() override;

	virtual FString GetStatusString() const override;
	//End UFlowNode public
#endif // WITH_EDITOR

	UFlowNode_CallOwnerFunction();

	UClass* GetRequiredParamsClass() const;
	UClass* GetExistingParamsClass() const;

	bool IsAcceptableParamsPropertyClass(const UClass* ParamsClass) const;

	UClass* TryGetExpectedOwnerClass() const;

	static bool DoesFunctionHaveValidFlowOwnerFunctionSignature(const UFunction& Function);

	static UClass* GetParamsClassForFunctionName(const UClass& ExpectedOwnerClass, const FName& FunctionName);
	static UClass* GetParamsClassForFunction(const UFunction& Function);

protected:

#if WITH_EDITOR
	// returns true if the InOutPins array was rebuilt
	bool RebuildPinArray(const TArray<FName>& NewPinNames, TArray<FFlowPin>& InOutPins, const FFlowPin& DefaultPin);

	void OnChangedParamsObject();
#endif // WITH_EDITOR

	//Begin UFlowNode protected
	virtual void ExecuteInput(const FName& PinName) override;
	//End UFlowNode protected

	bool ShouldFinishForOutputName(const FName& OutputName) const;
	bool TryExecuteOutputPin(const FName& OutputName);

	bool TryAllocateParamsInstance();

	// Helper function for DoesFunctionHaveValidFlowOwnerFunctionSignature()
	static bool DoesFunctionHaveNameReturnType(const UFunction& Function);

protected:

	// Function reference on the expected owner to call
	UPROPERTY(EditAnywhere, Category = "Call Owner", meta = (DisplayName = "Function"))
	FFlowOwnerFunctionRef FunctionRef;

	// Parameter object to pass to the function when called
	UPROPERTY(EditAnywhere, Category = "Call Owner", Instanced)
	UFlowOwnerFunctionParams* Params;
};
