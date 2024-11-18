// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/World/FlowNode_CallOwnerFunction.h"

#include "FlowAsset.h"
#include "FlowLogChannels.h"
#include "Interfaces/FlowOwnerInterface.h"
#include "Types/FlowOwnerFunctionParams.h"
#include "FlowSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_CallOwnerFunction)

#define LOCTEXT_NAMESPACE "FlowNode"

UFlowNode_CallOwnerFunction::UFlowNode_CallOwnerFunction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Params(nullptr)
{
#if WITH_EDITOR
	NodeDisplayStyle = FlowNodeStyle::Deprecated;
	Category = TEXT("Deprecated");
#endif // WITH_EDITOR
}

void UFlowNode_CallOwnerFunction::ExecuteInput(const FName& PinName)
{
	Super::ExecuteInput(PinName);

	if (!IsValid(Params))
	{
		UE_LOG(LogFlow, Error, TEXT("Expected a valid Params object"));

		return;
	}

	IFlowOwnerInterface* FlowOwnerInterface = GetFlowOwnerInterface();
	if (!FlowOwnerInterface)
	{
		UE_LOG(LogFlow, Error, TEXT("Expected an owner that implements the IFlowOwnerInterface"));

		return;
	}

	const UObject* FlowOwnerObject = CastChecked<UObject>(FlowOwnerInterface);
	const UClass* FlowOwnerClass = FlowOwnerObject->GetClass();
	check(IsValid(FlowOwnerClass));

	if (!FunctionRef.TryResolveFunction(*FlowOwnerClass))
	{
		UE_LOG(
			LogFlow,
			Error,
			TEXT("Could not resolve function named %s with flow owner class %s"),
			*FunctionRef.GetFunctionName().ToString(),
			*FlowOwnerClass->GetName());

		return;
	}

	Params->PreExecute(*this, PinName);

	const FName ResultOutputName = FunctionRef.CallFunction(*FlowOwnerInterface, *Params);

	Params->PostExecute();

	(void) TryExecuteOutputPin(ResultOutputName);
}

bool UFlowNode_CallOwnerFunction::TryExecuteOutputPin(const FName& OutputName)
{
	if (OutputName.IsNone())
	{
		return false;
	}

	const bool bFinish = ShouldFinishForOutputName(OutputName);
	TriggerOutput(OutputName, bFinish);

	return true;
}

bool UFlowNode_CallOwnerFunction::ShouldFinishForOutputName(const FName& OutputName) const
{
	if (ensure(IsValid(Params)))
	{
		return Params->ShouldFinishForOutputName(OutputName);
	}

	return true;
}

#if WITH_EDITOR

void UFlowNode_CallOwnerFunction::PostLoad()
{
	Super::PostLoad();

	FObjectPropertyBase* ParamsProperty = FindFProperty<FObjectPropertyBase>(GetClass(), GET_MEMBER_NAME_CHECKED(UFlowNode_CallOwnerFunction, Params));
	check(ParamsProperty);

	// NOTE (gtaylor) This fixes corruption in FlowNodes that could have been caused with
	// a previous version of the code (which was inadvisedly calling SetPropertyClass)
	// to restore the correct PropertyClass for this node.  
	// (it could be removed in a future release, once all assets have been updated)
	if (ParamsProperty->PropertyClass != UFlowOwnerFunctionParams::StaticClass())
	{
		ParamsProperty->SetPropertyClass(UFlowOwnerFunctionParams::StaticClass());
	}
}

bool UFlowNode_CallOwnerFunction::CanEditChange(const FProperty* InProperty) const
{
	if (!Super::CanEditChange(InProperty))
	{
		return false;
	}

	const FName PropertyName = InProperty->GetFName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UFlowNode_CallOwnerFunction, Params))
	{
		return false;
	}

	return true;
}

void UFlowNode_CallOwnerFunction::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName MemberPropertyName = PropertyChangedEvent.MemberProperty->GetFName();

	if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UFlowNode_CallOwnerFunction, Params))
	{
		OnReconstructionRequested.ExecuteIfBound();
	}

	const FName PropertyName = PropertyChangedEvent.Property->GetFName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FFlowOwnerFunctionRef, FunctionName))
	{
		if (TryAllocateParamsInstance())
		{
			OnReconstructionRequested.ExecuteIfBound();
		}
	}
}

