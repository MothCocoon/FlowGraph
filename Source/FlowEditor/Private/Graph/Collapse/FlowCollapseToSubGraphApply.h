// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Containers/Map.h"
#include "Internationalization/Text.h"
#include "Math/Vector2D.h"
#include "Misc/Guid.h"

class UEdGraphSchema;
class UFlowAsset;
class UFlowGraph;
class UFlowGraphNode;

namespace FlowCollapseToSubGraph
{
	struct FBoundaryPin;
	struct FCollapsePlan;

	/** Outcome of a collapse. */
	struct FCollapseResult
	{
		UFlowAsset* NewAsset = nullptr;
		UFlowGraphNode* SubGraphNode = nullptr;
		FText Error;
		bool bTransactionRecorded = false;

		bool Succeeded() const { return NewAsset != nullptr && Error.IsEmpty(); }
	};

	class FCollapseApply
	{
	public:
		FCollapseApply(const FCollapsePlan& InPlan, UFlowAsset& InSourceAsset, const FString& InNewAssetName);

		FCollapseResult Run();

	private:
		bool Fail(const FText& Error);
		bool InitializeSource();
		bool CreateAsset();
		bool CloneNodes();
		bool PrepareLayout();
		bool ConnectEntry();
		bool BuildExits();
		bool BuildExit(const FBoundaryPin& Exit, float ExitColumnPosX, float& NextExitNodePosY);
		UFlowGraphNode* CreateCustomOutput(const FBoundaryPin& Exit, float ExitColumnPosX, float& NextExitNodePosY);
		bool ReplaceSourceSelection();
		bool ReconnectSourceEntry();
		bool ReconnectSourceExits();
		void FinalizeNewAsset() const;
		void FinalizeSourceAsset() const;

		const FCollapsePlan& Plan;
		UFlowAsset& SourceAsset;
		const FString& NewAssetName;
		FCollapseResult Result;
		UFlowGraph* SourceGraph = nullptr;
		UFlowAsset* NewAsset = nullptr;
		UFlowGraph* NewGraph = nullptr;
		const UEdGraphSchema* NewGraphSchema = nullptr;
		UFlowGraphNode* StartNode = nullptr;
		UFlowGraphNode* FinishGraphNode = nullptr;
		UFlowGraphNode* SubGraphGraphNode = nullptr;
		TMap<FGuid, UFlowGraphNode*> ClonedByOriginalGuid;
		FVector2f ClonedRegionOrigin = FVector2f::ZeroVector;
		int32 ClonedRegionMaxX = 0;
	};

	/**
	 * Creates the new asset next to SourceAsset, moves the planned nodes into it, and replaces them in
	 * the source graph with a SubGraph node pointing at it.
	 *
	 * The caller owns the enclosing FScopedTransaction. Note that the new asset is a separate package
	 * and cannot be transacted away, so undo restores the source graph but leaves the new asset on disk.
	 */
	 FCollapseResult ApplyCollapse(
		const FCollapsePlan& Plan,
		UFlowAsset& SourceAsset,
		const FString& NewAssetName);
}
