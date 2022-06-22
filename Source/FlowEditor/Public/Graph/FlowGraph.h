// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "EdGraph/EdGraph.h"

#include "FlowAsset.h"
#include "FlowGraph.generated.h"

class FLOWEDITOR_API FFlowGraphInterface final : public IFlowGraphInterface
{
public:
	virtual ~FFlowGraphInterface() {}

	virtual void OnInputTriggered(UEdGraphNode* GraphNode, const int32 Index) const override;
	virtual void OnOutputTriggered(UEdGraphNode* GraphNode, const int32 Index) const override;
};

UCLASS()
class FLOWEDITOR_API UFlowGraph : public UEdGraph
{
	GENERATED_UCLASS_BODY()

	static UEdGraph* CreateGraph(UFlowAsset* InFlowAsset);

	// UEdGraph
	virtual void NotifyGraphChanged() override;
	// --

	/** Returns the FlowAsset that contains this graph */
	UFlowAsset* GetFlowAsset() const;
};
