// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Find/FlowSearch.h"

#include "FlowAsset.h"
#include "Nodes/FlowNodeBase.h"
#include "AddOns/FlowNodeAddOn.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"
#include "Graph/Nodes/FlowGraphNode.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "UObject/TopLevelAssetPath.h"

//////////////////////////////////////////////////////////////////////////
// FFindInFlowCache

TMap<TWeakObjectPtr<UEdGraphNode>, TMap<EFlowSearchFlags, TSet<FString>>> FFindInFlowCache::CategoryStringCache;

void FFindInFlowCache::OnFlowAssetChanged(UFlowAsset& ChangedFlowAsset)
{
	TArray<TWeakObjectPtr<UEdGraphNode>> EntriesToRemove;

	for (const auto& KV : CategoryStringCache)
	{
		const TWeakObjectPtr<UEdGraphNode>& EdNodePtr = KV.Key;

		UEdGraphNode* EdNode = EdNodePtr.Get();

		if (!IsValid(EdNode))
		{
			EntriesToRemove.Add(EdNodePtr);
			continue;
		}

		UEdGraph* EdGraph = ChangedFlowAsset.GetGraph();
		if (EdGraph && EdGraph->Nodes.Contains(EdNode))
		{
			EntriesToRemove.Add(EdNodePtr);
		}
	}

	for (const TWeakObjectPtr<UEdGraphNode>& EdNodePtr : EntriesToRemove)
	{
		CategoryStringCache.Remove(EdNodePtr);
	}
}

//////////////////////////////////////////////////////////////////////////
// FFlowSearch

bool FFlowSearch::Search(const FFlowSearchQuery& Query, TArray<FFlowSearchResultItem>& OutResults)
{
	if (Query.SearchText.IsEmpty())
	{
		return false;
	}

	FSearchContext Ctx;
	Ctx.Flags    = Query.Flags;
	Ctx.MaxDepth = FMath::Max(Query.MaxDepth, 1);

	// Build upper-cased token list (AND semantics, same as SFindInFlow).
	Query.SearchText.ParseIntoArray(Ctx.Tokens, TEXT(" "), true);
	for (FString& Token : Ctx.Tokens)
	{
		Token = Token.ToUpper();
	}

	if (Ctx.Tokens.IsEmpty())
	{
		return false;
	}

	const int32 ResultsBefore = OutResults.Num();

	switch (Query.Scope)
	{
	case EFlowSearchScope::ThisAssetOnly:
		{
			UFlowAsset* Asset = Query.ContextAsset.Get();
			if (Asset && Asset->GetGraph())
			{
				ProcessAsset(Asset, Ctx, false, FSoftObjectPath(), OutResults);
			}
		}
		break;

	case EFlowSearchScope::AllOfThisType:
	case EFlowSearchScope::AllFlowAssets:
		{
			FAssetRegistryModule& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
			TArray<FAssetData> Assets;

			FARFilter Filter;
			Filter.bRecursiveClasses = true;

			if (Query.Scope == EFlowSearchScope::AllFlowAssets || !Query.ContextAsset.IsValid())
			{
				Filter.ClassPaths.Add(FTopLevelAssetPath(UFlowAsset::StaticClass()->GetClassPathName()));
			}
			else
			{
				Filter.ClassPaths.Add(FTopLevelAssetPath(Query.ContextAsset->GetClass()->GetClassPathName()));
			}

			Registry.Get().GetAssets(Filter, Assets);

			for (const FAssetData& Data : Assets)
			{
				UFlowAsset* Asset = Cast<UFlowAsset>(Data.GetAsset());
				if (IsValid(Asset) && Asset->GetGraph())
				{
					ProcessAsset(Asset, Ctx, false, FSoftObjectPath(), OutResults);
				}
			}
		}
		break;

	default:
		checkNoEntry();
		break;
	}

	return OutResults.Num() > ResultsBefore;
}

