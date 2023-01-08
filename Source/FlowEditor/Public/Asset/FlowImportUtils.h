// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowAsset.h"
#include "FlowImportUtils.generated.h"

USTRUCT()
struct FLOWEDITOR_API FGraphNodeImport
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	UEdGraphNode* SourceGraphNode;

	TMap<FName, FConnectedPin> Connections

	FGraphNodeImport()
		: SourceGraphNode(nullptr)
	{
	}
};

/**
 * 
 */
UCLASS(meta = (ScriptName = "FlowImportUtils"))
class FLOWEDITOR_API UFlowImportUtils final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FlowImportUtils")
	static UFlowAsset* ImportBlueprintGraph(UObject* BlueprintAsset, TSubclassOf<UFlowAsset> FlowAssetClass, FString FlowAssetName, const FName StartEventName = TEXT("BeginPlay"));

	static void ImportBlueprintGraph(UBlueprint* Blueprint, const UFlowAsset* FlowAsset, const FName StartEventName = TEXT("BeginPlay"));
	static void ImportBlueprintFunction_Recursive(UEdGraphNode* PrecedingGraphNode, const TMap<FGuid, struct FGraphNodeImport> SourceNodes);
};