EDataValidationResult UFlowNode_CallOwnerFunction::ValidateNode()
{
	const bool bHasFunction = FunctionRef.IsConfigured();
	if (!bHasFunction)
	{
		ValidationLog.Error<UFlowNode>(TEXT("CallOwnerFunction requires a valid Function reference"), this);

		return EDataValidationResult::Invalid;
	}

	const bool bHasParams = IsValid(Params);
	if (!bHasParams)
	{
		ValidationLog.Error<UFlowNode>(TEXT("CallOwnerFunction requires a valid Params object"), this);

		return EDataValidationResult::Invalid;
	}

	checkf(bHasParams && bHasFunction, TEXT("This should be assured by the preceding logic"));

	const UClass* ExpectedOwnerClass = TryGetExpectedOwnerClass();
	if (!IsValid(ExpectedOwnerClass))
	{
		ValidationLog.Error<UFlowNode>(TEXT("Invalid or null Expected Owner Class for this Flow Asset"), this);

		return EDataValidationResult::Invalid;
	}

	// Check if the function can be found on the expected owner
	const UFunction* Function = FunctionRef.TryResolveFunction(*ExpectedOwnerClass);
	if (!IsValid(Function))
	{
		ValidationLog.Error<UFlowNode>(TEXT("Could not resolve function for flow owner"), this);

		return EDataValidationResult::Invalid;
	}

	// Check the function signature
	if (!DoesFunctionHaveValidFlowOwnerFunctionSignature(*Function))
	{
		ValidationLog.Error<UFlowNode>(TEXT("Flow Owner Function has an invalid signature"), this);

		return EDataValidationResult::Invalid;
	}

	const UClass* RequiredParamsClass = GetRequiredParamsClass();
	checkf(IsValid(RequiredParamsClass), TEXT("GetRequiredParamsClass() cannot return null if DoesFunctionHaveValidFlowOwnerFunctionSignature() is true"));

	const UClass* ExistingParamsClass = GetExistingParamsClass();
	checkf(IsValid(ExistingParamsClass), TEXT("This should be assured, if bHasParams == true"));

	// Check if the params (existing) are compatible with the function's expected (required) params
	if (!ExistingParamsClass->IsChildOf(RequiredParamsClass))
	{
		ValidationLog.Error<UFlowNode>(TEXT("Params object is not of the correct type for the flow owner function"), this);

		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}

FString UFlowNode_CallOwnerFunction::GetStatusString() const
{
	if (ActivationState != EFlowNodeState::NeverActivated)
	{
		return UEnum::GetDisplayValueAsText(ActivationState).ToString();
	}

	return Super::GetStatusString();
}

UClass* UFlowNode_CallOwnerFunction::TryGetExpectedOwnerClass() const
{
	const UFlowAsset* FlowAsset = GetFlowAsset();
	if (IsValid(FlowAsset))
	{
		return FlowAsset->GetExpectedOwnerClass();
	}

	return nullptr;
}

bool UFlowNode_CallOwnerFunction::DoesFunctionHaveValidFlowOwnerFunctionSignature(const UFunction& Function)
{
	if (GetParamsClassForFunction(Function) == nullptr)
	{
		return false;
	}

	checkf(Function.NumParms == 2, TEXT("This should be checked in GetParamsClassForFunction()"));

	if (!DoesFunctionHaveNameReturnType(Function))
	{
		return false;
	}

	return true;
}

bool UFlowNode_CallOwnerFunction::DoesFunctionHaveNameReturnType(const UFunction& Function)
{
	checkf(Function.NumParms == 2, TEXT("This should have already been checked in DoesFunctionHaveValidFlowOwnerFunctionSignature()"));

	TFieldIterator<FNameProperty> Iterator(&Function);

	while (Iterator)
	{
		const bool bIsOutParm = EnumHasAllFlags(Iterator->PropertyFlags, CPF_Parm | CPF_OutParm);

		return bIsOutParm;
	}

	return false;
}

UClass* UFlowNode_CallOwnerFunction::GetParamsClassForFunction(const UFunction& Function)
{
	if (Function.NumParms != 2)
	{
		// Flow Owner Functions expect exactly two parameters:
		//  - FFlowOwnerFunctionParams* 
		//  - FName (return)
		// See FFlowOwnerFunctionSignature

		return nullptr;
	}

	TFieldIterator<FObjectPropertyBase> Iterator(&Function);

	while (Iterator && (Iterator->PropertyFlags & CPF_Parm))
	{
		const FObjectPropertyBase* Prop = *Iterator;
		check(Prop);

		UClass* PropertyClass = Prop->PropertyClass;

		if (!IsValid(PropertyClass))
		{
			return nullptr;
		}

		if (!PropertyClass->IsChildOf<UFlowOwnerFunctionParams>())
		{
			return nullptr;
		}

		return PropertyClass;
	}

	return nullptr;
}

UClass* UFlowNode_CallOwnerFunction::GetParamsClassForFunctionName(const UClass& ExpectedOwnerClass, const FName& FunctionName)
{
	const UFunction* Function = ExpectedOwnerClass.FindFunctionByName(FunctionName);
	if (IsValid(Function))
	{
		return GetParamsClassForFunction(*Function);
	}

	return nullptr;
}

bool UFlowNode_CallOwnerFunction::TryAllocateParamsInstance()
{
	if (FunctionRef.GetFunctionName().IsNone())
	{
		// Throw out the old params object (if any)
		Params = nullptr;

		return false;
	}

	const UClass* ExistingParamsClass = GetExistingParamsClass();
	const UClass* RequiredParamsClass = GetRequiredParamsClass();

	if (!IsValid(RequiredParamsClass))
	{
		return false;
	}

	const bool bNeedsAllocateParams = !IsValid(ExistingParamsClass) || ExistingParamsClass != RequiredParamsClass;

	if (!bNeedsAllocateParams)
	{
		return false;
	}

	// Throw out the old params object (if any)
	Params = nullptr;

	// Create the new params object
	Params = NewObject<UFlowOwnerFunctionParams>(this, RequiredParamsClass);

	return true;
}

UClass* UFlowNode_CallOwnerFunction::GetRequiredParamsClass() const
{
	const UClass* ExpectedOwnerClass = TryGetExpectedOwnerClass();
	if (!IsValid(ExpectedOwnerClass))
	{
		return nullptr;
	}

	const FName FunctionNameAsName = FunctionRef.GetFunctionName();

	if (FunctionNameAsName.IsNone())
	{
		return nullptr;
	}

	UClass* RequiredParamsClass = GetParamsClassForFunctionName(*ExpectedOwnerClass, FunctionNameAsName);
	return RequiredParamsClass;
}

UClass* UFlowNode_CallOwnerFunction::GetExistingParamsClass() const
{
	if (!IsValid(Params))
	{
		return nullptr;
	}

	UClass* ExistingParamsClass = Params->GetClass();
	return ExistingParamsClass;
}

bool UFlowNode_CallOwnerFunction::IsAcceptableParamsPropertyClass(const UClass* ParamsClass) const
{
	if (!IsValid(ParamsClass))
	{
		return false;
	}

	if (!ParamsClass->IsChildOf<UFlowOwnerFunctionParams>())
	{
		return false;
	}

	const UClass* ExistingParamsClass = GetExistingParamsClass();

	if (IsValid(ExistingParamsClass) && ParamsClass != ExistingParamsClass)
	{
		return false;
	}

	return true;
}

FText UFlowNode_CallOwnerFunction::GetNodeTitle() const
{
	const bool bUseAdaptiveNodeTitles = UFlowSettings::Get()->bUseAdaptiveNodeTitles;

	if (bUseAdaptiveNodeTitles && !FunctionRef.GetFunctionName().IsNone())
	{
		const FText FunctionNameText = FText::FromName(FunctionRef.FunctionName);

		return FText::Format(LOCTEXT("CallOwnerFunction", "Call {0}"), {FunctionNameText});
	}
	else
	{
		return Super::GetNodeTitle();
	}
}

#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