bool FFlowSearch::ProcessAsset(
	UFlowAsset* Asset,
	FSearchContext& Ctx,
	bool bIsSubGraphNode,
	const FSoftObjectPath& SubgraphOwnerPath,
	TArray<FFlowSearchResultItem>& OutResults)
{
	if (!Asset || !Asset->GetGraph() || Ctx.VisitedAssets.Contains(Asset))
	{
		return false;
	}

	Ctx.VisitedAssets.Add(Asset);

	const FSoftObjectPath AssetPath(Asset);
	bool bAnyMatches = false;

	for (UEdGraphNode* EdNode : Asset->GetGraph()->Nodes)
	{
		const TMap<EFlowSearchFlags, TSet<FString>>* CategoryStrings = BuildCategoryStrings(EdNode, Ctx);
		if (!CategoryStrings)
		{
			continue;
		}

		EFlowSearchFlags NodeMatchedFlags = EFlowSearchFlags::None;
		for (const TPair<EFlowSearchFlags, TSet<FString>>& Pair : *CategoryStrings)
		{
			if (EnumHasAnyFlags(Ctx.Flags, Pair.Key) && StringSetMatchesTokens(Ctx.Tokens, Pair.Value))
			{
				EnumAddFlags(NodeMatchedFlags, Pair.Key);
			}
		}

		if (NodeMatchedFlags != EFlowSearchFlags::None)
		{
			FFlowSearchResultItem Item;
			Item.AssetPath    = AssetPath;
			Item.MatchedFlags = NodeMatchedFlags;
			Item.bIsSubGraphNode     = bIsSubGraphNode;
			Item.SubgraphOwnerAssetPath = SubgraphOwnerPath;
			Item.MatchedSnippet = BuildMatchedSnippet(*CategoryStrings, NodeMatchedFlags, Ctx.Tokens);

			Item.NodeGuid = EdNode->NodeGuid;

			Item.NodeTitle = EdNode->GetNodeTitle(ENodeTitleType::ListView).ToString();
			if (Item.NodeTitle.IsEmpty())
			{
				Item.NodeTitle = EdNode->GetClass()->GetName();
			}

			if (const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(EdNode))
			{
				if (const UFlowNodeBase* Base = FlowGraphNode->GetFlowNodeBase())
				{
					Item.NodeTypeName = Base->GetClass()->GetName();
				}
			}
			if (Item.NodeTypeName.IsEmpty())
			{
				Item.NodeTypeName = EdNode->GetClass()->GetName();
			}

			OutResults.Add(Item);
			bAnyMatches = true;
		}

		bAnyMatches |= RecurseIntoSubgraphsIfEnabled(EdNode, AssetPath, Ctx, OutResults);
	}

	return bAnyMatches;
}

bool FFlowSearch::RecurseIntoSubgraphsIfEnabled(
	UEdGraphNode* EdNode,
	const FSoftObjectPath& OwnerAssetPath,
	FSearchContext& Ctx,
	TArray<FFlowSearchResultItem>& OutResults)
{
	if (!EnumHasAnyFlags(Ctx.Flags, EFlowSearchFlags::Subgraphs))
	{
		return false;
	}

	const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(EdNode);
	if (!FlowGraphNode || !FlowGraphNode->GetFlowNodeBase())
	{
		return false;
	}

	const UFlowNode_SubGraph* SubGraph = Cast<UFlowNode_SubGraph>(FlowGraphNode->GetFlowNodeBase());
	if (!SubGraph)
	{
		return false;
	}

	UFlowAsset* SubAsset = Cast<UFlowAsset>(const_cast<UFlowNode_SubGraph*>(SubGraph)->GetAssetToEdit());
	if (!SubAsset)
	{
		return false;
	}

	// Subgraphs don't count against depth - same convention as SFindInFlow.
	return ProcessAsset(SubAsset, Ctx, true, OwnerAssetPath, OutResults);
}

