// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Factories/Factory.h"
#include "FlowAssetFactory.generated.h"

UCLASS(HideCategories = Object)
class FLOWEDITOR_API UFlowAssetFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = Asset)
	TSubclassOf<class UFlowAsset> AssetClass;

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

protected:
	// Parameterized guts of ConfigureProperties()
	bool ConfigurePropertiesInternal(const FText& TitleText);
};
