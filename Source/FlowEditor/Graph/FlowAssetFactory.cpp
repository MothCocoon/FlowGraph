#include "FlowAssetFactory.h"
#include "Flow/Graph/FlowAsset.h"

UFlowAssetFactory::UFlowAssetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UFlowAsset::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UFlowAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UFlowAsset>(InParent, Class, Name, Flags | RF_Transactional, Context);
}
