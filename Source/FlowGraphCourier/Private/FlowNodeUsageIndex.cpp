// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowNodeUsageIndex.h"

#include "AddOns/FlowNodeAddOn.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "FlowAsset.h"
#include "FlowCatalogQuery.h"
#include "FlowLogChannels.h"
#include "Nodes/FlowNode.h"
#include "Nodes/FlowNodeBase.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/Package.h"

namespace
{
	constexpr int32 FlowNodeUsageDefaultExampleLimit = 5;

	/** One readable line per instance: which asset, which node, and what was attached to it. */
	FString MakeInstanceSnippet(const UFlowAsset& FlowAsset, const UFlowNodeBase& Instance)
	{
		TArray<FString> AddOnNames;
		for (const UFlowNodeAddOn* AddOn : Instance.GetFlowNodeAddOnChildren())
		{
			if (AddOn)
			{
				AddOnNames.Add(AddOn->GetClass()->GetName());
			}
		}

		FString Snippet = FString::Printf(TEXT("%s :: %s"), *FlowAsset.GetPathName(), *Instance.GetName());
		if (!AddOnNames.IsEmpty())
		{
			Snippet += FString::Printf(TEXT(" [addons: %s]"), *FString::Join(AddOnNames, TEXT(", ")));
		}
		return Snippet;
	}
}

FFlowNodeUsageIndex& FFlowNodeUsageIndex::Get()
{
	static FFlowNodeUsageIndex Instance;
	return Instance;
}

void FFlowNodeUsageIndex::Invalidate()
{
	ClassUsage.Reset();
	AssetNodeCounts.Reset();
	bBuilt = false;
}

void FFlowNodeUsageIndex::RegisterCallbacks()
{
	if (!PackageSavedHandle.IsValid())
	{
		PackageSavedHandle = UPackage::PackageSavedWithContextEvent.AddRaw(this, &FFlowNodeUsageIndex::OnPackageSaved);
	}
}

void FFlowNodeUsageIndex::UnregisterCallbacks()
{
	if (PackageSavedHandle.IsValid())
	{
		UPackage::PackageSavedWithContextEvent.Remove(PackageSavedHandle);
		PackageSavedHandle.Reset();
	}
}

void FFlowNodeUsageIndex::OnPackageSaved(const FString& PackageFileName, UPackage* Package, FObjectPostSaveContext Context)
{
	if (!bBuilt || !Package)
	{
		return;
	}

	// Re-index only the saved asset to keep save-time work proportional to that package.
	TArray<UObject*> Objects;
	GetObjectsWithOuter(Package, Objects, /*bIncludeNestedObjects=*/false);
	for (UObject* Object : Objects)
	{
		if (const UFlowAsset* FlowAsset = Cast<UFlowAsset>(Object))
		{
			ForgetAsset(FlowAsset->GetPathName());
			IndexAsset(*FlowAsset);
		}
	}
}

void FFlowNodeUsageIndex::ForgetAsset(const FString& AssetPath)
{
	AssetNodeCounts.Remove(AssetPath);

	for (auto It = ClassUsage.CreateIterator(); It; ++It)
	{
		FClassUsageRecord& Record = It.Value();
		int32 RemovedInstances = 0;
		Record.AssetToInstanceCount.RemoveAndCopyValue(AssetPath, RemovedInstances);
		Record.InstanceCount -= RemovedInstances;

		Record.Snippets.RemoveAll([&AssetPath](const FString& Snippet) { return Snippet.StartsWith(AssetPath + TEXT(" ::")); });

		if (Record.AssetToInstanceCount.IsEmpty())
		{
			It.RemoveCurrent();
		}
	}
}

void FFlowNodeUsageIndex::IndexAsset(const UFlowAsset& FlowAsset)
{
	const FString AssetPath = FlowAsset.GetPathName();
	const TMap<FGuid, UFlowNode*>& Nodes = FlowAsset.GetNodes();
	AssetNodeCounts.Add(AssetPath, Nodes.Num());

	// Recording an addon against its parent's record is what makes "commonly co-attached" answerable,
	// and is the only place a pin contributed by an attached addon is observable at all.
	auto RecordInstance = [this, &FlowAsset, &AssetPath](const UFlowNodeBase& Instance, const UFlowNodeBase* Parent)
	{
		const FString ClassPath = Instance.GetClass()->GetPathName();
		FClassUsageRecord& Record = ClassUsage.FindOrAdd(ClassPath);
		++Record.InstanceCount;
		++Record.AssetToInstanceCount.FindOrAdd(AssetPath);
		Record.Snippets.Add(MakeInstanceSnippet(FlowAsset, Instance));

		if (Parent)
		{
			FClassUsageRecord& ParentRecord = ClassUsage.FindOrAdd(Parent->GetClass()->GetPathName());
			++ParentRecord.CoAttachedAddOnCounts.FindOrAdd(ClassPath);
		}
	};

	for (const TPair<FGuid, UFlowNode*>& Pair : Nodes)
	{
		const UFlowNode* Node = Pair.Value;
		if (!Node)
		{
			continue;
		}

		RecordInstance(*Node, nullptr);

		Node->ForEachAddOnConst([&RecordInstance, Node](const UFlowNodeAddOn& AddOn)
		{
			RecordInstance(AddOn, Node);
			return EFlowForEachAddOnFunctionReturnValue::Continue;
		});
	}
}

