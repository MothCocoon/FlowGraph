// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Factories/Factory.h"
#include "FlowNodeBlueprintFactory.generated.h"

UCLASS(Abstract, hidecategories=Object)
class UFlowNodeBaseBlueprintFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	// The Default parent class of the created blueprint (set by subclasses)
	UPROPERTY()
	TSubclassOf<class UFlowNodeBase> DefaultParentClass;
		
	// The parent class of the created blueprint
	UPROPERTY(EditAnywhere, Category = "FlowNodeBlueprintFactory")
	TSubclassOf<class UFlowNodeBase> ParentClass;

	// UFactory
	FLOWEDITOR_API virtual bool ConfigureProperties() override;
	FLOWEDITOR_API virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
	FLOWEDITOR_API virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

	void ShowCannotCreateBlueprintDialog();
	// --
};

// Specialization of UFlowNodeBaseBlueprintFactory for UFlowNode blueprints
UCLASS(hidecategories = Object)
class FLOWEDITOR_API UFlowNodeBlueprintFactory : public UFlowNodeBaseBlueprintFactory
{
	GENERATED_UCLASS_BODY()
};

// Specialization of UFlowNodeBaseBlueprintFactory for UFlowNodeAddOn blueprints
UCLASS(hidecategories = Object)
class FLOWEDITOR_API UFlowNodeAddOnBlueprintFactory : public UFlowNodeBaseBlueprintFactory
{
	GENERATED_UCLASS_BODY()

};
