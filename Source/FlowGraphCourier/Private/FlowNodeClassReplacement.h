// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Containers/Set.h"
#include "FlowCourierDocument.h"
#include "FlowGraphImporter.h"
#include "FlowMCPToolsetTypes.h"

class UFlowAsset;
class UFlowGraphNode;
class UFlowNode;
class UFlowNodeBase;

struct FFlowNodeClassReplacementPlan
{
	UFlowAsset* Asset = nullptr;
	UFlowNode* Original = nullptr;
	UFlowGraphNode* EditorNode = nullptr;
	UClass* TargetClass = nullptr;
	FGuid Guid;
	TMap<FString, FString> Properties;
	TMap<FString, FString> SourcePropertyNames;
	TArray<FFlowCourierPin> InputPins;
	TArray<FFlowCourierPin> OutputPins;
	TMap<FName, FName> PinMappings;
	TArray<FFlowGraphParsedNodeAddOn> AddOns;
	TArray<FGuid> ReplacedAddOnGuids;
	TArray<FFlowCourierOp> Connections;
	TArray<FFlowCourierOp> AllConnections;
	int32 AddOnCount = 0;
	bool bNeedsReplacement = false;
};

class FFlowNodeClassReplacement
{
public:
	static bool BuildPlan(
		const FFlowMCPReplaceFlowNodeClassRequest& Request,
		FFlowNodeClassReplacementPlan& OutPlan,
		TArray<FString>& OutFindings);

	static bool ApplyPlan(
		const FFlowNodeClassReplacementPlan& Plan,
		TArray<FString>& OutFindings);

private:
	static bool MapAddOns(
		TArray<FFlowGraphParsedNodeAddOn>& AddOns,
		const UFlowAsset& Asset,
		const FFlowMCPReplaceFlowNodeClassRequest& Request,
		TSet<FString>& SeenClassMappings,
		TSet<FString>& SeenPropertyMappings,
		TArray<FGuid>& ReplacedAddOnGuids,
		TArray<FString>& OutFindings);

	static bool PairEditorAddOns(
		const UFlowNodeBase& Original,
		UFlowNodeBase& Replacement,
		UFlowNode& RootNode,
		TArray<FString>& OutFindings);
};