const TMap<EFlowSearchFlags, TSet<FString>>* FFlowSearch::BuildCategoryStrings(
	UEdGraphNode* EdNode,
	FSearchContext& Ctx)
{
	if (!IsValid(EdNode))
	{
		return nullptr;
	}

	// Reuse the same global cache that SFindInFlow uses so the two paths don't double-build.
	if (const TMap<EFlowSearchFlags, TSet<FString>>* Cached = FFindInFlowCache::CategoryStringCache.Find(EdNode))
	{
		return Cached;
	}

	TMap<EFlowSearchFlags, TSet<FString>> NewMap;
	UpdateCategoryStringsForEdGraphNode(*EdNode, Ctx, NewMap);

	if (const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(EdNode))
	{
		if (UFlowNodeBase* Base = FlowGraphNode->GetFlowNodeBase())
		{
			UpdateCategoryStringsForFlowNodeBase(*Base, Ctx, 0, NewMap);
		}
	}

	return &FFindInFlowCache::CategoryStringCache.Add(EdNode, MoveTemp(NewMap));
}

void FFlowSearch::UpdateCategoryStringsForEdGraphNode(
	const UEdGraphNode& EdGraphNode,
	const FSearchContext& Ctx,
	TMap<EFlowSearchFlags, TSet<FString>>& OutMap)
{
	if (EnumHasAnyFlags(Ctx.Flags, EFlowSearchFlags::Comments))
	{
		OutMap.FindOrAdd(EFlowSearchFlags::Comments).Add(EdGraphNode.NodeComment);
	}
}

void FFlowSearch::UpdateCategoryStringsForFlowNodeBase(
	const UFlowNodeBase& FlowNodeBase,
	const FSearchContext& Ctx,
	int32 Depth,
	TMap<EFlowSearchFlags, TSet<FString>>& OutMap)
{
	if (EnumHasAnyFlags(Ctx.Flags, EFlowSearchFlags::Titles))
	{
		OutMap.FindOrAdd(EFlowSearchFlags::Titles).Add(FlowNodeBase.GetNodeTitle().ToString());
	}

	if (EnumHasAnyFlags(Ctx.Flags, EFlowSearchFlags::Tooltips))
	{
		OutMap.FindOrAdd(EFlowSearchFlags::Tooltips).Add(FlowNodeBase.GetNodeToolTip().ToString());
	}

	if (EnumHasAnyFlags(Ctx.Flags, EFlowSearchFlags::Classes))
	{
		TSet<FString>& ClassesSet = OutMap.FindOrAdd(EFlowSearchFlags::Classes);
		ClassesSet.Add(FlowNodeBase.GetClass()->GetDisplayNameText().ToString());
		ClassesSet.Add(FlowNodeBase.GetClass()->GetName());
	}

	if (EnumHasAnyFlags(Ctx.Flags, EFlowSearchFlags::Descriptions))
	{
		OutMap.FindOrAdd(EFlowSearchFlags::Descriptions).Add(FlowNodeBase.GetNodeDescription());
	}

	if (EnumHasAnyFlags(Ctx.Flags, EFlowSearchFlags::ConfigText))
	{
		OutMap.FindOrAdd(EFlowSearchFlags::ConfigText).Add(FlowNodeBase.GetNodeConfigText().ToString());
	}

	if (EnumHasAnyFlags(Ctx.Flags, EFlowSearchFlags::PropertiesFlags))
	{
		AppendPropertyValues(&FlowNodeBase, FlowNodeBase.GetClass(), &FlowNodeBase, Ctx, Depth, OutMap);
	}

	if (EnumHasAnyFlags(Ctx.Flags, EFlowSearchFlags::AddOns))
	{
		FlowNodeBase.ForEachAddOnConst([&Ctx, &OutMap, Depth](const UFlowNodeAddOn& AddOn)
		{
			FFlowSearch::UpdateCategoryStringsForFlowNodeBase(AddOn, Ctx, Depth, OutMap);
			return EFlowForEachAddOnFunctionReturnValue::Continue;
		});
	}
}

