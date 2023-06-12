// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowOwnerFunctionRefCustomization.h"

#include "FlowAsset.h"
#include "FlowOwnerInterface.h"
#include "Nodes/FlowNode.h"
#include "Nodes/World/FlowNode_CallOwnerFunction.h"

#include "UObject/UnrealType.h"
#include "FlowOwnerFunctionParams.h"


// FFlowOwnerFunctionRefCustomization Implementation

void FFlowOwnerFunctionRefCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Do not include children properties (the header is all we need to show for this struct)
}

TSharedPtr<IPropertyHandle> FFlowOwnerFunctionRefCustomization::GetCuratedNamePropertyHandle() const
{
	check(StructPropertyHandle->IsValidHandle());

	TSharedPtr<IPropertyHandle> FoundHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowOwnerFunctionRef, FunctionName));
	check(FoundHandle);

	return FoundHandle;
}

TArray<FName> FFlowOwnerFunctionRefCustomization::GetCuratedNameOptions() const
{
	TArray<FName> Results;

	const UClass* ExpectedOwnerClass = TryGetExpectedOwnerClass();
	if (!IsValid(ExpectedOwnerClass))
	{
		return Results;
	}

	const UFlowNode_CallOwnerFunction* FlowNodeOwner = Cast<UFlowNode_CallOwnerFunction>(TryGetFlowNodeOuter());
	if (!IsValid(FlowNodeOwner))
	{
		return Results;
	}

	Results = GetFlowOwnerFunctionRefs(*FlowNodeOwner, *ExpectedOwnerClass);

	return Results;
}

const UClass* FFlowOwnerFunctionRefCustomization::TryGetExpectedOwnerClass() const
{
	const UFlowNode* NodeOwner = TryGetFlowNodeOuter();
	const UFlowNode_CallOwnerFunction* CallOwnerFunctionNode = Cast<UFlowNode_CallOwnerFunction>(NodeOwner);

	if (IsValid(CallOwnerFunctionNode))
	{
		return CallOwnerFunctionNode->TryGetExpectedOwnerClass();
	}

	return nullptr;
}

TArray<FName> FFlowOwnerFunctionRefCustomization::GetFlowOwnerFunctionRefs(
	const UFlowNode_CallOwnerFunction& FlowNodeOwner,
	const UClass& ExpectedOwnerClass)
{
	TArray<FName> ValidFunctionNames;

	// Gather a list of potential functions
	TArray<FName> PotentialFunctionNames;
	ExpectedOwnerClass.GenerateFunctionList(PotentialFunctionNames);

	if (PotentialFunctionNames.Num() == 0)
	{
		return ValidFunctionNames;
	}

	ValidFunctionNames.Reserve(PotentialFunctionNames.Num());

	// Filter out any unusable functions (that do not match the expected signature)
	for (const FName& PotentialFunctionName : PotentialFunctionNames)
	{
		const UFunction* PotentialFunction = ExpectedOwnerClass.FindFunctionByName(PotentialFunctionName);
		check(IsValid(PotentialFunction));

		if (IsFunctionUsable(*PotentialFunction, FlowNodeOwner))
		{
			ValidFunctionNames.Add(PotentialFunctionName);
		}
	}

	return ValidFunctionNames;
}

bool FFlowOwnerFunctionRefCustomization::IsFunctionUsable(const UFunction& Function, const UFlowNode_CallOwnerFunction& FlowNodeOwner)
{
	if (!UFlowNode_CallOwnerFunction::DoesFunctionHaveValidFlowOwnerFunctionSignature(Function))
	{
		return false;
	}

	if (!DoesFunctionHaveExpectedParamType(Function, FlowNodeOwner))
	{
		return false;
	}

	return true;
}

bool FFlowOwnerFunctionRefCustomization::DoesFunctionHaveExpectedParamType(const UFunction& Function, const UFlowNode_CallOwnerFunction& FlowNodeOwner)
{
	const UClass* PropertyClass = UFlowNode_CallOwnerFunction::GetParamsClassForFunction(Function);

	return FlowNodeOwner.IsAcceptableParamsPropertyClass(PropertyClass);
}

void FFlowOwnerFunctionRefCustomization::SetCuratedName(const FName& NewFunctionName)
{
	TSharedPtr<IPropertyHandle> FunctionNameHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowOwnerFunctionRef, FunctionName));

	check(FunctionNameHandle);

	FunctionNameHandle->SetPerObjectValue(0, NewFunctionName.ToString());
}

FName FFlowOwnerFunctionRefCustomization::GetCuratedName() const
{
	const FFlowOwnerFunctionRef* FlowOwnerFunction = GetFlowOwnerFunctionRef();
	if (FlowOwnerFunction)
	{
		return FlowOwnerFunction->FunctionName;
	}
	else
	{
		return NAME_None;
	}
}

UFlowNode* FFlowOwnerFunctionRefCustomization::TryGetFlowNodeOuter() const
{
	check(StructPropertyHandle->IsValidHandle());

	TArray<UObject*> OuterObjects;
	StructPropertyHandle->GetOuterObjects(OuterObjects);

	for (UObject* OuterObject : OuterObjects)
	{
		UFlowNode* FlowNodeOuter = Cast<UFlowNode>(OuterObject);
		if (IsValid(FlowNodeOuter))
		{
			return FlowNodeOuter;
		}
	}

	return nullptr;
}