void FFlowNodeUsageIndex::EnsureBuilt()
{
	if (bBuilt)
	{
		return;
	}

	// Set the flag before the sweep so a re-entrant query cannot start a second build.
	bBuilt = true;
	RegisterCallbacks();

	FARFilter Filter;
	Filter.ClassPaths.Add(UFlowAsset::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> FoundAssets;
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	AssetRegistryModule.Get().GetAssets(Filter, FoundAssets);

	int32 IndexedAssetCount = 0;
	for (const FAssetData& AssetData : FoundAssets)
	{
		if (const UFlowAsset* FlowAsset = Cast<UFlowAsset>(AssetData.GetAsset()))
		{
			IndexAsset(*FlowAsset);
			++IndexedAssetCount;
		}
	}

	UE_LOG(LogFlow, Log, TEXT("FlowNodeUsageIndex: indexed %d FlowAssets covering %d node/addon classes."),
		IndexedAssetCount, ClassUsage.Num());
}

FFlowNodeUsageSummary FFlowNodeUsageIndex::GetUsageSummary(const FString& ClassPath)
{
	EnsureBuilt();

	FFlowNodeUsageSummary Summary;
	if (const FClassUsageRecord* Record = ClassUsage.Find(ClassPath))
	{
		Summary.InstanceCount = Record->InstanceCount;
		Summary.AssetCount = Record->AssetToInstanceCount.Num();
	}
	return Summary;
}

FFlowNodeUsageResult FFlowNodeUsageIndex::QueryUsage(const FString& ClassPath, int32 Limit, bool bIncludeSnippets)
{
	FFlowNodeUsageResult Result;

	// Accept whatever identifier the caller has to hand, then answer in terms of the authoritative one.
	const UClass* ResolvedClass = UFlowCatalogQuery::FindFlowNodeOrAddOnClassByName(ClassPath);
	if (!ResolvedClass)
	{
		Result.ErrorMessage = FString::Printf(TEXT("No Flow node or addon class resolved from \"%s\"."), *ClassPath);
		return Result;
	}

	EnsureBuilt();

	Result.ClassPath = ResolvedClass->GetPathName();

	const FClassUsageRecord* Record = ClassUsage.Find(Result.ClassPath);
	if (!Record)
	{
		// Zero usage is a real, useful answer - not an error.
		return Result;
	}

	Result.InstanceCount = Record->InstanceCount;
	Result.AssetCount = Record->AssetToInstanceCount.Num();

	for (const TPair<FString, int32>& Pair : Record->AssetToInstanceCount)
	{
		FFlowNodeUsageExample& Example = Result.Examples.AddDefaulted_GetRef();
		Example.AssetPath = Pair.Key;
		Example.InstanceCount = Pair.Value;
		Example.AssetNodeCount = AssetNodeCounts.FindRef(Pair.Key);
	}

	// Smallest graph first: a 6-node asset teaches the node's shape far better than a 90-node one.
	Result.Examples.Sort([](const FFlowNodeUsageExample& A, const FFlowNodeUsageExample& B)
	{
		if (A.AssetNodeCount != B.AssetNodeCount)
		{
			return A.AssetNodeCount < B.AssetNodeCount;
		}
		return A.AssetPath < B.AssetPath;
	});

	const int32 EffectiveLimit = Limit > 0 ? Limit : FlowNodeUsageDefaultExampleLimit;
	if (Result.Examples.Num() > EffectiveLimit)
	{
		Result.Examples.SetNum(EffectiveLimit);
	}

	for (const TPair<FString, int32>& Pair : Record->CoAttachedAddOnCounts)
	{
		FFlowNodeUsageCoAttachment& CoAttachment = Result.CoAttachedAddOns.AddDefaulted_GetRef();
		CoAttachment.ClassPath = Pair.Key;
		CoAttachment.Count = Pair.Value;
		if (const UClass* AddOnClass = FindObject<UClass>(nullptr, *Pair.Key))
		{
			CoAttachment.ClassName = AddOnClass->GetName();
		}
	}
	Result.CoAttachedAddOns.Sort([](const FFlowNodeUsageCoAttachment& A, const FFlowNodeUsageCoAttachment& B)
	{
		return A.Count > B.Count;
	});

	if (bIncludeSnippets)
	{
		// Snippets are the expensive part of the payload, so they are only ever emitted on request and
		// only for the examples actually returned.
		for (const FFlowNodeUsageExample& Example : Result.Examples)
		{
			for (const FString& Snippet : Record->Snippets)
			{
				if (Snippet.StartsWith(Example.AssetPath + TEXT(" ::")))
				{
					Result.Snippets.Add(Snippet);
				}
			}
		}
	}

	return Result;
}
