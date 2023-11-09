// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UnrealExtensions/IFlowCuratedNamePropertyCustomization.h"

#include "FlowOwnerFunctionRef.h"


// Forward Declaration
class UFlowAsset;
class UFlowNode;
class UObject;
class UClass;
class UFunction;
class UFlowNode_CallOwnerFunction;


// Details customization for FFlowOwnerFunctionRef
class FFlowOwnerFunctionRefCustomization : public IFlowCuratedNamePropertyCustomization
{
private:
	typedef IFlowCuratedNamePropertyCustomization Super;

public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShareable(new FFlowOwnerFunctionRefCustomization()); }

protected:

	//~Begin IPropertyTypeCustomization
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	//~End IPropertyTypeCustomization

	//~Begin ICuratedNamePropertyCustomization
	virtual TSharedPtr<IPropertyHandle> GetCuratedNamePropertyHandle() const override;
	virtual void SetCuratedName(const FName& NewName) override;
	virtual FName GetCuratedName() const override;
	virtual TArray<FName> GetCuratedNameOptions() const override;
	//~End ICuratedNamePropertyCustomization

	// Accessor to return the actual struct being edited
	FORCEINLINE FFlowOwnerFunctionRef* GetFlowOwnerFunctionRef() const
		{ return IFlowExtendedPropertyTypeCustomization::TryGetTypedStructValue<FFlowOwnerFunctionRef>(StructPropertyHandle); }

	const UClass* TryGetExpectedOwnerClass() const;
	UFlowNode* TryGetFlowNodeOuter() const;

	static TArray<FName> GetFlowOwnerFunctionRefs(const UFlowNode_CallOwnerFunction& FlowNodeOwner, const UClass& ExpectedOwnerClass);

	static bool IsFunctionUsable(const UFunction& Function, const UFlowNode_CallOwnerFunction& FlowNodeOwner);
	static bool DoesFunctionHaveExpectedParamType(const UFunction& Function, const UFlowNode_CallOwnerFunction& FlowNodeOwner);

};
