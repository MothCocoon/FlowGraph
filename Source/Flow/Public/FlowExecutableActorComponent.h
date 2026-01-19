// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Components/ActorComponent.h"
#include "Interfaces/FlowContextPinSupplierInterface.h"
#include "Interfaces/FlowCoreExecutableInterface.h"
#include "Interfaces/FlowDataPinValueOwnerInterface.h"
#include "Interfaces/FlowExternalExecutableInterface.h"

#include "FlowExecutableActorComponent.generated.h"

/**
 * A base class for blueprint components that are expected to be executed from an ExecuteComponent flow node.
 * Provides the support for FFlowDataPinValue subclasses, so that blueprint components (that derive from this)
 * can have their pins be automatically discovered and supplied.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DisplayName = "Flow Executable Actor Component", hidecategories = (Tags, Activation, Cooking, AssetUserData, Navigation))
class FLOW_API UFlowExecutableActorComponent
	: public UActorComponent
	, public IFlowContextPinSupplierInterface
	, public IFlowCoreExecutableInterface
	, public IFlowDataPinValueOwnerInterface
	, public IFlowExternalExecutableInterface
{
	GENERATED_BODY()

private:
	FSimpleDelegate FlowDataPinValuesRebuildDelegate;

protected:
	// FlowNodeBase that will execute this component in the FlowGraph on our behalf
	UPROPERTY(Transient, BlueprintReadOnly, Category = DataPins)
	TObjectPtr<UFlowNodeBase> FlowNodeProxy;

public:

	// IFlowContextPinSupplierInterface
	virtual bool K2_SupportsContextPins_Implementation() const override { return true; }
	// --

#if WITH_EDITOR

	// IFlowDataPinValueOwnerInterface
	virtual bool CanModifyFlowDataPinType() const override;
	virtual bool ShowFlowDataPinValueInputPinCheckbox() const override;
	virtual bool ShowFlowDataPinValueClassFilter(const FFlowDataPinValue* Value) const override;
	virtual bool CanEditFlowDataPinValueClassFilter(const FFlowDataPinValue* Value) const override;
	virtual void SetFlowDataPinValuesRebuildDelegate(FSimpleDelegate InDelegate) override;
	virtual void RequestFlowDataPinValuesDetailsRebuild() override;
	// --
#endif //WITH_EDITOR

	// IFlowExternalExecutableInterface
	virtual void PreActivateExternalFlowExecutable(UFlowNodeBase& FlowNodeBase) override;
	// --

protected:

	FORCEINLINE bool IsDefaultObject() const { return HasAnyFlags(RF_ClassDefaultObject); }
};