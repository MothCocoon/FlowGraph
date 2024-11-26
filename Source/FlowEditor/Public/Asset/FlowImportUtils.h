// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "FlowAsset.h"
#include "Nodes/FlowPin.h"
#include "FlowImportUtils.generated.h"

// Helper structure allowing to recreate blueprint graph as Flow Graph
USTRUCT()
struct FLOWEDITOR_API FImportedGraphNode
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TObjectPtr<UEdGraphNode> SourceGraphNode;

	TMultiMap<FName, FConnectedPin> Incoming;
	TMultiMap<FName, FConnectedPin> Outgoing;

	FImportedGraphNode()
		: SourceGraphNode(nullptr)
	{
	}
};

// Helper structure allowing to copy properties from blueprint function pin to the Flow Node property of different name
USTRUCT(BlueprintType)
struct FLOWEDITOR_API FBlueprintToFlowPinName
{
	GENERATED_USTRUCT_BODY()

	// Key represents Flow Node property name
	// Value represents Input Pin name of blueprint function
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pins")
	TMap<FName, FName> NodePropertiesToFunctionPins;

	FBlueprintToFlowPinName()
	{
	}
};

/**
 * Groundwork for converting blueprint graphs to Flow Graph.
 * It's NOT meant to be universal, out-of-box solution as complexity of blueprint graphs conflicts with simplicity of Flow Graph.
 * However, it might useful to provide this basic utility to anyone who would like to batch-convert their custom blueprint-based event system to Flow Graph.
 * Pull requests are welcome if you able to improve this utility w/o with minimal amount of code.
 */
UCLASS(meta = (ScriptName = "FlowImportUtils"))
class FLOWEDITOR_API UFlowImportUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static TMap<FName, TSubclassOf<UFlowNode>> FunctionsToFlowNodes;
	static TMap<TSubclassOf<UFlowNode>, FBlueprintToFlowPinName> PinMappings;

	UFUNCTION(BlueprintCallable, Category = "FlowImportUtils")
	static UFlowAsset* ImportBlueprintGraph(UObject* BlueprintAsset, const TSubclassOf<UFlowAsset> FlowAssetClass, const FString FlowAssetName,
											const TMap<FName, TSubclassOf<UFlowNode>> InFunctionsToFlowNodes, const TMap<TSubclassOf<UFlowNode>, FBlueprintToFlowPinName> InPinMappings, const FName StartEventName = TEXT("BeginPlay"));

	static void ImportBlueprintGraph(UBlueprint* Blueprint, UFlowAsset* FlowAsset, const FName StartEventName = TEXT("BeginPlay"));
	static void ImportBlueprintFunction(const UFlowAsset* FlowAsset, const FImportedGraphNode& NodeImport, const TMap<FGuid, struct FImportedGraphNode>& SourceNodes, TMap<FGuid, class UFlowGraphNode*>& TargetNodes);

	static void GetValidInputPins(const UEdGraphNode* GraphNode, TMap<const FName, const UEdGraphPin*>& Result);
	static const UEdGraphPin* FindPinMatchingToProperty(UClass* FlowNodeClass, const FProperty* Property, const TMap<const FName, const UEdGraphPin*>Pins);
};
