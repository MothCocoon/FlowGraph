#pragma once

#include "Factories/Factory.h"
#include "FlowAssetFactory.generated.h"

UCLASS(hidecategories=Object, MinimalAPI)
class UFlowAssetFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
};
