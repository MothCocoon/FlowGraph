// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowAssetFactory.h"
#include "FlowAsset.h"
#include "Graph/FlowGraph.h"

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
	UFlowAsset* NewFlow = NewObject<UFlowAsset>(InParent, Class, Name, Flags | RF_Transactional, Context);
	UFlowGraph::CreateGraph(NewFlow);
	return NewFlow;
}
