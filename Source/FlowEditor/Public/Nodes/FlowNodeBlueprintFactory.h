// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Factories/Factory.h"
#include "FlowNodeBlueprintFactory.generated.h"

UCLASS(hidecategories=Object, MinimalAPI)
class UFlowNodeBlueprintFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	// The parent class of the created blueprint
	UPROPERTY(EditAnywhere, Category = "FlowNodeBlueprintFactory")
	TSubclassOf<class UFlowNode> ParentClass;

	// UFactory
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	// --
};
