// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowNode_SubGraphDetails.h"

#include "DetailLayoutBuilder.h"
#include "FlowAsset.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"

void FFlowNode_SubGraphDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingEdited;
	DetailLayout.GetObjectsBeingCustomized(ObjectsBeingEdited);

	if (ObjectsBeingEdited[0].IsValid())
	{
		const UFlowNode_SubGraph* SubGraphNode = CastChecked<UFlowNode_SubGraph>(ObjectsBeingEdited[0]);

		// Generate the list of asset classes allowed or disallowed
		TArray<UClass*> FlowAssetClasses;
		GetDerivedClasses(UFlowAsset::StaticClass(), FlowAssetClasses, true);
		FlowAssetClasses.Add(UFlowAsset::StaticClass());

		TArray<UClass*> DisallowedClasses;
		TArray<UClass*> AllowedClasses;
		for (auto FlowAssetClass : FlowAssetClasses)
		{
			if (const UFlowAsset* DefaultAsset = Cast<UFlowAsset>(FlowAssetClass->GetDefaultObject()))
			{
				if (DefaultAsset->DeniedInSubgraphNodeClasses.Contains(SubGraphNode->GetClass()))
				{
					DisallowedClasses.Add(FlowAssetClass);
				}
	
				if (DefaultAsset->AllowedInSubgraphNodeClasses.Contains(SubGraphNode->GetClass()))
				{
					AllowedClasses.Add(FlowAssetClass);
				}
			}
		}
	
		DisallowedClasses.Append(SubGraphNode->DeniedAssignedAssetClasses);
	
		for (auto FlowAssetClass : SubGraphNode->AllowedAssignedAssetClasses)
		{
			if (!DisallowedClasses.Contains(FlowAssetClass))
			{
				AllowedClasses.AddUnique(FlowAssetClass);
			}
		}
	
		FString AllowedClassesString;
		for (UClass* Class : AllowedClasses)
		{
			AllowedClassesString.Append(FString::Printf(TEXT("%s,"), *Class->GetClassPathName().ToString()));
		}
	
		FString DisallowedClassesString;
		for (UClass* Class : DisallowedClasses)
		{
			DisallowedClassesString.Append(FString::Printf(TEXT("%s,"), *Class->GetClassPathName().ToString()));
		}
	
		const auto AssetProperty = DetailLayout.GetProperty(FName("Asset"), UFlowNode_SubGraph::StaticClass());
		if (FProperty* MetaDataProperty = AssetProperty->GetMetaDataProperty())
		{
			MetaDataProperty->SetMetaData(TEXT("AllowedClasses"), *AllowedClassesString);
			MetaDataProperty->SetMetaData(TEXT("DisallowedClasses"), *DisallowedClassesString);
		}
	}
}
