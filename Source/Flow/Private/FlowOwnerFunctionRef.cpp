// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowOwnerFunctionRef.h"
#include "FlowOwnerFunctionParams.h"
#include "FlowModule.h"
#include "FlowOwnerInterface.h"

#include "UObject/Class.h"
#include "Logging/LogMacros.h"


// FFlowOwnerFunctionRef Implementation

UFunction* FFlowOwnerFunctionRef::TryResolveFunction(const UClass& InClass)
{
	if (IsConfigured())
	{
		Function = InClass.FindFunctionByName(FunctionName);
	}
	else
	{
		Function = nullptr;
	}

	return Function;
}

FName FFlowOwnerFunctionRef::CallFunction(IFlowOwnerInterface& InFlowOwnerInterface, UFlowOwnerFunctionParams& InParams) const
{
	if (!IsResolved())
	{
		const UObject* FlowOwnerObject = CastChecked<UObject>(&InFlowOwnerInterface);

		UE_LOG(
			LogFlow,
			Error,
			TEXT("Could not resolve function named %s with flow owner class %s"),
			*FunctionName.ToString(),
			*FlowOwnerObject->GetClass()->GetName());

		return NAME_None;
	}

	UObject* FlowOwnerObject = CastChecked<UObject>(&InFlowOwnerInterface);

	struct FFlowOwnerFunctionRef_Parms
	{
		// Single FunctionParams object parameter
		UFlowOwnerFunctionParams* Params;

		// Return value
		FName OutputPinName;
	};

	FFlowOwnerFunctionRef_Parms Parms = { &InParams, NAME_None };

	// Call the owner function itself
	FlowOwnerObject->ProcessEvent(Function, &Parms);

	// Ensure the return value is valid
	if (!Parms.OutputPinName.IsNone())
	{
		const TArray<FName> OutputNames = InParams.GatherOutputNames();

		if (!OutputNames.Contains(Parms.OutputPinName))
		{
			FString OutputNamesStr = TEXT("None");
			for (const FName& OutputName : OutputNames)
			{
				OutputNamesStr += TEXT(", ") + OutputName.ToString();
			}

			UE_LOG(
				LogFlow,
				Error,
				TEXT("Flow Owner Function %s returned an invalid OutputPinName '%s', which is not in the valid outputs: { %s }"),
				*FunctionName.ToString(),
				*Parms.OutputPinName.ToString(),
				*OutputNamesStr);

			// Replace the invalid output pin name with None
			Parms.OutputPinName = NAME_None;
		}
	}

	return Parms.OutputPinName;
}
