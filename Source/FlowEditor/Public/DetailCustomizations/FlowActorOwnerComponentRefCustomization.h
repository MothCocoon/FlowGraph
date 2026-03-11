// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "UnrealExtensions/IFlowCuratedNamePropertyCustomization.h"
#include "Types/FlowActorOwnerComponentRef.h"

class UFlowAsset;
class UFlowNode;
class UObject;
class UClass;

/**
 * Details customization for FFlowActorOwnerComponentRef.
 */
class FFlowActorOwnerComponentRefCustomization : public IFlowCuratedNamePropertyCustomization
{
private:
	typedef IFlowCuratedNamePropertyCustomization Super;

public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShareable(new FFlowActorOwnerComponentRefCustomization()); }

protected:

	// IPropertyTypeCustomization
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	// --

	// ICuratedNamePropertyCustomization
	virtual TSharedPtr<IPropertyHandle> GetCuratedNamePropertyHandle() const override;
	virtual void SetCuratedName(const FName& NewName) override;
	virtual bool TryGetCuratedName(FName& OutName) const override;
	virtual TArray<FName> GetCuratedNameOptions() const override;
	// --

	/* Accessor to return the actual struct being edited. */
	FORCEINLINE FFlowActorOwnerComponentRef* GetFlowActorOwnerComponentRef() const
		{ return IFlowExtendedPropertyTypeCustomization::TryGetTypedStructValue<FFlowActorOwnerComponentRef>(StructPropertyHandle); }

	UClass* TryGetExpectedOwnerClass() const;
	UFlowNode* TryGetFlowNodeOuter() const;

	TArray<FName> GetFlowActorOwnerComponents(TSubclassOf<AActor> ExpectedActorOwnerClass) const;
};
