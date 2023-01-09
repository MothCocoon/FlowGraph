// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowAsset.h"
#include "FlowImportUtils.generated.h"

USTRUCT()
struct FLOWEDITOR_API FImportedGraphNode
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	UEdGraphNode* SourceGraphNode;

	TMultiMap<FName, FConnectedPin> Incoming;
	TMultiMap<FName, FConnectedPin> Outgoing;

	FImportedGraphNode()
		: SourceGraphNode(nullptr)
	{
	}
};

/**
 * 
 */
UCLASS(meta = (ScriptName = "FlowImportUtils"))
class FLOWEDITOR_API UFlowImportUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static TMap<FName, TSubclassOf<UFlowNode>> FunctionsToFlowNodes;
	
	UFUNCTION(BlueprintCallable, Category = "FlowImportUtils")
	static UFlowAsset* ImportBlueprintGraph(UObject* BlueprintAsset, TSubclassOf<UFlowAsset> FlowAssetClass, FString FlowAssetName, TMap<FName, TSubclassOf<UFlowNode>> BlueprintFunctionsToFlowNodes, const FName StartEventName = TEXT("BeginPlay"));

	static void ImportBlueprintGraph(UBlueprint* Blueprint, UFlowAsset* FlowAsset, const FName StartEventName = TEXT("BeginPlay"));
	static void ImportBlueprintFunction(UFlowAsset* FlowAsset, const FImportedGraphNode& NodeImport, const TMap<FGuid, struct FImportedGraphNode>& SourceNodes, TMap<FGuid, UFlowGraphNode*>& TargetNodes);
};
