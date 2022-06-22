// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Factories/Factory.h"
#include "FlowAssetFactory.generated.h"

UCLASS(HideCategories = Object, MinimalAPI)
class UFlowAssetFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
