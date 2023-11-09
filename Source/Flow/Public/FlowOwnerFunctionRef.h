// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

#include "FlowOwnerFunctionRef.generated.h"


// Forward Declarations
class UFlowOwnerFunctionParams;
class IFlowOwnerInterface;


// Similar to FAnimNodeFunctionRef, providing a FName-based function binding
//  that is resolved at runtime
USTRUCT(BlueprintType)
struct FFlowOwnerFunctionRef
{
	GENERATED_BODY()

	// For GET_MEMBER_NAME_CHECKED access
	friend class UFlowNode_CallOwnerFunction;
	friend class FFlowOwnerFunctionRefCustomization;

public:

	// Resolves the function and returns the UFunction
	UFunction* TryResolveFunction(const UClass& InClass);

	// Returns a the resolved function
	//  (assumes TryResolveFunction was called previously)
	UFunction* GetResolvedFunction() const { return Function; }

	// Call the function and return the Output Pin Name result
	FName CallFunction(IFlowOwnerInterface& InFlowOwnerInterface, UFlowOwnerFunctionParams& InParams) const;

	// Accessors
	FName GetFunctionName() const { return FunctionName; }
	bool IsConfigured() const { return !FunctionName.IsNone(); }
	bool IsResolved() const { return ::IsValid(Function); }

protected:

	// The name of the function to call
	UPROPERTY(VisibleAnywhere, Category = "FlowOwnerFunction")
	FName FunctionName = NAME_None;	

	// The function to call
	//  (resolved by looking for a function named FunctionName on the ExpectedOwnerClass)
	UPROPERTY(Transient)
	TObjectPtr<UFunction> Function = nullptr;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "FlowOwnerFunction", meta = (DisplayName = "Function Parameters Class"))
	TSubclassOf<UFlowOwnerFunctionParams> ParamsClass;
#endif // WITH_EDITORONLY_DATA
};