void FFlowSearch::AppendPropertyValues(
	const void* Container,
	const UStruct* Struct,
	const UObject* ParentObject,
	const FSearchContext& Ctx,
	int32 Depth,
	TMap<EFlowSearchFlags, TSet<FString>>& OutMap)
{
	if (!Container || !Struct || !ParentObject || Depth >= Ctx.MaxDepth)
	{
		return;
	}

	for (TFieldIterator<FProperty> It(Struct, EFieldIteratorFlags::IncludeSuper); It; ++It)
	{
		FProperty* Prop = *It;
		if (!Prop->HasAnyPropertyFlags(CPF_Edit | CPF_SimpleDisplay | CPF_AdvancedDisplay | CPF_BlueprintVisible | CPF_Config))
		{
			continue;
		}

		const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Container);

		if (EnumHasAnyFlags(Ctx.Flags, EFlowSearchFlags::PropertyNames))
		{
			TSet<FString>& NamesSet = OutMap.FindOrAdd(EFlowSearchFlags::PropertyNames);
			const FString DisplayName = Prop->GetMetaData(TEXT("DisplayName"));
			if (!DisplayName.IsEmpty())
			{
				NamesSet.Add(DisplayName);
			}
			NamesSet.Add(Prop->GetName());
		}

		if (EnumHasAnyFlags(Ctx.Flags, EFlowSearchFlags::PropertyValues))
		{
			FString ValueStr;
			UObject* MutableParent = const_cast<UObject*>(ParentObject);
			Prop->ExportText_InContainer(0, ValueStr, Container, nullptr, MutableParent, PPF_None);
			ValueStr = ValueStr.Replace(TEXT("\""), TEXT("")).TrimStartAndEnd();
			OutMap.FindOrAdd(EFlowSearchFlags::PropertyValues).Add(ValueStr);
		}

		if (EnumHasAnyFlags(Ctx.Flags, EFlowSearchFlags::Tooltips))
		{
			OutMap.FindOrAdd(EFlowSearchFlags::Tooltips).Add(Prop->GetMetaData(TEXT("ToolTip")));
		}

		if (const FStructProperty* StructProp = CastField<FStructProperty>(Prop))
		{
			// Recurse into structs (no depth penalty - same as original).
			AppendPropertyValues(ValuePtr, StructProp->Struct, ParentObject, Ctx, Depth, OutMap);
		}
		else if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
		{
			UObject* Obj = ObjProp->GetObjectPropertyValue(ValuePtr);
			if (IsValid(Obj) && !Obj->HasAnyFlags(RF_ClassDefaultObject))
			{
				// Inline objects incur a depth penalty (same as original).
				AppendPropertyValues(Obj, Obj->GetClass(), Obj, Ctx, Depth + 1, OutMap);
			}
		}
	}
}

bool FFlowSearch::StringMatchesTokens(const TArray<FString>& Tokens, const FString& Str)
{
	const FString Upper = Str.ToUpper();
	for (const FString& Token : Tokens)
	{
		if (!Upper.Contains(Token))
		{
			return false;
		}
	}
	return true;
}

bool FFlowSearch::StringSetMatchesTokens(const TArray<FString>& Tokens, const TSet<FString>& StringSet)
{
	for (const FString& Str : StringSet)
	{
		if (StringMatchesTokens(Tokens, Str))
		{
			return true;
		}
	}
	return false;
}

FString FFlowSearch::BuildMatchedSnippet(
	const TMap<EFlowSearchFlags, TSet<FString>>& CategoryStrings,
	EFlowSearchFlags MatchedFlags,
	const TArray<FString>& Tokens)
{
	static const EFlowSearchFlags FlagOrder[] = {
		EFlowSearchFlags::Titles,
		EFlowSearchFlags::Classes,
		EFlowSearchFlags::Descriptions,
		EFlowSearchFlags::Comments,
		EFlowSearchFlags::ConfigText,
		EFlowSearchFlags::PropertyValues,
		EFlowSearchFlags::PropertyNames,
		EFlowSearchFlags::Tooltips,
		EFlowSearchFlags::AddOns,
	};

	// Walk the matched categories in priority order; return the first string that hits a token.
	for (EFlowSearchFlags Flag : FlagOrder)
	{
		if (!EnumHasAnyFlags(MatchedFlags, Flag))
		{
			continue;
		}
		const TSet<FString>* StringSet = CategoryStrings.Find(Flag);
		if (!StringSet)
		{
			continue;
		}
		for (const FString& Str : *StringSet)
		{
			if (StringMatchesTokens(Tokens, Str) && !Str.IsEmpty())
			{
				return Str.Left(120); // Truncate long property values for readability.
			}
		}
	}
	return FString();
}
