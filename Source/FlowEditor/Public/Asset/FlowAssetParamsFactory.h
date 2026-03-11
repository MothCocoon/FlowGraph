// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Factories/Factory.h"
#include "UObject/SoftObjectPtr.h"

#include "FlowAssetParamsFactory.generated.h"

class UFlowAssetParams;

/**
 * Factory for creating Flow Asset Params via the Content Browser "Add New" menu.
 * This creation path is strictly for creating CHILD params: the user must select a Parent FlowAssetParams.
 */
UCLASS(HideCategories = Object)
class FLOWEDITOR_API UFlowAssetParamsFactory : public UFactory
{
	GENERATED_BODY()

public:
	UFlowAssetParamsFactory();

	// UFactory
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	// --

private:
	/* Required selection. */
	TSoftObjectPtr<UFlowAssetParams> SelectedParentParams;

	bool ShowParentPickerDialog();
};
