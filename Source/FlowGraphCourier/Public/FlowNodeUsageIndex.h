// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "UObject/ObjectKey.h"

class FObjectPostSaveContext;
class UFlowAsset;
class UPackage;

/** How often one node or addon class appears in shipped content. */
struct FFlowNodeUsageSummary
{
	/** Total instances across every FlowAsset in the project. */
	int32 InstanceCount = 0;

	/** How many distinct FlowAssets contain at least one instance. */
	int32 AssetCount = 0;
};

/** One containing asset, ranked so the smallest and therefore most readable examples come first. */
struct FFlowNodeUsageExample
{
	FString AssetPath;

	/** Instances of the queried class in this asset. */
	int32 InstanceCount = 0;

	/** Total nodes in this asset - the readability proxy used for ranking. */
	int32 AssetNodeCount = 0;
};

/** One class commonly attached alongside the queried class, with how often. */
struct FFlowNodeUsageCoAttachment
{
	FString ClassName;
	FString ClassPath;
	int32 Count = 0;
};

/** Result for FFlowNodeUsageIndex::QueryUsage. */
struct FFlowNodeUsageResult
{
	FString ClassPath;
	int32 InstanceCount = 0;
	int32 AssetCount = 0;
	TArray<FFlowNodeUsageExample> Examples;
	TArray<FFlowNodeUsageCoAttachment> CoAttachedAddOns;

	/** One line per example instance. Populated only when snippets are explicitly requested. */
	TArray<FString> Snippets;
	FString ErrorMessage;
};

/**
 * Editor-side index of which FlowAssets instantiate which node and addon classes, built on demand
 * and refreshed per asset on save.
 *
 * Exists so "show me real usage of X" is one cheap lookup instead of a full re-harvest of every
 * FlowAsset in the project, and so the pins an *attached addon* contributes - which no CDO can
 * report - are still observable somewhere.
 */
class FLOWGRAPHCOURIER_API FFlowNodeUsageIndex
{
public:
	static FFlowNodeUsageIndex& Get();

	/** Counts only. Builds the index first if it has not been built yet. */
	FFlowNodeUsageSummary GetUsageSummary(const FString& ClassPath);

	/**
	 * Full usage answer for one class.
	 * @param ClassPath Full class path, short class name, or stem of the node/addon class.
	 * @param Limit Maximum number of example assets to return; 0 uses a small default.
	 * @param bIncludeSnippets True to also return a one-line description per example instance.
	 */
	FFlowNodeUsageResult QueryUsage(const FString& ClassPath, int32 Limit, bool bIncludeSnippets);

	/** Discards the index so the next query rebuilds it. */
	void Invalidate();

	/** Registers the save hook that keeps the index current. Safe to call more than once. */
	void RegisterCallbacks();
	void UnregisterCallbacks();

private:
	struct FClassUsageRecord
	{
		int32 InstanceCount = 0;

		/** Asset path -> instances of this class in that asset. */
		TMap<FString, int32> AssetToInstanceCount;

		/** Attached addon class path -> how many times it appeared on an instance of this class. */
		TMap<FString, int32> CoAttachedAddOnCounts;

		/** One line per instance, in discovery order. */
		TArray<FString> Snippets;
	};

	void EnsureBuilt();
	void IndexAsset(const UFlowAsset& FlowAsset);
	void ForgetAsset(const FString& AssetPath);
	void OnPackageSaved(const FString& PackageFileName, UPackage* Package, FObjectPostSaveContext Context);

	/** Class path -> usage. Keyed by path because a short name is not unique project-wide. */
	TMap<FString, FClassUsageRecord> ClassUsage;

	/** Asset path -> total node count, for the readability ranking. */
	TMap<FString, int32> AssetNodeCounts;
	bool bBuilt = false;
	FDelegateHandle PackageSavedHandle;
};
